# Policy: Fee Rates and Virtual Size

Bitcoin Core needs a reliable method to estimate how much “block space” each transaction will
consume when building block templates and enforcing mempool policies. While BIP‑141 defines
the concept of _virtual transaction size_ (vsize) based strictly on block weight, the mempool
policy introduces a _sigop adjustment_ to account for potential
denial‑of‑service vectors from transactions with unusually high signature‑operation counts. This
document explains the definitions, how they are calculated, how they are exposed through RPC, and
provides guidance for users and integrators.

## Weight, Virtual Size, and Sigop‑Adjusted Size

**Transaction weight** is defined in BIP‑141 as the sum of four times the stripped transaction size
plus the witness datasize. This weight metric is what consensus rules use to enforce the maximum
block weight of 4 000 000 units.
**BIP‑141 virtual size** (sometimes referred to as `vsize_bip141`) is simply the block weight divided
by four, rounded up. This value represents the consensus‑level “size” of a transaction without any
consideration of signature operations. To account for transactions with unusually high signature-operation counts,
the mempool applies a sigop-adjusted size. This is calculated as the maximum of the pure BIP-141 vsize
and a sigop-based calculation (sigop count multiplied by the configurable `-bytespersigop` setting,
defaulting to 20 vbytes per sigop). Using a single combined metric like this simplifies block template
construction by avoiding the need to optimize for both weight and sigops separately — a potential
“two-dimensional knapsack” problem — though in practice the impact of that problem is often limited.
Some miners prefer to handle weight and sigops as separate constraints, while others use the sigop-adjusted
size for simplicity.

## RPC Field Exposures

In mempool‑related RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, and `submitpackage`),
the field named `vsize` represents the **sigop‑adjusted size**, reflecting the policy‑adjusted estimate
of block space usage. These same RPCs also provide a field called `vsize_bip141` which exposes the
pure BIP‑141 virtual size without sigop adjustment. Additionally, `getrawtransaction` now includes
a `sigopsize` field and continues to report the unadjusted BIP-141 virtual size in its `vsize` field.

## Guidance for Users

When performing operations involving or affected by mempool policy (e.g. fee estimation, eviction, or acceptance),
use the sigop-adjusted size. Doing so should lead to better accuracy assuming most of the network/miners
do the same. Node operators should also be cautious when changing the `-bytespersigop` setting, as it
affects these aspects.

For block template construction, miners have two main approaches:
1. **Single-metric heuristic:** Use the sigop-adjusted size to rank transactions by fee rate.
   This simplifies selection by reducing the two-dimensional (weight + sigops) problem to a one-dimensional metric.
2. **Separate constraints:** Rank transactions by actual fee-per-weight and enforce the 80 000 sigop limit separately.
   This can yield slightly higher block fee revenue but requires more complex selection logic.
