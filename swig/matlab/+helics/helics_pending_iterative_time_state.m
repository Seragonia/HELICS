function v = helics_pending_iterative_time_state()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1432107612);
  end
  v = vInitialized;
end