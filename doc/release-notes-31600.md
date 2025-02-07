Updated RPCs
---
- the `getblocktemplate` RPC `curtime` (BIP22) and `mintime` (BIP23) fields now
  account for the timewarp fix proposed in BIP94 on all networks. This ensures
  that, in the event a timewarp fix softfork activates on mainnet, un-upgraded
  miners will not accidentally violate the timewarp rule. (#31376, #31600)

As a reminder, it's important that any software which uses the `getblocktemplate`
RPC takes these values into account (either `curtime` or `mintime` is fine).
Relying only on a clock can lead to invalid blocks under some circumstances,
especially once a timewarp fix is deployed.
