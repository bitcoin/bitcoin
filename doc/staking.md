BitGold Staker
==============

BitGold combines proof-of-work with proof-of-stake. The genesis block and its
immediate successor are mined using proof-of-work. Staking begins
automatically at height 2, and every subsequent block must include a valid
coinstake transaction. The staking engine implements the PoSV3 consensus rules
described below.

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
schedule. The staking thread adheres to PoSV3 without additional options, but
`-debug=staking` can help diagnose failures.

## PoSV3 consensus rules

PoSV3 introduces stricter requirements for proof-of-stake validation:

* Block timestamps must be multiples of 16 seconds.
* Each staked input must be at least 1 BG and must have aged for one hour or
  more before being eligible.
* Blocks contain a zero-value coinbase transaction followed by the coinstake.
* The coinstake output pays back the original amount plus the current block
  subsidy.

These parameters are fixed by consensus and cannot be changed via configuration.

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

## Cold staking

BitGold supports delegating staking rights to an online node while keeping the
spending key offline. To create a cold‑stake address:

1. Generate an owner (spending) address in the offline wallet and a staking
   address on the online node.
2. On either system, call:

   ```
   bitcoin-cli delegatestakeaddress "OWNER_ADDR" "STAKER_ADDR"
   ```

   This returns a P2SH address and redeem script. Send coins to the returned
   address.
3. On the staking node, register the address:

   ```
   bitcoin-cli registercoldstakeaddress "DELEGATE_ADDR" "REDEEM_SCRIPT"
   ```

   The staking thread will now include the delegated coins when attempting to
   create blocks.
4. To spend the staked rewards, use the owner wallet with the original private
   key.
