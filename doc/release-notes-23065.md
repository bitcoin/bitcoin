Notable changes
===============

Updated RPCs
------------

- `lockunspent` now optionally takes a third parameter, `persistent`, which
causes the lock to be written persistently to the wallet database. This
allows UTXOs to remain locked even after node restarts or crashes.

GUI changes
-----------

- UTXOs which are locked via the GUI are now stored persistently in the
wallet database, so are not lost on node shutdown or crash.