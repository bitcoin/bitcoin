New settings
------------

- Until now, the `debug.log` file grows without bound until the node
  is restarted, at which time it is trimmed to 10 MB. Beginning with this
  release, log rotation is enabled by default. When the `debug.log` file
  reaches 1 MB, it's renamed to a name that includes the current date and
  time, such as `debug-2021-06-27T21:19:42Z.log` and `debug.log` is
  recreated as an empty file. Running `cat` or `grep` with an argument
  `debug*.log` accesses all debug log files, including the current one
  (`debug.log`) in time-order. The `-debuglogrotatekeep=n` configuration
  option specifies the number of rotated debug log files that are retained
  (default 10). If this value is given as zero, log rotation is disabled
  (the debug log file is managed as in prior releases). The `-debugloglimit=n`
  configuration option specifies how large the `debug.log` file must
  become (in MB) before it is rotated (default is 1). If you specify an
  alternate debug log file name using `-debuglogfile=path`, the last
  component of the path must be `debug.log` or else log rotation won't be
  enabled. (#22350)
