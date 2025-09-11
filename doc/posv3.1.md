# Proof-of-Stake v3.1

PoS v3.1 refines BitGold's hybrid consensus and is active on mainnet once the
network reaches the community-defined activation height. The upgrade adjusts the
stake modifier schedule and tightens validation around stake age and block
spacing.

## Consensus rules and activation

- **Activation** – Nodes begin enforcing PoS v3.1 at the agreed block height
  (set in `chainparams.cpp`). Prior releases continue to follow legacy rules
  but fall out of consensus after activation.
- **Timestamp granularity** – Block timestamps must be multiples of 16 seconds.
- **Stake eligibility** – Inputs must contain at least 1&nbsp;BG and mature for one
  hour before they can stake.
- **Block structure** – Each block contains a zero‑value coinbase followed by the
  coinstake transaction, which pays the input amount plus subsidy.

See the [staking guide](staking.md) for the broader PoS v3 rules and operational
considerations.

## Wallet staking setup

1. Fund the wallet with spendable outputs.
2. Start the node with staking enabled:

   ```bash
   bitgoldd -staking=1
   ```

   Alternatively add `staking=1` to `bitgold.conf`.
3. Confirm activity with the RPC interface:

   ```bash
   bitgold-cli getstakinginfo
   bitgold-cli getwalletinfo
   ```

   The `getstakinginfo` result shows whether the wallet is currently trying to
   stake and reports the expected time to find a block.

For advanced scenarios such as cold staking, refer to [staking.md](staking.md)
 and the [PoS 3.1 overview](pos3.1-overview.md).

## Testing on regtest

A minimal regtest configuration for exercising PoS v3.1 looks like:

```
regtest=1
staking=1
server=1
rpcuser=user
rpcpassword=pass
```

With the above in `bitgold.conf`, run:

```bash
bitgoldd -conf=bitgold.conf
bitgold-cli -regtest getstakinginfo
```

The node will immediately attempt to create blocks when it holds eligible
stake. Additional test scripts are documented in
[doc/pos_test_guide.md](pos_test_guide.md).

## Further reading

- [Staking guide](staking.md)
- [PoS 3.1 overview](pos3.1-overview.md)
- [PoS testing guide](pos_test_guide.md)

