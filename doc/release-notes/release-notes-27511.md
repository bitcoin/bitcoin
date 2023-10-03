New RPCs
--------

- A new RPC `getaddrmaninfo` has been added to view the distribution of addresses in the new and tried table of the
  node's address manager across different networks(ipv4, ipv6, onion, i2p, cjdns). The RPC returns count of addresses
  in new and tried table as well as their sum for all networks. (#27511)
