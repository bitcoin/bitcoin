Logging
-------
Unconditional logging to disk via LogPrintf, LogInfo, LogWarning, LogError, and
LogPrintLevel is now rate limited by giving each source location a logging quota of
1MiB per hour. This affects -logsourcelocation as the entire function signature is
logged rather than only the name.
