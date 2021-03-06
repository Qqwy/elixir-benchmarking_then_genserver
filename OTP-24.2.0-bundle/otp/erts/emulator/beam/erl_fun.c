/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 2000-2021. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * %CopyrightEnd%
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_fun.h"
#include "hash.h"

static Hash erts_fun_table;

static erts_rwmtx_t erts_fun_table_lock;

#define erts_fun_read_lock()	erts_rwmtx_rlock(&erts_fun_table_lock)
#define erts_fun_read_unlock()	erts_rwmtx_runlock(&erts_fun_table_lock)
#define erts_fun_write_lock()	erts_rwmtx_rwlock(&erts_fun_table_lock)
#define erts_fun_write_unlock()	erts_rwmtx_rwunlock(&erts_fun_table_lock)

static HashValue fun_hash(ErlFunEntry* obj);
static int fun_cmp(ErlFunEntry* obj1, ErlFunEntry* obj2);
static ErlFunEntry* fun_alloc(ErlFunEntry* template);
static void fun_free(ErlFunEntry* obj);

/*
 * The address field of every fun that has no loaded code will point
 * to unloaded_fun[]. The -1 in unloaded_fun[0] will be interpreted
 * as an illegal arity when attempting to call a fun.
 */
static BeamInstr unloaded_fun_code[4] = {NIL, NIL, -1, 0};
static ErtsCodePtr unloaded_fun = &unloaded_fun_code[3];

void
erts_init_fun_table(void)
{
    HashFunctions f;
    erts_rwmtx_opt_t rwmtx_opt = ERTS_RWMTX_OPT_DEFAULT_INITER;
    rwmtx_opt.type = ERTS_RWMTX_TYPE_FREQUENT_READ;
    rwmtx_opt.lived = ERTS_RWMTX_LONG_LIVED;

    erts_rwmtx_init_opt(&erts_fun_table_lock, &rwmtx_opt, "fun_tab", NIL,
        ERTS_LOCK_FLAGS_PROPERTY_STATIC | ERTS_LOCK_FLAGS_CATEGORY_GENERIC);

    f.hash = (H_FUN) fun_hash;
    f.cmp  = (HCMP_FUN) fun_cmp;
    f.alloc = (HALLOC_FUN) fun_alloc;
    f.free = (HFREE_FUN) fun_free;
    f.meta_alloc = (HMALLOC_FUN) erts_alloc;
    f.meta_free = (HMFREE_FUN) erts_free;
    f.meta_print = (HMPRINT_FUN) erts_print;

    hash_init(ERTS_ALC_T_FUN_TABLE, &erts_fun_table, "fun_table", 16, f);
}

void
erts_fun_info(fmtfn_t to, void *to_arg)
{
    int lock = !ERTS_IS_CRASH_DUMPING;
    if (lock)
	erts_fun_read_lock();
    hash_info(to, to_arg, &erts_fun_table);
    if (lock)
	erts_fun_read_unlock();
}

int erts_fun_table_sz(void)
{
    int sz;
    int lock = !ERTS_IS_CRASH_DUMPING;
    if (lock)
	erts_fun_read_lock();
    sz = hash_table_sz(&erts_fun_table);
    if (lock)
	erts_fun_read_unlock();
    return sz;
}

ErlFunEntry*
erts_put_fun_entry2(Eterm mod, int old_uniq, int old_index,
		    const byte* uniq, int index, int arity)
{
    ErlFunEntry template;
    ErlFunEntry* fe;
    erts_aint_t refc;

    ASSERT(is_atom(mod));
    template.old_uniq = old_uniq;
    template.index = index;
    template.module = mod;
    erts_fun_write_lock();
    fe = (ErlFunEntry *) hash_put(&erts_fun_table, (void*) &template);
    sys_memcpy(fe->uniq, uniq, sizeof(fe->uniq));
    fe->old_index = old_index;
    fe->arity = arity;
    refc = erts_refc_inctest(&fe->refc, 0);
    if (refc < 2) /* New or pending delete */
	erts_refc_inc(&fe->refc, 1);
    erts_fun_write_unlock();
    return fe;
}

ErlFunEntry*
erts_get_fun_entry(Eterm mod, int uniq, int index)
{
    ErlFunEntry template;
    ErlFunEntry *ret;

    ASSERT(is_atom(mod));
    template.old_uniq = uniq;
    template.index = index;
    template.module = mod;
    erts_fun_read_lock();
    ret = (ErlFunEntry *) hash_get(&erts_fun_table, (void*) &template);
    if (ret) {
	erts_aint_t refc = erts_refc_inctest(&ret->refc, 1);
	if (refc < 2) /* Pending delete */
	    erts_refc_inc(&ret->refc, 1);
    }
    erts_fun_read_unlock();
    return ret;
}

int erts_is_fun_loaded(ErlFunEntry* fe) {
    int res = fe->address != unloaded_fun;

    ASSERT(res == (0 != ((const BeamInstr*)fe->address)[0]));

    return res;
}

static void
erts_erase_fun_entry_unlocked(ErlFunEntry* fe)
{
    hash_erase(&erts_fun_table, (void *) fe);
}

void
erts_erase_fun_entry(ErlFunEntry* fe)
{
    erts_fun_write_lock();
    /*
     * We have to check refc again since someone might have looked up
     * the fun entry and incremented refc after last check.
     */
    if (erts_refc_dectest(&fe->refc, -1) <= 0)
    {
	if (fe->address != unloaded_fun)
	    erts_exit(ERTS_ERROR_EXIT,
		     "Internal error: "
		     "Invalid reference count found on #Fun<%T.%d.%d>: "
		     " About to erase fun still referred by code.\n",
		     fe->module, fe->old_index, fe->old_uniq);
	erts_erase_fun_entry_unlocked(fe);
    }
    erts_fun_write_unlock();
}

static void fun_purge_foreach(ErlFunEntry *fe, struct erl_module_instance* modp)
{
    const char *fun_addr, *mod_start;

    fun_addr = (const char*)fe->address;
    mod_start = (const char*)modp->code_hdr;

    if (ErtsInArea(fun_addr, mod_start, modp->code_length)) {
        fe->pend_purge_address = fe->address;
        ERTS_THR_WRITE_MEMORY_BARRIER;
        fe->address = unloaded_fun;
        erts_purge_state_add_fun(fe);
    }
}

void
erts_fun_purge_prepare(struct erl_module_instance* modp)
{
    erts_fun_read_lock();
    hash_foreach(&erts_fun_table, (HFOREACH_FUN)fun_purge_foreach, modp);
    erts_fun_read_unlock();
}

void
erts_fun_purge_abort_prepare(ErlFunEntry **funs, Uint no)
{
    Uint ix;

    for (ix = 0; ix < no; ix++) {
        ErlFunEntry *fe = funs[ix];

        if (fe->address == unloaded_fun) {
            fe->address = fe->pend_purge_address;
        }
    }
}

void
erts_fun_purge_abort_finalize(ErlFunEntry **funs, Uint no)
{
    Uint ix;

    for (ix = 0; ix < no; ix++) {
        funs[ix]->pend_purge_address = NULL;
    }
}

void
erts_fun_purge_complete(ErlFunEntry **funs, Uint no)
{
    Uint ix;

    for (ix = 0; ix < no; ix++) {
	ErlFunEntry *fe = funs[ix];
	fe->pend_purge_address = NULL;
	if (erts_refc_dectest(&fe->refc, 0) == 0)
	    erts_erase_fun_entry(fe);
    }
    ERTS_THR_WRITE_MEMORY_BARRIER;
}

struct dump_fun_foreach_args {
    fmtfn_t to;
    void *to_arg;
};

static void
dump_fun_foreach(ErlFunEntry *fe, struct dump_fun_foreach_args *args)
{
    erts_print(args->to, args->to_arg, "=fun\n");
    erts_print(args->to, args->to_arg, "Module: %T\n", fe->module);
    erts_print(args->to, args->to_arg, "Uniq: %d\n", fe->old_uniq);
    erts_print(args->to, args->to_arg, "Index: %d\n",fe->old_index);
    erts_print(args->to, args->to_arg, "Address: %p\n", fe->address);
    erts_print(args->to, args->to_arg, "Refc: %ld\n", erts_refc_read(&fe->refc, 1));
}

void
erts_dump_fun_entries(fmtfn_t to, void *to_arg)
{
    struct dump_fun_foreach_args args = {to, to_arg};
    int lock = !ERTS_IS_CRASH_DUMPING;

    if (lock)
	erts_fun_read_lock();
    hash_foreach(&erts_fun_table, (HFOREACH_FUN)dump_fun_foreach, &args);
    if (lock)
	erts_fun_read_unlock();
}

static HashValue
fun_hash(ErlFunEntry* obj)
{
    return (HashValue) (obj->old_uniq ^ obj->index ^ atom_val(obj->module));
}

static int
fun_cmp(ErlFunEntry* obj1, ErlFunEntry* obj2)
{
    /*
     * OTP 23: Use 'index' (instead of 'old_index') when comparing fun
     * entries. In OTP 23, multiple make_fun2 instructions may refer to the
     * the same 'index' (for the wrapper function generated for the
     * 'fun F/A' syntax).
     *
     * This is safe when loading code compiled with OTP R15 and later,
     * because since R15 (2011), the 'index' has been reliably equal
     * to 'old_index'. The loader refuses to load modules compiled before
     * OTP R15.
     */

    return !(obj1->module == obj2->module &&
	     obj1->old_uniq == obj2->old_uniq &&
	     obj1->index == obj2->index);
}

static ErlFunEntry*
fun_alloc(ErlFunEntry* template)
{
    ErlFunEntry* obj = (ErlFunEntry *) erts_alloc(ERTS_ALC_T_FUN_ENTRY,
						  sizeof(ErlFunEntry));

    obj->old_uniq = template->old_uniq;
    obj->index = template->index;
    obj->module = template->module;
    erts_refc_init(&obj->refc, -1);
    obj->address = unloaded_fun;
    obj->pend_purge_address = NULL;
    return obj;
}

static void
fun_free(ErlFunEntry* obj)
{
    erts_free(ERTS_ALC_T_FUN_ENTRY, (void *) obj);
}
