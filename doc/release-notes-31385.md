RPC

The `submitpackage` RPC, which allows submissions of child-with-parents
packages, no longer requires that all unconfirmed parents be present. The
package may contain other in-mempool ancestors as well. (#31385)

P2P

Opportunistic 1-parent-1-child package relay has been improved to handle
situations when the child already has unconfirmed parent(s) in the mempool.
This means that 1p1c packages can be accepted and propagate, even if they are
connected to broader topologies: multi-parent-1-child (where only 1 parent
requires fee-bumping), grandparent-parent-child (where only parent requires
fee-bumping) etc. (#31385)
