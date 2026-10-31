Logging
-------

- The `logging` RPC no longer restores log levels previously assigned via
  `-loglevel` when re-enabling categories. It now consistently toggles categories
  between `info` and `debug` levels. This only affects setups that combine
  `logging` RPC calls with `-loglevel` configuration. When used independently,
  both `-loglevel` and `logging` behave as before.
