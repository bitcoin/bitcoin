# Mempool Replacements

## Current Replace-by-Fee Policy

A transaction conflicts with an in-mempool transaction ("directly conflicting transaction") if they
spend one or more of the same inputs. A transaction may conflict with multiple in-mempool
transactions.

A transaction ("replacement transaction") may replace its directly conflicting transactions and
their in-mempool descendants (together, "original transactions") if, in addition to passing all
other consensus and policy rules, each of the following conditions are met:

1. If `-mempoolfullrbf=0` (the value is 1 by default), the directly conflicting transactions all signal replaceability explicitly. A transaction is
   signaling BIP125 replaceability if any of its inputs have an nSequence number less than (0xffffffff - 1).
   A transaction also signals replaceability if its version field is set to 3.

   *Rationale*: See [BIP125
   explanation](https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki#motivation).

2. [REDACTED]

3. The replacement transaction pays an absolute fee of at least the sum paid by the original
   transactions.

   *Rationale*: Only requiring the replacement transaction to have a higher feerate could allow an
   attacker to bypass node minimum relay feerate requirements and cause the network to repeatedly
   relay slightly smaller replacement transactions without adding any more fees. Additionally, if
   any of the original transactions would be included in the next block assembled by an economically
   rational miner, a replacement policy allowing the replacement transaction to decrease the absolute
   fees in the next block would be incentive-incompatible.

4. The additional fees (difference between absolute fee paid by the replacement transaction and the
   sum paid by the original transactions) pays for the replacement transaction's bandwidth at or
   above the rate set by the node's incremental relay feerate. For example, if the incremental relay
   feerate is 1 satoshi/vB and the replacement transaction is 500 virtual bytes total, then the
   replacement pays a fee at least 500 satoshis higher than the sum of the original transactions.

   *Rationale*: Try to prevent DoS attacks where an attacker causes the network to repeatedly relay
   transactions each paying a tiny additional amount in fees, e.g. just 1 satoshi.

5. The number of distinct clusters corresponding to conflicting transactions does not exceed 100.

   *Rationale*: Limit CPU usage required to update the mempool for so many transactions being
   removed at once.

6. The feerate diagram of the mempool must be strictly improved by the replacement transaction.

   *Rationale*: This ensures that block fees in all future blocks will go up
   after the replacement (ignoring tail effects at the end of a block).


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

* Full replace-by-fee is the default policy as of **v28.0** ([PR #30493](https://github.com/bitcoin/bitcoin/pull/30493)).

* Feerate diagram policy enabled in conjunction with switch to cluster mempool as of **v??.0**.
