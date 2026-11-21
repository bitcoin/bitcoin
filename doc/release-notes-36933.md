P2P and network changes
-----------------------

- With stale-tip relay enabled, the node serves available blocks on eligible
  tracked branches to peers that negotiated BIP 332. The new `-staletips=blocks`
  mode also fetches blocks on offered stale branches and asks peers to
  announce tips once they have block data. (#36933)

- Both `headers` and `blocks` modes try to fetch an offered branch with more
  work than the active tip, even if the peer only has the tip block. Requests
  made solely for stale-tip relay can expire without disconnecting the peer.
  Ordinary block download can request the same block independently. (#36933)
