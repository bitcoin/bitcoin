Logging
=======

- A new `-trace` option has been added. The only trace logs currently
  supported are detailed logs about wallet sql statements, which can
  be accessed via `-trace=walletdb`. The `-loglevel` configuration option
  has been removed. (PR#34038)
- The `logging` RPC command is now able to switch trace level logs on
  and off, via its third parameter. Since a boolean can no longer
  represent a category's state, the output format is changed to three
  arrays (excluded, debug, trace) showing each category's logging
  level. The `-deprecatedrpc=logging` startup option can currently be
  used to revert to the old format. (PR#34038)
