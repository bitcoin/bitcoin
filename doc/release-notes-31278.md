RPC and Startup Option
---
The `-paytxfee` startup option and the `settxfee` RPC are now deprecated and will be removed in Bitcoin Core 31.0. They allowed the user to set a static fee rate for wallet transactions, which could potentially lead to overpaying or underpaying. Users should instead rely on fee estimation or specify a fee rate per transaction using the `fee_rate` argument in RPCs such as `fundrawtransaction`, `sendtoaddress`, `send`, `sendall`, and `sendmany`. (#31278)
