Logging
-------

- The `-loglevel` option now works standalone without `-debug`. Previously,
  `-loglevel` only set level thresholds but required `-debug` to be separately
  specified to actually enable log categories. This made `-loglevel` harder to
  use than necessary.

  The advantage of the `-loglevel` option is that it supports finer-grained
  control over logging, with three log levels `info`, `debug`, and `trace`
  instead of just the default `info` level and `debug`. It is a superset of
  other logging options, with `-loglevel=debug` being a synonym for `-debug=1`,
  `-loglevel=<category>:debug` being a synonym for `-debug=<category>`, and
  `-loglevel=<category>:info` being a synonym for `-debugexclude=<category>`.
