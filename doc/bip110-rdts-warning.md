# Warning: BIP-110 (Reduced Data Temporary Softfork) and Chain Reorganization Risk

## Summary

[BIP-110](https://github.com/bitcoin/bips/blob/master/bip-0110.mediawiki), also known as the Reduced Data Temporary Softfork (RDTS), is an active User-Activated Soft Fork (UASF) with a mandatory signaling period beginning at approximately **block 961,632 (~August 7, 2026)** and a mandatory activation deadline of approximately **block 965,664 (~September 1, 2026)**. Bitcoin Core has not implemented enforcement of BIP-110's consensus rules, and has not taken an explicit position on the resulting chain. Users running Bitcoin Core nodes should be aware of the risks this creates.

## Background

BIP-110 proposes to temporarily limit the size of data fields at the consensus level in order to restrict non-monetary data storage in Bitcoin blocks. It uses a modified BIP9 UASF activation mechanism with the following parameters:

- **Signaling bit:** 4
- **Activation threshold:** 55% of blocks over a 2016-block period
- **Mandatory signaling period:** blocks 961,632–963,647 (~August 7–21, 2026) — BIP-110-enforcing nodes reject any block during this window that does not signal bit 4
- **Lock-in:** block 963,648
- **Mandatory activation deadline:** block 965,664 (~September 1, 2026)
- **Active duration:** 52,416 blocks (~1 year), after which the rules expire

BIP-110 began signaling on the network in early 2026. As of this writing, [Bitcoin Knots](https://github.com/bitcoinknots/bitcoin) — whose latest release (v29.3.knots20260508) includes BIP-110 enforcement — represents a meaningful and growing share of the reachable node network.

## Bitcoin Core's Position

Two pull requests were submitted to Bitcoin Core to implement BIP-110 enforcement:

- [PR #34929](https://github.com/bitcoin/bitcoin/pull/34929): "Versionbits extensions for BIP-110" — closed March 26, 2026
- [PR #34930](https://github.com/bitcoin/bitcoin/pull/34930): "Reduced Data Temporary Softfork (BIP-110/RDTS)" — closed March 26, 2026

Both were closed without being merged. Bitcoin Core has also not implemented a User-Rejected Soft Fork (URSF) mechanism to explicitly and unambiguously commit to the non-BIP-110 chain.

As a result, Bitcoin Core currently takes no active position on BIP-110: it neither enforces the new rules nor explicitly rejects them.

## Risk to Bitcoin Core Users

When a UASF activates, nodes that enforce the new rules will reject blocks that violate them. If BIP-110 activates — either through early threshold signaling or at its mandatory deadline — the following scenario becomes possible:

1. BIP-110-enforcing nodes reject any block that violates RDTS rules. They follow only the BIP-110-valid chain.
2. Miners producing blocks that are valid under pre-BIP-110 rules but invalid under BIP-110 create a chain that BIP-110 nodes will not follow.
3. Bitcoin Core nodes, which do not enforce BIP-110 rules, may follow whichever chain has the most cumulative proof of work — including a chain that BIP-110 nodes consider invalid.
4. If the chain followed by Bitcoin Core later has less cumulative proof of work than the BIP-110-valid chain, a **reorganization** will occur. Transactions that appeared confirmed on the Core chain may be reversed.

This risk is not hypothetical: it is the standard mechanism by which a UASF can cause a chain split, and it has precedent in Bitcoin's history (e.g., the 2017 SegWit UASF period).

The absence of either BIP-110 enforcement *or* an explicit URSF in Bitcoin Core means that Core nodes are in an indeterminate state relative to the BIP-110 fork: they will simply follow the chain with the most proof of work, regardless of which consensus rules it follows. Users relying on Bitcoin Core for transaction finality — including exchanges, payment processors, and custodians — should be aware that confirmed transactions may be subject to reversal if a reorg occurs across a chain split boundary.

**The mandatory signaling period beginning at block 961,632 (~August 7, 2026) represents a critical threshold.** From that point, BIP-110-enforcing nodes will actively reject non-signaling blocks, making chain divergence increasingly likely if miner support is insufficient.

## Options

Node operators running Bitcoin Core should consider the following:

1. **Switch to a BIP-110-enforcing client** — [Bitcoin Knots](https://github.com/bitcoinknots/bitcoin) (latest release: v29.3.knots20260508) includes BIP-110 enforcement and will follow the BIP-110-valid chain.
2. **Monitor the situation closely** as the mandatory signaling period approaches and assess which chain is likely to have the most cumulative proof of work.
3. **Increase confirmation thresholds** for high-value transactions during the period surrounding activation.

## Recommendation

A minor release of Bitcoin Core that explicitly documents this situation and provides users with the information needed to make an informed decision would be appropriate. Regardless, at minimum it should be clearly communicated that:

- That PRs #34929 and #34930 were declined and BIP-110 enforcement will not be included in Core
- That no URSF has been implemented to explicitly commit to the non-BIP-110 chain
- The practical implications for transaction finality during and after BIP-110 activation
- What steps users can take to protect themselves

## References

- [BIP-110 specification](https://github.com/bitcoin/bips/blob/master/bip-0110.mediawiki)
- [BIP-110 website](https://bip110.org/)
- [PR #34929: Versionbits extensions for BIP-110](https://github.com/bitcoin/bitcoin/pull/34929)
- [PR #34930: Reduced Data Temporary Softfork (BIP-110/RDTS)](https://github.com/bitcoin/bitcoin/pull/34930)
- [Bitcoin Knots (includes BIP-110 enforcement)](https://github.com/bitcoinknots/bitcoin)
- [BIP-110 node signaling monitor](https://bip110monitor.com/)
