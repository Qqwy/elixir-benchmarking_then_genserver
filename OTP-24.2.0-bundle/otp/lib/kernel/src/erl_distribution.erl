%%
%% %CopyrightBegin%
%% 
%% Copyright Ericsson AB 1996-2021. All Rights Reserved.
%% 
%% Licensed under the Apache License, Version 2.0 (the "License");
%% you may not use this file except in compliance with the License.
%% You may obtain a copy of the License at
%%
%%     http://www.apache.org/licenses/LICENSE-2.0
%%
%% Unless required by applicable law or agreed to in writing, software
%% distributed under the License is distributed on an "AS IS" BASIS,
%% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
%% See the License for the specific language governing permissions and
%% limitations under the License.
%% 
%% %CopyrightEnd%
%%
-module(erl_distribution).

-behaviour(supervisor).

-include_lib("kernel/include/logger.hrl").

-export([start_link/0,start_link/3,init/1,start/1,stop/0]).

-define(DBG,erlang:display([?MODULE,?LINE])).

%% Called during system start-up.

start_link() ->
    do_start_link([{sname,shortnames},{name,longnames}]).

%% Called from net_kernel:start/1 to start distribution after the
%% system has already started.

start(Args) ->
    C = #{id => net_sup_dynamic,
          start => {?MODULE,start_link,[Args,false,net_sup_dynamic]},
          restart => permanent,
          shutdown => 1000,
          type => supervisor,
          modules => [erl_distribution]},
    supervisor:start_child(kernel_sup, C).

%% Stop distribution.

stop() ->
    case supervisor:terminate_child(kernel_sup, net_sup_dynamic) of
	ok ->
	    supervisor:delete_child(kernel_sup, net_sup_dynamic);
	Error ->
	    case whereis(net_sup) of
		Pid when is_pid(Pid) ->
		    %% Dist. started through -sname | -name flags
		    {error, not_allowed};
		_ ->
		    Error
	    end
    end.

%%%
%%% Internal helper functions.
%%%

%% Helper start function.

start_link(Args, CleanHalt, NetSup) ->
    supervisor:start_link({local,net_sup}, ?MODULE, [Args,CleanHalt,NetSup]).

init(NetArgs) ->
    Epmd = 
	case init:get_argument(no_epmd) of
	    {ok, [[]]} ->
		[];
	    _ ->
		EpmdMod = net_kernel:epmd_module(),
		[#{id => EpmdMod,
                   start => {EpmdMod,start_link,[]},
                   restart => permanent,
                   shutdown => 2000,
                   type => worker,
                   modules => [EpmdMod]}]
	end,
    Auth = #{id => auth,
             start => {auth,start_link,[]},
             restart => permanent,
             shutdown => 2000,
             type => worker,
             modules => [auth]},
    Kernel = #{id => net_kernel,
               start => {net_kernel,start_link,NetArgs},
               restart => permanent,
               shutdown => 2000,
               type => worker,
               modules => [net_kernel]},
    EarlySpecs = net_kernel:protocol_childspecs(),
    SupFlags = #{strategy => one_for_all,
                 intensity => 0,
                 period => 1},
    {ok, {SupFlags, EarlySpecs ++ Epmd ++ [Auth,Kernel]}}.

do_start_link([{Arg,Flag}|T]) ->
    case init:get_argument(Arg) of
	{ok,[[Name]]} ->
	    start_link([list_to_atom(Name),Flag|ticktime()], true, net_sup);
        {ok,[[Name]|_Rest]} ->
            ?LOG_WARNING("Multiple -~p given to erl, using the first, ~p",
                         [Arg, Name]),
	    start_link([list_to_atom(Name),Flag|ticktime()], true, net_sup);
        {ok,[Invalid|_]} ->
            ?LOG_ERROR("Invalid -~p given to erl, ~ts",
                       [Arg, lists:join(" ",Invalid)]),
	    do_start_link(T);
	_ ->
	    do_start_link(T)
    end;
do_start_link([]) ->
    ignore.

ticktime() ->
    %% catch, in case the system was started with boot file start_old,
    %% i.e. running without the application_controller.
    %% Time is given in seconds. The net_kernel tick time is
    %% Time/4 milliseconds.
    case catch application:get_env(net_ticktime) of
	{ok, Value} when is_integer(Value), Value > 0 ->
	    [Value * 250]; %% i.e. 1000 / 4 = 250 ms.
	_ ->
	    []
    end.
