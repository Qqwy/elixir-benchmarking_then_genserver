# BenchmarkingThenGenserver

An approach to benchmark the overhead of `Kernel.then/2` vs inlining it, in a more 'realistic' scenario of using it inside the `handle_cast` callback of a GenServer.

## How to run

_NOTE: These steps work on x86_64 versions of Linux. They probably need changes to run on e.g. OSX, especially the step where Erlang is built._

1. Compile Erlang with both JIT and EMU support. This is automated by the `compile_erlang` make-command (`make compile_erlang`.)
2. Edit the `elixir` 'executable' file (it is actually a shell script) , by commenting the line `ERTS_BIN=`. (locate it using `which elixir`).
This allows us to override the location of the Erlang Runtime System by setting this environment variable, which is used internally by the final step to ensure we can use both the JIT and EMU versions of Erlang.
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

It seems that all three variants execute similarly efficiently (within 5% of each-other, with different ones being the fastest between input sets and benchmark runs, i.e. within margin of error.)
Also, memory usage is the same.

### JIT

```
ERTS_BIN="./OTP-24.2.0-bundle/otp/bin/" ELIXIR_ERL_OPTIONS="-emu_flavor jit" mix run lib/benchmark.exs
Using emu_flavor: jit
Error trying to dermine erlang version enoent
Operating System: Linux
CPU Information: Intel(R) Core(TM) i7-6700HQ CPU @ 2.60GHz
Number of Available Cores: 8
Available memory: 31.18 GB
Elixir 1.13.1
Erlang ok

Benchmark suite executing with the following configuration:
warmup: 2 s
time: 2 s
memory time: 2 s
parallel: 1
inputs: 1, 10, 100, 1000, 10000, 100000
Estimated total run time: 1.80 min

Benchmarking Manual with input 1...
Benchmarking Manual with input 10...
Benchmarking Manual with input 100...
Benchmarking Manual with input 1000...
Benchmarking Manual with input 10000...
Benchmarking Manual with input 100000...
Benchmarking Then with input 1...
Benchmarking Then with input 10...
Benchmarking Then with input 100...
Benchmarking Then with input 1000...
Benchmarking Then with input 10000...
Benchmarking Then with input 100000...
Benchmarking ThenInlined with input 1...
Benchmarking ThenInlined with input 10...
Benchmarking ThenInlined with input 100...
Benchmarking ThenInlined with input 1000...
Benchmarking ThenInlined with input 10000...
Benchmarking ThenInlined with input 100000...

##### With input 1 #####
Name                  ips        average  deviation         median         99th %
Manual            70.19 K       14.25 μs    ±52.32%       12.75 μs       36.28 μs
Then              69.77 K       14.33 μs    ±49.31%       12.82 μs       38.72 μs
ThenInlined       69.39 K       14.41 μs    ±51.05%       12.82 μs       37.22 μs

Comparison: 
Manual            70.19 K
Then              69.77 K - 1.01x slower +0.0858 μs
ThenInlined       69.39 K - 1.01x slower +0.166 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Manual             903.89 B     ±0.52%          904 B          904 B
Then               903.87 B     ±0.57%          904 B          904 B
ThenInlined        903.90 B     ±0.50%          904 B          904 B

Comparison: 
Manual                904 B
Then               903.87 B - 1.00x memory usage -0.02375 B
ThenInlined        903.90 B - 1.00x memory usage +0.00908 B

##### With input 10 #####
Name                  ips        average  deviation         median         99th %
ThenInlined       52.58 K       19.02 μs    ±40.66%       16.56 μs       50.85 μs
Manual            52.46 K       19.06 μs    ±40.86%       16.56 μs       51.58 μs
Then              52.10 K       19.19 μs    ±43.66%       16.56 μs       50.92 μs

Comparison: 
ThenInlined       52.58 K
Manual            52.46 K - 1.00x slower +0.0446 μs
Then              52.10 K - 1.01x slower +0.176 μs

Memory usage statistics:

Name                average  deviation         median         99th %
ThenInlined         1.30 KB     ±0.00%        1.30 KB        1.30 KB
Manual              1.30 KB     ±0.06%        1.30 KB        1.30 KB
Then                1.30 KB     ±0.25%        1.30 KB        1.30 KB

Comparison: 
ThenInlined         1.30 KB
Manual              1.30 KB - 1.00x memory usage -0.00000 KB
Then                1.30 KB - 1.00x memory usage -0.00005 KB

##### With input 100 #####
Name                  ips        average  deviation         median         99th %
Then              19.08 K       52.42 μs    ±20.94%       48.37 μs      105.24 μs
Manual            18.75 K       53.32 μs    ±22.57%       48.47 μs      105.57 μs
ThenInlined       17.31 K       57.77 μs    ±23.10%       51.64 μs      105.10 μs

Comparison: 
Then              19.08 K
Manual            18.75 K - 1.02x slower +0.90 μs
ThenInlined       17.31 K - 1.10x slower +5.34 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Then                5.52 KB     ±0.05%        5.52 KB        5.52 KB
Manual              5.52 KB     ±0.10%        5.52 KB        5.52 KB
ThenInlined         5.52 KB     ±0.04%        5.52 KB        5.52 KB

Comparison: 
Then                5.52 KB
Manual              5.52 KB - 1.00x memory usage -0.00012 KB
ThenInlined         5.52 KB - 1.00x memory usage +0.00001 KB

##### With input 1000 #####
Name                  ips        average  deviation         median         99th %
Then               2.84 K      352.18 μs    ±13.71%      339.34 μs      545.01 μs
Manual             2.71 K      368.88 μs    ±19.17%      350.88 μs      747.43 μs
ThenInlined        2.58 K      387.29 μs    ±10.23%      380.81 μs      540.37 μs

Comparison: 
Then               2.84 K
Manual             2.71 K - 1.05x slower +16.71 μs
ThenInlined        2.58 K - 1.10x slower +35.11 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Then               47.71 KB     ±0.03%       47.71 KB       47.71 KB
Manual             47.71 KB     ±0.03%       47.71 KB       47.71 KB
ThenInlined        47.71 KB     ±0.04%       47.71 KB       47.71 KB

Comparison: 
Then               47.71 KB
Manual             47.71 KB - 1.00x memory usage +0.00010 KB
ThenInlined        47.71 KB - 1.00x memory usage -0.00119 KB

##### With input 10000 #####
Name                  ips        average  deviation         median         99th %
Manual             243.86        4.10 ms    ±12.58%        4.09 ms        6.06 ms
Then               238.00        4.20 ms    ±14.13%        4.14 ms        7.79 ms
ThenInlined        230.07        4.35 ms    ±11.95%        4.29 ms        6.29 ms

Comparison: 
Manual             243.86
Then               238.00 - 1.02x slower +0.101 ms
ThenInlined        230.07 - 1.06x slower +0.25 ms

Memory usage statistics:

Name                average  deviation         median         99th %
Manual            469.59 KB     ±0.00%      469.59 KB      469.59 KB
Then              469.59 KB     ±0.00%      469.59 KB      469.59 KB
ThenInlined       469.59 KB     ±0.00%      469.59 KB      469.59 KB

Comparison: 
Manual            469.59 KB
Then              469.59 KB - 1.00x memory usage -0.00003 KB
ThenInlined       469.59 KB - 1.00x memory usage -0.00004 KB

##### With input 100000 #####
Name                  ips        average  deviation         median         99th %
Manual              26.10       38.31 ms     ±3.86%       38.13 ms       43.02 ms
Then                23.85       41.93 ms    ±10.08%       41.86 ms       64.52 ms
ThenInlined         23.48       42.60 ms     ±7.40%       41.88 ms       53.79 ms

Comparison: 
Manual              26.10
Then                23.85 - 1.09x slower +3.62 ms
ThenInlined         23.48 - 1.11x slower +4.28 ms

Memory usage statistics:

Name           Memory usage
Manual              4.58 MB
Then                4.58 MB - 1.00x memory usage +0 MB
ThenInlined         4.58 MB - 1.00x memory usage +0 MB

**All measurements for memory usage were the same**
```


### EMU

```
ERTS_BIN="./OTP-24.2.0-bundle/otp/bin/" ELIXIR_ERL_OPTIONS="-emu_flavor emu" mix run lib/benchmark.exs
Using emu_flavor: emu
Error trying to dermine erlang version enoent
Operating System: Linux
CPU Information: Intel(R) Core(TM) i7-6700HQ CPU @ 2.60GHz
Number of Available Cores: 8
Available memory: 31.18 GB
Elixir 1.13.1
Erlang ok

Benchmark suite executing with the following configuration:
warmup: 2 s
time: 2 s
memory time: 2 s
parallel: 1
inputs: 1, 10, 100, 1000, 10000, 100000
Estimated total run time: 1.80 min

Benchmarking Manual with input 1...
Benchmarking Manual with input 10...
Benchmarking Manual with input 100...
Benchmarking Manual with input 1000...
Benchmarking Manual with input 10000...
Benchmarking Manual with input 100000...
Benchmarking Then with input 1...
Benchmarking Then with input 10...
Benchmarking Then with input 100...
Benchmarking Then with input 1000...
Benchmarking Then with input 10000...
Benchmarking Then with input 100000...
Benchmarking ThenInlined with input 1...
Benchmarking ThenInlined with input 10...
Benchmarking ThenInlined with input 100...
Benchmarking ThenInlined with input 1000...
Benchmarking ThenInlined with input 10000...
Benchmarking ThenInlined with input 100000...

##### With input 1 #####
Name                  ips        average  deviation         median         99th %
ThenInlined       57.75 K       17.32 μs    ±44.37%       15.92 μs       44.22 μs
Manual            56.35 K       17.75 μs    ±47.10%       15.82 μs       47.69 μs
Then              56.03 K       17.85 μs    ±44.63%       15.76 μs       46.81 μs

Comparison: 
ThenInlined       57.75 K
Manual            56.35 K - 1.02x slower +0.43 μs
Then              56.03 K - 1.03x slower +0.53 μs

Memory usage statistics:

Name                average  deviation         median         99th %
ThenInlined        904.00 B     ±0.10%          904 B          904 B
Manual                904 B     ±0.00%          904 B          904 B
Then               904.00 B     ±0.10%          904 B          904 B

Comparison: 
ThenInlined           904 B
Manual                904 B - 1.00x memory usage +0.00379 B
Then               904.00 B - 1.00x memory usage +0.00001 B

##### With input 10 #####
Name                  ips        average  deviation         median         99th %
Manual            45.65 K       21.91 μs    ±36.04%       19.90 μs       54.67 μs
Then              45.54 K       21.96 μs    ±33.57%       19.97 μs       54.82 μs
ThenInlined       45.40 K       22.03 μs    ±34.35%       20.15 μs       54.92 μs

Comparison: 
Manual            45.65 K
Then              45.54 K - 1.00x slower +0.0531 μs
ThenInlined       45.40 K - 1.01x slower +0.117 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Manual              1.30 KB     ±0.07%        1.30 KB        1.30 KB
Then                1.30 KB     ±0.00%        1.30 KB        1.30 KB
ThenInlined         1.30 KB     ±0.00%        1.30 KB        1.30 KB

Comparison: 
Manual              1.30 KB
Then                1.30 KB - 1.00x memory usage +0.00000 KB
ThenInlined         1.30 KB - 1.00x memory usage +0.00000 KB

##### With input 100 #####
Name                  ips        average  deviation         median         99th %
Manual            15.40 K       64.95 μs    ±21.28%       59.58 μs      127.68 μs
Then              15.33 K       65.22 μs    ±23.33%       59.37 μs      131.68 μs
ThenInlined       15.28 K       65.44 μs    ±23.32%       59.50 μs      132.65 μs

Comparison: 
Manual            15.40 K
Then              15.33 K - 1.00x slower +0.27 μs
ThenInlined       15.28 K - 1.01x slower +0.49 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Manual              5.52 KB     ±0.04%        5.52 KB        5.52 KB
Then                5.52 KB     ±0.00%        5.52 KB        5.52 KB
ThenInlined         5.52 KB     ±0.04%        5.52 KB        5.52 KB

Comparison: 
Manual              5.52 KB
Then                5.52 KB - 1.00x memory usage +0.00002 KB
ThenInlined         5.52 KB - 1.00x memory usage -0.00000 KB

##### With input 1000 #####
Name                  ips        average  deviation         median         99th %
Then               2.32 K      430.16 μs    ±12.56%      412.84 μs      646.84 μs
Manual             2.31 K      433.45 μs    ±11.09%      427.69 μs      598.63 μs
ThenInlined        2.29 K      437.36 μs    ±11.51%      429.15 μs      644.56 μs

Comparison: 
Then               2.32 K
Manual             2.31 K - 1.01x slower +3.29 μs
ThenInlined        2.29 K - 1.02x slower +7.20 μs

Memory usage statistics:

Name                average  deviation         median         99th %
Then               47.69 KB     ±0.00%       47.69 KB       47.69 KB
Manual             47.69 KB     ±0.01%       47.69 KB       47.69 KB
ThenInlined        47.69 KB     ±0.01%       47.69 KB       47.69 KB

Comparison: 
Then               47.69 KB
Manual             47.69 KB - 1.00x memory usage -0.00022 KB
ThenInlined        47.69 KB - 1.00x memory usage -0.00006 KB

##### With input 10000 #####
Name                  ips        average  deviation         median         99th %
Then               228.04        4.39 ms     ±8.14%        4.36 ms        5.66 ms
Manual             223.50        4.47 ms    ±10.66%        4.55 ms        5.69 ms
ThenInlined        221.09        4.52 ms     ±9.13%        4.53 ms        5.90 ms

Comparison: 
Then               228.04
Manual             223.50 - 1.02x slower +0.0891 ms
ThenInlined        221.09 - 1.03x slower +0.138 ms

Memory usage statistics:

Name                average  deviation         median         99th %
Then              469.58 KB     ±0.00%      469.59 KB      469.59 KB
Manual            469.59 KB     ±0.00%      469.59 KB      469.59 KB
ThenInlined       469.58 KB     ±0.00%      469.59 KB      469.59 KB

Comparison: 
Then              469.59 KB
Manual            469.59 KB - 1.00x memory usage +0.00118 KB
ThenInlined       469.58 KB - 1.00x memory usage -0.00002 KB

##### With input 100000 #####
Name                  ips        average  deviation         median         99th %
Manual              22.94       43.59 ms     ±4.17%       43.33 ms       49.17 ms
ThenInlined         22.93       43.61 ms     ±3.03%       43.36 ms       46.78 ms
Then                22.60       44.24 ms     ±6.58%       43.74 ms       60.24 ms

Comparison: 
Manual              22.94
ThenInlined         22.93 - 1.00x slower +0.0194 ms
Then                22.60 - 1.01x slower +0.65 ms

Memory usage statistics:

Name                average  deviation         median         99th %
Manual              4.58 MB     ±0.00%        4.58 MB        4.58 MB
ThenInlined         4.58 MB     ±0.00%        4.58 MB        4.58 MB
Then                4.58 MB     ±0.00%        4.58 MB        4.58 MB

Comparison: 
Manual              4.58 MB
ThenInlined         4.58 MB - 1.00x memory usage +0 MB
Then                4.58 MB - 1.00x memory usage -0.00001 MB
```
