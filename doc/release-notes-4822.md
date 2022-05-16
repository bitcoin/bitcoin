Miscellaneous RPC Changes
-------------------------
- In rpc `upgradetohd` new parameter `rescan` was added which allows users to skip or force blockchain rescan. This params defaults to `false` when `mnemonic` parameter is empty and `true` otherwise.
