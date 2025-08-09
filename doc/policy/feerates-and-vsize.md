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
consideration of signature operations. To protect against “2D‑Knapsack” attacks that exploit a
combination of block weight and high sigop counts, the mempool applies
a **sigop‑adjusted size**. This value is the maximum of the pure BIP‑141 vsize and a sigop‑based
calculation (sigop count multiplied by a configurable `-bytespersigop` setting, which defaults to
20 vbytes per sigop). The sigop adjustment is used internally for mempool admission, eviction, fee
estimation, and mining template construction.

## RPC Field Exposures

In mempool‑related RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, and `submitpackage`),
the field named `vsize` represents the **sigop‑adjusted size**, reflecting the policy‑adjusted estimate
of block space usage. These same RPCs also provide a field called `vsize_bip141` which exposes the
pure BIP‑141 virtual size without sigop adjustment. An exception to this rule is `getrawtransaction`,
where the reported `vsize` is the BIP‑141 value due to backwards compatibility.

## Guidance for Users

When performing operations involving or affected by mempool policy (e.g fee estimation, eviction, or acceptance),
use the sigop-adjusted size. Doing so should lead to better accuracy assuming most of the network/miners
do the same. Node operators should also be cautious when changing the`-bytespersigop` setting, as it
affects these aspects. However, when building block templates, miners should prioritize transactions
by actual fee per weight unit and be mindful of the 80 000 sigop limit, instead of using the
sigop-adjusted size from the mempool RPCs. For consensus‑level weight calculations, refer instead
to `vsize_bip141`. If you need to inspect the raw BIP‑141 size of a transaction in the mempool, use
`getrawtransaction`; but if you need the policy‑adjusted size, use `getmempoolentry` or `getrawmempool`.
Node operators should be cautious when changing the `-bytespersigop` setting, as it can affect fee
estimation accuracy and block utilization without impacting consensus rules.
