# Notable changes

## Asset Unlock transactions (platform transfer)

This version introduces a new fork `withdrawals` that changes consensus rules.
New logic of validation of Asset Unlock transactions's signature. It let to use all 24 active quorums and the most recent inactive, while previous version of Dash Core may refuse withdrawal with error `bad-assetunlock-not-active-quorum` even if quorum is active.

Limits for withdrawals has been increased to flat 2000 Dash per 576 latest blocks.
