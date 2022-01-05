defmodule Benchmark do
  defmacrop code_under_test(module, input) do
    quote do
      {:ok, pid} = unquote(module).start_link()

      for num <- unquote(input) do
        unquote(module).add(pid, num)
      end

      unquote(module).current_state(pid)
    end
  end


  # We run the benchmark on all elements in a list
  # as we're simulating something which will often be run in a tight loop
  def run() do
    Benchee.run(
      %{
        "Manual" => fn input -> code_under_test(ManualServer, input) end,
        "Then" => fn input -> code_under_test(ThenServer, input) end,
        "ThenInlined" => fn input -> code_under_test(ThenInlinedServer, input) end,

      },
      time: 10,
      memory_time: 5,
      inputs: %{
        "100" => 1..100,
        # "1000" => 1..1000,
      }
    )
  end

end

Benchmark.run()
