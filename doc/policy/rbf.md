# Replace By Fee

## Current Policy

A transaction conflicts with an in-mempool transaction ("directly conflicting transaction") if they
spend the same inputs. A transaction ("replacement transaction") may replace its directly
conflicting transactions and all of their in-mempool descendants ("original transactions" together)
if the following conditions are met:

* The directly conflicting transactions signal replaceability explicitly.

* The replacement transaction pays an absolute fee of at least the sum paid by the original
  transactions.

* The replacement transaction must also pay for its own bandwidth at or above the rate set by the
node's minimum relay fee setting.  For example, if the minimum relay fee is 1 satoshi/byte and the
replacement transaction is 500 bytes total, then the replacement must pay a fee at least 500
satoshis higher than the sum of the originals.

* The number of original transactions must not exceed 100.

* The ancestor feerate of the replacement transaction must be greater than the ancestor feerate of
  each of the original transactions.

## History

* Opt-in full replace-by-fee (without inherited signaling) honoured in mempool and mining as of
  **v0.12.0** ([PR 6871](https://github.com/bitcoin/bitcoin/pull/6871)).

* [BIP125](https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki) defined based on
  Bitcoin Core implementation.

* RBF enabled by default in the wallet GUI as of **v0.18.1** ([PR #11605](https://github.com/bitcoin/bitcoin/pull/11605)).

* Restriction on replacement transaction's inputs (BIP125 Rule #2) removed in ([PR #23121](https://github.com/bitcoin/bitcoin/pull/23121)).
