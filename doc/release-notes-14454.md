Low-level RPC changes
----------------------

The `importmulti` RPC has been updated to support P2WSH, P2WPKH, P2SH-P2WPKH,
P2SH-P2WSH. Each request now accepts an additional `witnessscript` to be used
for P2WSH or P2SH-P2WSH.
