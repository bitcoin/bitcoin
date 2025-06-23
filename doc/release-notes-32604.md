Logging
-------
Unconditional logging to disk is now rate limited by giving each source location
a quota of 1MiB per hour. Unconditional logging is any logging with a log level
higher than debug, that is `info`, `warning`, and `error`. All logs will be
prefixed with `[*]` if there is at least one source location that is currently
being suppressed. (#32604)

When `-logsourcelocations` is enabled, the log output now contains the entire
function signature instead of just the function name. (#32604)
