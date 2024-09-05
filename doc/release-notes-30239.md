P2P and network changes
-----------------------

Ephemeral dust is a new concept that allows a single
dust output in a transaction, provided the transaction
is zero fee. In order to spend any unconfirmed outputs
from this transaction, the spender must also spend
this dust in addition to any other outputs.

In other words, this type of transaction
should be created in a transaction package where
the dust is both created and spent simultaneously.
