BitGold Staker
==============

BitGold combines proof-of-work with proof-of-stake. Any node that holds
mature coins may stake to help secure the network and earn rewards.

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
