RPC and Startup Option
---
The `-paytxfee` startup option and the `settxfee` RPC are now deleted after being deprecated in Bitcoin Core 30.0. They used to allow the user to set a static fee rate for wallet transactions, which could potentially lead to overpaying or underpaying. Users should instead rely on fee estimation or specify a fee rate per transaction using the `fee_rate` argument in RPCs such as `fundrawtransaction`, `sendtoaddress`, `send`, `sendall`, and `sendmany`. (#32138)
