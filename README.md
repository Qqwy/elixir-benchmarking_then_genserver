# BenchmarkingThenGenserver

## How to run

1. Edit the `elixir` file (locate it using `which elixir`), by commenting the line `ERTS_BIN=`.
This allows us to override the location of the Erlang Runtime System by setting this environment variable ourselves.
2. Compile Erlang with both JIT and EMU support. This is automated by the `compile_erlang` make-command (`make compile_erlang`.)
