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

- A new `loglevel` RPC has been added, which provides a superset of
  functionality of the `logging` RPC, and allows enabling `trace` logs as well
  as `debug` logs. Examples:

  ```sh
   bitcoin rpc loglevel                               # See current log levels
   bitcoin rpc loglevel trace                         # Set global log level
   bitcoin rpc loglevel debug                         # Set global log level
   bitcoin rpc loglevel info                          # Set global log level
   bitcoin rpc loglevel libevent=info                 # Set per-category log level
   bitcoin rpc loglevel trace net=debug libevent=info # Set global and category levels
  ```
