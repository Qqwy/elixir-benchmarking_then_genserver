# BenchmarkingThenGenserver

## How to run

_NOTE: These steps work on x86_64 versions of Linux. They probably need changes to run on e.g. OSX, especially the step where Erlang is built._

1. Compile Erlang with both JIT and EMU support. This is automated by the `compile_erlang` make-command (`make compile_erlang`.)
2. Edit the `elixir` 'executable' file (it is actually a shell script) , by commenting the line `ERTS_BIN=`. (locate it using `which elixir`).
This allows us to override the location of the Erlang Runtime System by setting this environment variable ourselves.
3. Run `make run` to run the benchmark in both JIT and EMU modes.

## What do we benchmark?

To check the overhead of the extra anonymous (capture-free) function introduced by the `Kernel.then/2`-macro in a realistic scenario, we build a GenServer whose `handle_cast`-callback uses it as final step of a pipeline.

This is compared to `Manual` a version where we write the body of this anonymous function by hand (we 'manually' inline/desugar it.) and `ThenInlined` where we add `@compiler :inline` to ensure the compiler is allowed to inline all in-module function calls.

The GenServer is a simple 'adder' where during a single run we:
- start the GenServer
- call the async addition callback many times
- Read the state once
- Stop the GenServer

## Results

It seems that `Then` is consistently 5% slower when not inlined, even if it is tail-recursion-optimized.
