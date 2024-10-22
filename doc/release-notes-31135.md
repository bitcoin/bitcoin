Low-level changes
=================

RPC
---

- When the node is synced, RPCs getblockchaininfo and getchainstates now return
  a "verificationprogress" of 1 and CLI -getinfo returns 100.0000%. The previous
  behavior at the tip was to return a value like 0.9999991055242994, or 99.9999%
  for -getinfo. (#31135)
