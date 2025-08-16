# BitGold Staking

Bitcoin Core includes an experimental proof-of-stake staker that can create
blocks from mature wallet coins. The current implementation follows the
PoSV3 (Proof-of-Stake Version 3) rules.

## PoSV3 rules

Blocks created by the staker must obey the following consensus checks:

- Every block contains a zero-value coinbase and a coinstake transaction.
- Block timestamps are masked to 16‑second granularity.
- Each staked input must be at least 1 coin and must have aged for at least
  one hour.
- The staking reward is added to the staked output in the coinstake
  transaction and follows the network’s halving schedule.

These rules are enforced automatically when staking is enabled.

## Enabling

Start the node with `-staker=1` (or add `staker=1` to `bitcoin.conf`) to run
the staking thread. No additional configuration is required for PoSV3, but
`-debug=staking` may be useful for troubleshooting.

## Monitoring

Use the `stakerstatus` RPC to check if staking is enabled and running:

```
$ bitcoin-cli stakerstatus
{
  "enabled": true,
  "staking": true
}
```

The `enabled` field indicates whether the `-staker` option is set, while `staking` shows if the staking thread is currently active.
