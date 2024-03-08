# Mempool Replacements

## Current Replace-by-Fee Policy

A transaction conflicts with an in-mempool transaction ("directly conflicting transaction") if they
spend one or more of the same inputs. A transaction may conflict with multiple in-mempool
transactions.

A transaction ("replacement transaction") may replace its directly conflicting transactions and
their in-mempool descendants (together, "original transactions") if, in addition to passing all
other consensus and policy rules, each of the following conditions are met:

1. The directly conflicting transactions all signal replaceability explicitly. A transaction is
   signaling BIP125 replaceability if any of its inputs have an nSequence number less than (0xffffffff - 1).
   A transaction also signals replaceability if its nVersion field is set to 3.

   *Rationale*: See [BIP125
   explanation](https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki#motivation).
   Use the (`-mempoolfullrbf`) configuration option to allow transaction replacement without enforcement of the
   opt-in signaling rule.

2. The replacement transaction pays an absolute fee of at least the sum paid by the original
   transactions.

   *Rationale*: Only requiring the replacement transaction to have a higher feerate could allow an
   attacker to bypass node minimum relay feerate requirements and cause the network to repeatedly
   relay slightly smaller replacement transactions without adding any more fees. Additionally, if
   any of the original transactions would be included in the next block assembled by an economically
   rational miner, a replacement policy allowing the replacement transaction to decrease the absolute
   fees in the next block would be incentive-incompatible.

3. The additional fees (difference between absolute fee paid by the replacement transaction and the
   sum paid by the original transactions) pays for the replacement transaction's bandwidth at or
   above the rate set by the node's incremental relay feerate. For example, if the incremental relay
   feerate is 1 satoshi/vB and the replacement transaction is 500 virtual bytes total, then the
   replacement pays a fee at least 500 satoshis higher than the sum of the original transactions.

   *Rationale*: Try to prevent DoS attacks where an attacker causes the network to repeatedly relay
   transactions each paying a tiny additional amount in fees, e.g. just 1 satoshi.

4. The number of conflicting transactions does not exceed 100. This only counts
   direct conflicts, and not their descendants.

   *Rationale*: This bound the number of clusters that might need to be
   relinearized due to accepting a single replacement transaction.

5. The mempool's feerate diagram must improve as a result of accepting the
   replacement. That is, at every size, the amount of total fee that would be
   available if a block of that size were to be mined from the mempool, must
   be the same or greater if we accept the replacement (and there must be at
   least one size at which the new mempool has strictly more fee than before).
   Note: to ignore the tail effects of packing a block, we treat the mempool
   chunks as having a single feerate that is smoothed over the full size of
   the chunk.

   *Rationale*: This is a conservative way to ensure that we never accept a
   transaction that is somehow worse for miners than what was available before.

## History

* Opt-in full replace-by-fee (without inherited signaling) honoured in mempool and mining as of
  **v0.12.0** ([PR 6871](https://github.com/bitcoin/bitcoin/pull/6871)).

* [BIP125](https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki) defined based on
  Bitcoin Core implementation.

* The incremental relay feerate used to calculate the required additional fees is distinct from
  `-minrelaytxfee` and configurable using `-incrementalrelayfee`
  ([PR #9380](https://github.com/bitcoin/bitcoin/pull/9380)).

* RBF enabled by default in the wallet GUI as of **v0.18.1** ([PR
  #11605](https://github.com/bitcoin/bitcoin/pull/11605)).

* Full replace-by-fee enabled as a configurable mempool policy as of **v24.0** ([PR
  #25353](https://github.com/bitcoin/bitcoin/pull/25353)).
