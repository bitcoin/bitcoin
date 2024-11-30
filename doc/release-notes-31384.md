- Node and Mining

---

- PR #31384 fixed an issue where duplicate coinbase transaction weight reservation occurred. The `getblocktemplate` RPC will now correctly creates blocks with a weight of `3,996,000` weight units, reserving `4,000` weight units for the coinbase transaction.

- Bitcoin Core will now **fail to start** if users set the `-blockmaxweight` parameter above the maximum consensus block weight of `4,000,000` weight units.
