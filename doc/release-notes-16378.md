RPC
---
- A new `send` RPC with similar syntax to `walletcreatefundedpsbt`, including
  support for coin selection and a custom fee rate. Using the new `send` method
  is encouraged: `sendmany` and `sendtoaddress` may be deprecated in a future release.
