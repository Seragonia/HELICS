function v = helics_flag_ignore_time_mismatch_warnings()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1936535400);
  end
  v = vInitialized;
end