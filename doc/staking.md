BitGold Staker
==============

BitGold combines proof-of-work with proof-of-stake. Any node that holds
mature coins may stake to help secure the network and earn rewards. All
blocks—including the first after the genesis block—must include a valid
coinstake transaction.

## Enabling staking

1. Ensure your wallet contains spendable BitGold outputs.
2. Start the node with staking enabled:

   ```
   bitgoldd -staking=1
   ```

   or add the following line to `bitgold.conf`:

   ```
   staking=1
   ```
3. Use `bitgold-cli getstakinginfo` to verify that your node is staking.

The staker will automatically attempt to create new blocks at the protocol's
8-minute target spacing. Staking rewards follow BitGold's 90 000-block halving
schedule.

## Monitoring block production

Use `bitgold-cli getblockheader` or `bitgold-cli getblockchaininfo` to inspect
recent block timestamps. If the interval between consecutive blocks exceeds
roughly eight minutes, check `debug.log` for entries mentioning `kernel` that
indicate failed stake evaluations. Reducing stake difficulty or adding more
mature inputs to the wallet can improve block production.

The helper script `contrib/stake_monitor.py` automates this check:

```
$ python3 contrib/stake_monitor.py
```
