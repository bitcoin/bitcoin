# BitGold Staking

Bitcoin Core includes an experimental proof-of-stake staker that can create blocks from mature wallet coins.

## Enabling

Start the node with `-staker=1` (or add `staker=1` to `bitcoin.conf`) to run the staking thread.

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
