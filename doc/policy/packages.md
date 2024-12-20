# Package Mempool Accept

## Definitions

A **package** is an ordered list of transactions, representable by a connected Directed Acyclic
Graph (a directed edge exists between a transaction that spends the output of another transaction).

For every transaction `t` in a **topologically sorted** package, if any of its parents are present
in the package, they appear somewhere in the list before `t`.

A **child-with-unconfirmed-parents** package is a topologically sorted package that consists of
exactly one child and all of its unconfirmed parents (no other transactions may be present).
The last transaction in the package is the child, and its package can be canonically defined based
on the current state: each of its inputs must be available in the UTXO set as of the current chain
tip or some preceding transaction in the package.

## Package Mempool Acceptance Rules

The following rules are enforced for all packages:

* Packages cannot exceed `MAX_PACKAGE_COUNT=25` count and `MAX_PACKAGE_SIZE=101KvB` total size
   (#20833)

   - *Rationale*: This is already enforced as mempool ancestor/descendant limits. If
     transactions in a package are all related, exceeding this limit would mean that the package
     can either be split up or it wouldn't pass individual mempool policy.

   - Note that, if these mempool limits change, package limits should be reconsidered. Users may
     also configure their mempool limits differently.

* Packages must be topologically sorted. (#20833)

* Packages cannot have conflicting transactions, i.e. no two transactions in a package can spend
   the same inputs. Packages cannot have duplicate transactions. (#20833)

* No transaction in a package can conflict with a mempool transaction.

* When packages are evaluated against ancestor/descendant limits, the union of all transactions'
  descendants and ancestors is considered. (#21800)

   - *Rationale*: This is essentially a "worst case" heuristic intended for packages that are
     heavily connected, i.e. some transaction in the package is the ancestor or descendant of all
     the other transactions.

The following rules are only enforced for packages to be submitted to the mempool (not enforced for
test accepts):

* Packages must be child-with-unconfirmed-parents packages. This also means packages must contain at
  least 2 transactions. (#22674)

* Transactions in the package that have the same txid as another transaction already in the mempool
  will be removed from the package prior to submission ("deduplication").

   - *Rationale*: Node operators are free to set their mempool policies however they please, nodes
     may receive transactions in different orders, and malicious counterparties may try to take
     advantage of policy differences to pin or delay propagation of transactions. As such, it's
     possible for some package transaction(s) to already be in the mempool, and there is no need to
     repeat validation for those transactions or double-count them in fees.

   - *Rationale*: We want to prevent potential censorship vectors. We should not reject entire
     packages because we already have one of the transactions. Also, if an attacker first broadcasts
     a competing package, the honest package should still be considered for acceptance.
