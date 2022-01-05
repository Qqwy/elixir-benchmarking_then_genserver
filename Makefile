.PHONY: run run_jit run_emu

ERTS_BIN="/run/media/qqwy/Serendipity/Programming/Personal/erlang/otp_24/OTP-24.2.0-bundle/otp/bin"

run: run_jit run_emu

run_jit:
	ERTS_BIN=$(ERTS_BIN) mix run lib/benchmark.exs --erl "-emu_flavor jit"
run_emu:
	ERTS_BIN=$(ERTS_BIN) mix run lib/benchmark.exs --erl "-emu_flavor emu"

compile_erlang:
	cd ./OTP-24.2.0-bundle/otp/ && ./configure
	MAKEFLAGS="j8" FLAVOR=jit cd ./OTP-24.2.0-bundle/otp/ && make
	MAKEFLAGS="j8" FLAVOR=emu cd ./OTP-24.2.0-bundle/otp/ && make
