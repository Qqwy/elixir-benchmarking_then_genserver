defmodule BenchmarkingThenGenserverTest do
  use ExUnit.Case
  doctest BenchmarkingThenGenserver

  test "greets the world" do
    assert BenchmarkingThenGenserver.hello() == :world
  end
end
