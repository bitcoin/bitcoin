# JSON-RPC Interface

The headless daemon `dashd` has the JSON-RPC API enabled by default, the GUI
`dash-qt` has it disabled by default. This can be changed with the `-server`
option. In the GUI it is possible to execute RPC methods in the Debug Console
Dialog.

## RPC consistency guarantees

State that can be queried via RPCs is guaranteed to be at least up-to-date with
the chain state immediately prior to the call's execution. However, the state
returned by RPCs that reflect the mempool may not be up-to-date with the
current mempool state.

### Transaction Pool

The mempool state returned via an RPC is consistent with itself and with the
chain state at the time of the call. Thus, the mempool state only encompasses
transactions that are considered mine-able by the node at the time of the RPC.

The mempool state returned via an RPC reflects all effects of mempool and chain
state related RPCs that returned prior to this call.

### Wallet

The wallet state returned via an RPC is consistent with itself and with the
chain state at the time of the call.

Wallet RPCs will return the latest chain state consistent with prior non-wallet
RPCs. The effects of all blocks (and transactions in blocks) at the time of the
call is reflected in the state of all wallet transactions. For example, if a
block contains transactions that conflicted with mempool transactions, the
wallet would reflect the removal of these mempool transactions in the state.

However, the wallet may not be up-to-date with the current state of the mempool
or the state of the mempool by an RPC that returned before this RPC.

## Limitations

There is a known issue in the JSON-RPC interface that can cause a node to crash if
too many http connections are being opened at the same time because the system runs
out of available file descriptors. To prevent this from happening you might
want to increase the number of maximum allowed file descriptors in your system
and try to prevent opening too many connections to your JSON-RPC interface at the
same time if this is under your control. It is hard to give general advice
since this depends on your system but if you make several hundred requests at
once you are definitely at risk of encountering this issue.
