Updated settings
----------------

- A new `mining` logging category has been added, enabled with `-debug=mining`.
  The `CreateNewBlock(): block weight: ...` line, previously logged on every
  block template build regardless of logging configuration, is now logged only
  when this category is enabled. `getblocktemplate` and Mining IPC users are
  encouraged to enable it to see this log and future mining logs.
