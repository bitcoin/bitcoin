# Transactions with nVersion 3

A transaction with its `nVersion` field set to 3 ("V3 transactions") is allowed in mempool and
transaction relay, with additional policies described below.

The goal with V3 is to create a policy that is DoS-resistant and makes fee-bumping more robust by
avoiding specific RBF pinning attacks. Contracting protocols in which transactions are signed by
untrusted counterparties before broadcast time, e.g. the Lightning Network (LN), may benefit from
opting in to these policies.

## V3 Rationale: RBF Pinning Attacks

Since contracting transactions are shared between multiple parties and mempool congestion is
difficult to predict, [RBF policy](./mempool-replacements.md) restrictions may accidentally allow a
malicious party to "pin" a transaction, making it impossible or difficult to replace.

### "Rule 3" Pinning

Imagine that counterparties Alice (honest) and Mallory (malicious) have conflicting transactions A
and B, respectively.  RBF rules require the replacement transaction pay a higher absolute fee than
the aggregate fees paid by all original transactions. This means Mallory may increase the fees
required to replace B by:

1. Adding transaction(s) that descend from B and pay a feerate too low to fee-bump B through CPFP.
   For example, assuming the default descendant size limit is 101KvB and B is 1000vB paying a
feerate of 2sat/vB, adding a 100KvB, 2sat/vB child increases the cost to replace B by 200Ksat.

2. Adding a high-fee descendant of B that also spends from a large, low-feerate mempool transaction,
   C. The child may pay a very large fee but not actually be fee-bumping B if its overall ancestor
feerate is still lower than B's individual feerate. For example, assuming the default ancestor size
limit is 101KvB, B is 1000vB paying 2sat/vB, and C is 99KvB paying 1sat/vB, adding a 1000vB child of
B and C increases the cost to replace B by 101Ksat.

### "Rule 5" Pinning

RBF rules requires that no replacement trigger the removal of more than 100 transactions. This
number includes the descendants of the conflicted mempool transactions. Mallory can make it more
difficult to replace transactions by attaching lots of descendants to them. For example, if Alice
wants to replace 4 transactions and each one has 25 or more descendants, the replacement will be
rejected regardless of its fees.

## Version 3 Rules

All existing standardness rules and policies apply to V3. The following set of additional
rules apply to V3 transactions:

1. A v3 transaction signals replaceability, even if it does not signal BIP125 replaceability. Other
   conditions apply, see [RBF rules](./mempool-replacements.md) and [Package RBF
rules][./packages.md#Package-Replace-By-Fee].

2. Any descendant of an unconfirmed V3 transaction must also be V3.

*Rationale*: Combined with Rule 1, this gives us the property of "inherited signaling" when
descendants of unconfirmed transactions are created. Additionally, checking whether a transaction
signals replaceability this way does not require mempool traversal, and does not change based on
what transactions are mined.

*Note*: A V3 transaction can spend outputs from *confirmed* non-V3 transactions.

*Note*: This rule is enforced during reorgs. Transactions violating this rule are removed from the
mempool.

3. A V3 transaction's unconfirmed ancestors must all be V3.

*Rationale*: Ensure the ancestor feerate rule does not underestimate a to-be-replaced V3 mempool
transaction's incentive compatibility. Imagine the original transaction, A, has a child B and
co-parent C (i.e. B spends from A and C). C also has another child, D. B is one of the original
transactions and thus its ancestor feerate must be lower than the package's. However, this may be an
underestimation because D can bump C without B's help. This is resolved if V3 transactions can only
have V3 ancestors, as then C cannot have another child.

*Note*: This rule is enforced during reorgs. Transactions violating this rule are removed from the
mempool.

4. A V3 transaction cannot have more than 1 unconfirmed descendant.

Also, [CPFP Carve Out](./mempool-limits.md#CPFP-Carve-Out) does not apply to V3 transactions.

*Rationale*: (upper bound) the larger the descendant limit, the more transactions may need to be
replaced. This is a problematic pinning attack, i.e., a malicious counterparty prevents the
transaction from being replaced by adding many descendant transactions that aren't fee-bumping.
See example #1 in ["Rule 3" Pining](#Rule-3-Pinning) section above.

*Rationale*: (lower bound) at least 1 descendant is required to allow CPFP of the presigned
transaction. The contract protocol can create presigned transactions paying 0 fees and 1 output for
attaching a CPFP at broadcast time ("anchor output"). Without package RBF, multiple anchor outputs
would be required to allow each counterparty to fee-bump any presigned transaction. With package
RBF, since the presigned transactions can replace each other, 1 anchor output is sufficient.

5. A V3 transaction that has an unconfirmed V3 ancestor cannot have a sigop-adjusted virtual size
   larger than 1000vB.

*Rationale*: (upper bound) the larger the descendant size limit, the more vbytes may need to be
replaced. With default limits, if the child is e.g. 100,000vB, that might be an additional
100,000sats (at 1sat/vbyte) or more, depending on the feerate. Restricting all children to 4000Wu
(at most 1000vB) bounds the potential fees by at least 1/100.

*Rationale*: (lower bound) the smaller this limit, the fewer UTXOs a child may use to fund this
fee-bump. For example, only allowing the V3 child to have 2 inputs would require L2 protocols to
manage a wallet with high-value UTXOs and make batched fee-bumping impossible. However, as the
fee-bumping child only needs to fund fees (as opposed to payments), just a few UTXOs should suffice.

*Rationale*: With a limit of 4000 weight units, depending on the output types, the child can have
6-15 UTXOs, which should be enough to fund a fee-bump without requiring a carefully-managed UTXO
pool. With this descendant limit, the cost to replace a V3 transaction has much lower variance.

*Rationale*: This makes the rule very easily "tacked on" to existing logic for policy and wallets.
A transaction may be up to 100KvB on its own (`MAX_STANDARD_TX_WEIGHT`) and 101KvB with descendants
(`DEFAULT_DESCENDANT_SIZE_LIMIT_KVB`). If an existing V3 transaction in the mempool is 100KvB, its
descendant cannot be more than 1000vB, even if the policy is 10KvB.

6. A V3 transaction cannot have more than 1 unconfirmed ancestor.

*Rationale*: Prevent the child of an unconfirmed V3 transaction from "bringing in" more unconfirmed
ancestors. See example #2 in ["Rule 3" Pining](#Rule-3-Pinning) section above.

7. An individual V3 transaction is permitted to be below the mempool min relay feerate, assuming it
   is considered within a [package](./packages.md#Package-Feerate) that meets all feerate requirements.

*Rationale*: This allows for contracting protocols that create presigned transactions
with 0 fees and bump them at broadcast time.
