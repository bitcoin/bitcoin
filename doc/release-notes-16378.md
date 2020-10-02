RPC
---
- A new `send` RPC with similar syntax to `walletcreatefundedpsbt`, including
  support for coin selection and a custom fee rate. The `send` RPC is experimental
  and may change in subsequent releases. Using it is encouraged once it's no
  longer experimental: `sendmany` and `sendtoaddress` may be deprecated in a future release.
