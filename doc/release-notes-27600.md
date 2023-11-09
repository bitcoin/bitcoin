P2P
---

- A new net permission `forceinbound` (set with `-whitelist=forceinbound@...`
  or `-whitebind=forceinbound@...`) is introduced that extends `noban` permissions.
  Inbound connections from hosts protected by `forceinbound` will now be more
  likely to connect even if `maxconnections` is reached and the node's inbound
  slots are full. This is achieved by attempting to force the eviction of a random,
  inbound, otherwise unprotected peer. Because this permission can be exploited
  to disrupt existing connections, a maximum of 8 forced inbound peers are allowed
  simultaneously. RPC `getpeerinfo` will also now indicate `forced_inbound`. (#27600)