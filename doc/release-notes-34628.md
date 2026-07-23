P2P and network changes
-----------------------

- To reduce memory and CPU usage during periods of high transaction
  volume, rate-limiting of outgoing transaction relay has been changed
  to use a global backlog instead of being done on a per-peer basis. The
  default rate-limit remains as 14 tx/s (boosted by 2.5x for outbound
  peers), though this can be changed via the `-txsendrate` configuration
  option. An additional bandwidth rate-limit has also been introduced
  at 12MB of transactions per 10 minutes, with a high burst rate. The
  size of the global backlog and the token bucket values for the rate
  limits can be queried via the `getnetworkinfo` RPC. (#34628)

