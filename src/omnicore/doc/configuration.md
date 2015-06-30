Omni Core configuration
=======================

Omni Core can be configured via command-line options, or by editing `bitcoin.conf`.

##### General

 - `-startclean` (boolean, clear persistence files, disabled by default)

##### Logging

 - `-omnilogfile=file` (string, log file, `omnicore.log` by default)
 - `-omnidebug=category` (multi string, enable log categories, can be `all`, `none`)

##### Transactions

 - `-autocommit` (boolean, create or broadcast transactions, enabled by default)
 - `-omnitxcache=n` (number, max. size of input transaction cache, `500000` transactions by default)
 - `-datacarrier` and `-datacarriersize` determine whether to use class B (multisig) or class C encoding

##### Notifications

 - `-omnialertallowsender=source` (multi string, authorize alert senders, can be `any`)
 - `-omnialertignoresender=source` (multi string, ignore alert senders)
 - `-overrideforcedshutdown` (boolean, overwrite shutdown, triggered by an alert, disabled by default)

##### UI

 - `-disclaimer` (boolean, explicitly show QT disclaimer on startup, disabled by default)
 - `-omniuiwalletscope=n` (number, max. transactions to show in history, `500` by default)
