function v = HELICS_DATA_TYPE_COMPLEX()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1936535430);
  end
  v = vInitialized;
end