defmodule Benchmark do
  defmacrop code_under_test(module, input) do
    quote do
      {:ok, pid} = unquote(module).start_link()

      for num <- unquote(input) do
        unquote(module).add(pid, num)
      end

      result = unquote(module).current_state(pid)
      GenServer.stop(pid)
      result
    end
  end

  # We run the benchmark on all elements in a list
  # as we're simulating something which will often be run in a tight loop
  def run() do
    IO.puts("Using emu_flavor: #{:erlang.system_info(:emu_flavor)}")
    Benchee.run(
      %{
        "Manual" => fn input -> code_under_test(SuperManualServer, input) end,
        "Manual" => fn input -> code_under_test(ManualServer, input) end,
        "Then" => fn input -> code_under_test(ThenServer, input) end,
        "ThenInlined" => fn input -> code_under_test(ThenInlinedServer, input) end
      },
      time: 2,
      memory_time: 2,
      inputs: %{
        "1" => 1..1,
        "10" => 1..10,
        "100" => 1..100,
        "1000" => 1..1000,
        "10000" => 1..10000,
        "100000" => 1..100_000,
      }
    )
  end
end

Benchmark.run()
