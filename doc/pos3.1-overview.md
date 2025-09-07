# Proof-of-Stake 3.1 Overview

Proof-of-Stake (PoS) 3.1 refines BitGold's hybrid consensus by improving stake weighting
and simplifying wallet configuration. Blocks may be created by miners or stakers and
all participants are encouraged to keep their nodes online to maximise network security.

## Highlights

- Dynamic stake modifier refreshes periodically to resist grinding attacks.
- Staking rewards scale with coin age up to a configurable maximum.
- Cold staking lets a hot node stake using coins held in an offline wallet.

## Deployment

PoS 3.1 activates at the network height agreed by the community. Nodes should be
upgraded prior to the activation block.

1. Install the latest BitGold release.
2. Review the configuration examples in `doc/examples/`.
3. Restart the node with staking enabled:
   
   ```bash
   bitgoldd -staking=1
   ```
4. Monitor the logs for `stake` messages to verify participation.

Existing stakers do not need to re-create their wallets. The upgrade is
backwards compatible and the node will continue operating after the
activation block.

## Further Reading

- [Staking guide](staking.md)
- [Network parameters](examples/network.conf)
