## Command-line options
### Changes in existing cmd-line options:

- `-walletnotify=<cmd>` has a new format options "%h" and "%b".
%b is replaced by the hash of the block including the transaction (set to 'unconfirmed' if the transaction is not included).
%h is replaced by the block height (-1 if not included).
