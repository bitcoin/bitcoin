P2P and network changes
-----------------------

- With `-staletips=headers`, the node negotiates stale-tip relay (BIP 332)
  with peers that advertise the BIP 434 `BIP332` feature. It announces the
  recent stale tips it tracks to those peers, tracks the stale tips they
  announce, and requests the block of an announced tip that has more work
  than its active tip. A peer that serves an invalid block on a stale
  branch it announced is not disconnected for it, since BIP 332 lets peers
  serve such blocks without having validated them. (#36920)

Updated RPCs
------------

- `getnetworkinfo` has a new `localfeatures` field and `getpeerinfo` a new
  `features` field, listing the BIP 434 features the node and each peer
  advertise, with their decoded data. (#36920)
