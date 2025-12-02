# Mempool design and limits

## Definitions

We view the unconfirmed transactions in the mempool as a directed graph,
with an edge from transaction B to transaction A if B spends an output created
by A (i.e., B is a **child** of A, and A is a **parent** of B).

A transaction's **ancestors** include, recursively, its parents, the parents of
its parents, etc.  A transaction's **descendants** include, recursively, its
children, the children of its children, etc.

A **cluster** is a connected component of the graph, i.e., a set of
transactions where each transaction is reachable from any other transaction in
the set by following edges in either direction. The cluster corresponding to a
given transaction consists of that transaction, its ancestors and descendants,
and the ancestors and descendants of those transactions, and so on.

Each cluster is **linearized**, or sorted, in a topologically valid order (i.e.,
no transaction appears before any of its ancestors). Our goal is to construct a
linearization where the highest feerate subset of a cluster appears first,
followed by the next highest feerate subset of the remaining transactions, and
so on[1]. We call these subsets **chunks**, and the chunks of a linearization
have the property that they are always in monotonically decreasing feerate
order.

Given two or more linearized clusters, we can construct a linearization of the
union by simply merge sorting the chunks of each cluster by feerate.

For any set of linearized clusters, then, we can define the **feerate diagram**
of the set by plotting the cumulative fee (y-axis) against the cumulative size
(x-axis) as we progress from chunk to chunk. Given two linearizations for the
same set of transactions, we can compare their feerate diagrams by
comparing their cumulative fees at each size value. Two diagrams may be
**incomparable** if neither contains the other (i.e., there exist size values at
which each one has a greater cumulative fee than the other). Or, they may be
**equivalent** if they have identical cumulative fees at every size value; or
one may be **strictly better** than the other if they are comparable and there
exists at least one size value for which the cumulative fee is strictly higher
in one of them.

For more background and rationale, see [2] and [3] below.

## Mining/eviction

As described above, the linearization of each cluster gives us a linearization
of the entire mempool. We use this ordering for both block building and
eviction, by selecting chunks at the front of the linearization when
constructing a block template, and by evicting chunks from the back of the
linearization when we need to free up space in the mempool.

## Replace-by-fee

Prior to the cluster mempool implementation, it was possible for replacements
to be prevented even if they would make the mempool more profitable for miners,
and it was possible for replacements to be permitted even if the newly accepted
transaction was less desirable to miners than the transactions it was
replacing. With the ability to construct linearizations of the mempool, we're
now able to compare the feerate diagram of the mempool before and after a
proposed replacement, and only accept the replacement if it makes the feerate
diagram strictly better.

In simple cases, the intuition is that a replacement should have a higher
feerate and fee than the transaction(s) it replaces. But for more complex cases
(where some transactions may have unconfirmed parents), there may not be a
simple way to describe the fee that is needed to successfully replace a set of
transactions, other than to say that the overall feerate diagram of the
resulting mempool must improve somewhere and not be worse anywhere.

## Mempool limits

### Motivation

Selecting chunks in decreasing feerate order when building a block template
will be close to optimal when the maximum size of any chunk is small compared
to the block size. And for mempool eviction, we don't wish to evict too much of
the mempool at once when a single (potentially small) transaction arrives that
takes us over our mempool size limit. For both of these reasons, it's desirable
to limit the maximum size of a cluster and thereby limit the maximum size of
any chunk (as a cluster may consist entirely of one chunk).

The computation required to linearize a transaction grows (in polynomial time)
with the number of transactions in a cluster, so limiting the number of
transactions in a cluster is necessary to ensure that we're able to find good
(ideally, optimal) linearizations in a reasonable amount of time.

### Limits

Transactions submitted to the mempool must not result in clusters that would
exceed the cluster limits (64 transactions and 101 kvB total per cluster).

## References/Notes
[1] This is an instance of the maximal-ratio closure problem, which is closely
related to the maximal-weight closure problem, as found in the field of mineral
extraction for open pit mining.

[2] See
https://delvingbitcoin.org/t/an-overview-of-the-cluster-mempool-proposal/393
for a high level overview of the cluster mempool implementation (PR#33629,
since v31.0) and its design rationale.

[3] See https://delvingbitcoin.org/t/mempool-incentive-compatibility/553 for an
explanation of why and how we use feerate diagrams for mining, eviction, and
evaluating transaction replacements.
