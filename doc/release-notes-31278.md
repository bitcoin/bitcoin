Updated RPCs
---
- The `settxfee` RPC is now **deprecated** and will be removed in Bitcoin Core 30.0. This RPC allowed users to set a static fee rate for wallet transactions, but in a dynamic fee environment, this can lead to overpaying or underpaying. Users should instead rely on fee estimation or specify a fee rate per transaction. (#31278)