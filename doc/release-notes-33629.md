Mempool
=======

The mempool has been reimplemented with a new design ("cluster mempool"), to
facilitate better decision-making when constructing block templates, evicting
transactions, relaying transactions, and validating replacement transactions
(RBF). Most changes should be transparent to users, but some behavior changes
are noted:

- The mempool no longer enforces ancestor or descendant size/count limits.
  Instead, two new default policy limits are introduced governing connected
  components, or clusters, in the mempool, limiting clusters to 64 transactions
  and up to 101 kB in virtual size.  Transactions are considered to be in the
  same cluster if they are connected to each other via any combination of
  parent/child relationships in the mempool. These limits can be overridden
  using command line arguments; see the extended help (`-help-debug`)
  for more information.

- Within the mempool, transactions are ordered based on the feerate at which
  they are expected to be mined, which takes into account the full set, or
  "chunk", of transactions that would be included together (e.g., a parent and
  its child, or more complicated subsets of transactions). This ordering is
  utilized by the algorithms that implement transaction selection for
  constructing block templates; eviction from the mempool when it is full; and
  transaction relay announcements to peers.

- The replace-by-fee validation logic has been updated so that transaction
  replacements are only accepted if the resulting mempool's feerate diagram is
  strictly better than before the replacement. This eliminates all known cases
  of replacements occurring that make the mempool worse off, which was possible
  under previous RBF rules. For singleton transactions (that are in clusters by
  themselves) it's sufficient for a replacement to have a higher fee and
  feerate than the original. See
  [delvingbitcoin.org post](https://delvingbitcoin.org/t/an-overview-of-the-cluster-mempool-proposal/393#rbf-can-now-be-made-incentive-compatible-for-miners-11)
  for more information.

- Two new RPCs have been added: `getmempoolcluster` will provide the set of
  transactions in the same cluster as the given transaction, along with the
  ordering of those transactions and grouping into chunks; and
  `getmempoolfeeratediagram` will return the feerate diagram of the entire
  mempool.

- Chunk size and chunk fees are now also included in the output of `getmempoolentry`.
