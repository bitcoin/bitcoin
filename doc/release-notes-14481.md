Low-level RPC changes
----------------------

The `listunspent` RPC has been modified so that it also returns `witnessScript`,
the witness script in the case of a P2WSH or P2SH-P2WSH output.

The `signrawtransactionwithkey` and `signrawtransactionwithwallet` RPCs have been
modified so that they also optionally accept a `witnessScript`, the witness script in the
case of a P2WSH or P2SH-P2WSH output. This is compatible with the change to `listunspent`.
