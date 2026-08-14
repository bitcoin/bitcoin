Updated settings
----------------

- Invalid `-noconnect` values such as `-noconnect=0` (double-negatives that
  previously became `-connect=1` and attempted a connection to `0.0.0.1`)
  now cause startup to fail with an error instead of silently connecting.
  `-noconnect` and `-noconnect=1` still disable automatic outbound
  connections. (#35939)
