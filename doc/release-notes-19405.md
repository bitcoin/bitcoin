## Updated RPCs

- `getnetworkinfo` now returns fields `connections_in`, `connections_out`,
  `connections_mn_in`, `connections_mn_out`, `connections_mn`
  that provide the number of inbound and outbound peer
  connections. These new fields are in addition to the existing `connections`
  field, which returns the total number of peer connections. Old fields
  `inboundconnections`, `outboundconnections`, `inboundmnconnections`,
  `outboundmnconnections` and `mnconnections` are removed (dash#5823)

## CLI

- The `connections` field of `dash-cli -getinfo` is expanded to return a JSON
  object with `in`, `out` and `total` numbers of peer connections and `mn_in`,
  `mn_out` and `mn_total` numbers of verified mn connections. It previously
  returned a single integer value for the total number of peer connections. (dash#5823)
