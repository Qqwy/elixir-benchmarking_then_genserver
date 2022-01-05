.PHONY: run run_jit run_emu

ERTS_BIN="./OTP-24.2.0-bundle/otp/bin/"

run: run_jit run_emu

run_jit:
	ERTS_BIN=$(ERTS_BIN) ELIXIR_ERL_OPTIONS="-emu_flavor jit" mix run lib/benchmark.exs

run_emu:
	ERTS_BIN=$(ERTS_BIN) ELIXIR_ERL_OPTIONS="-emu_flavor emu" mix run lib/benchmark.exs

compile_erlang:
	cd ./OTP-24.2.0-bundle/otp/ && ./configure --enable-jit
	MAKEFLAGS="j16" FLAVOR=jit cd ./OTP-24.2.0-bundle/otp/ && make
	MAKEFLAGS="j16" FLAVOR=emu cd ./OTP-24.2.0-bundle/otp/ && make
