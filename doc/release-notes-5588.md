Block Reward Reallocation
-------------------------

Once Masternode Reward Location Reallocation activates:

- Treasury is bumped to 20% of block subsidy.
- Block reward shares are immediately set to 60% for MN and 20% miners.

Note: Previous reallocation periods are dropped.

Updated RPCs
------------

- `getgovernanceinfo` RPC returns the field `governancebudget`: the governance budget for the next superblock.
