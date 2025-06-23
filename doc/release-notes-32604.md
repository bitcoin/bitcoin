Logging
-------
Unconditional logging to disk via LogPrintf, LogInfo, LogWarning, LogError, and
LogPrintLevel is now rate limited by giving each source location a logging quota of
1MiB per hour. (#32604)

When `-logsourcelocation` is enabled, the log output now contains the entire
function signature instead of just the function name. (#32604)
