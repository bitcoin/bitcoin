Coin selection
--------------

### Reuse Avoidance

A new wallet flag `avoid_reuse` has been added (default off). When enabled,
a wallet will distinguish between used and unused addresses, and default to not
use the former in coin selection.

Rescanning the blockchain is required, to correctly mark previously
used destinations.

Together with "avoid partial spends" (present as of Bitcoin v0.17), this
addresses a serious privacy issue where a malicious user can track spends by
peppering a previously paid to address with near-dust outputs, which would then
be inadvertently included in future payments.

New RPCs
--------

- A new `setwalletflag` RPC sets/unsets flags for an existing wallet.


Updated RPCs
------------

Several RPCs have been updated to include an "avoid_reuse" flag, used to control
whether already used addresses should be left out or included in the operation.
These include:

- createwallet
- getbalance
- getbalances
- sendtoaddress

In addition, `sendtoaddress` has been changed to avoid partial spends when `avoid_reuse`
is enabled (if not already enabled via the  `-avoidpartialspends` command line flag),
as it would otherwise risk using up the "wrong" UTXO for an address reuse case.

The listunspent RPC has also been updated to now include a "reused" bool, for nodes
with "avoid_reuse" enabled.
