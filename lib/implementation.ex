impls = %{
  ThenServer =>
    quote do
      @impl true
      def handle_cast({:add, num}, state) do
        state
        |> Map.put(:result, state.result + num)
        |> Map.put(:n_operations, state.n_operations + 1)
        |> then(&{:noreply, &1})
      end
    end,
  ThenInlinedServer =>
    quote do
      @compile :inline
      @impl true
      def handle_cast({:add, num}, state) do
        state
        |> Map.put(:result, state.result + num)
        |> Map.put(:n_operations, state.n_operations + 1)
        |> then(&{:noreply, &1})
      end
    end,
  ManualServer =>
    quote do
      @impl true
      def handle_cast({:add, num}, state) do
        state =
          state
          |> Map.put(:result, state.result + num)
          |> Map.put(:n_operations, state.n_operations + 1)

        {:noreply, state}
      end
  end,
  SuperManualServer =>
    quote do
    @impl true
    def handle_cast({:add, num}, %{result: result, n_operations: n_operations}) do
      new_state = %{result: result + num, n_operations: n_operations + 1}
      {:noreply, new_state}
    end
  end

}

for {module_name, snippet} <- impls do
  body =
    quote do
      use GenServer

      def start_link, do: GenServer.start_link(__MODULE__, {})
      def add(pid, num), do: GenServer.cast(pid, {:add, num})
      def current_state(pid), do: GenServer.call(pid, :current_state)

      @impl true
      def init(_), do: {:ok, %{result: 0, n_operations: 0}}

      @impl true
      def handle_call(:current_state, _from, state) do
        {:reply, state, state}
      end

      unquote(snippet)
    end

  Module.create(module_name, body, Macro.Env.location(__ENV__))
end
