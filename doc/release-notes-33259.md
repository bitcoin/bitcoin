RPC
---

The `getblockchaininfo` RPC now exposes progress for background validation if the `assumeutxo` feature is used. Once a node has synced from snapshot to tip, `verificationprogress` returns 1.0 and `initialblockdownload` false even though the node may still be validating blocks in the background. A new object, `backgroundvalidation`, provides details about the snapshot being validated, including snapshot height, number of blocks processed, best block hash, chainwork, median time, and verification progress.
