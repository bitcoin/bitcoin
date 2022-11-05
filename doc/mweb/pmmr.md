# Prunable Merkle Mountain Ranges (PMMRs)

Merkle Mountain Ranges (MMRs), an invention of Peter Todd for use in [Open Timestamps](https://github.com/opentimestamps/opentimestamps-server/blob/master/doc/merkle-mountain-range.md), are append-only binary trees that can be used to commit to a collection of elements. In addition to supporting efficient insertion, they also come in a prunable form,called Prunable Merkle Mountain Ranges (PMMRs), which support pruning of entire subtrees. They are used in Litecoin\'s MWEB to build TXO commitments.

## Example 1

```
Height 4:                                       30
                                               / \
                                              /   \
                                             /     \
                                            /       \
                                           /         \
                                          /           \
                                         /             \
                                        /               \
                                       /                 \
                                      /                   \
                                     /                     \
                                    /                       \
                                   /                         \
                                  /                           \
                                 /                             \
                                /                               \
                               /                                 \
                              /                                   \
                             /                                     \
Height 3:                   14                                      29                                      45
                           / \                                     / \                                     / \
                          /   \                                   /   \                                   /   \
                         /     \                                 /     \                                 /     \
                        /       \                               /       \                               /       \
                       /         \                             /         \                             /         \
                      /           \                           /           \                           /           \
                     /             \                         /             \                         /             \
                    /               \                       /               \                       /               \
                   /                 \                     /                 \                     /                 \ 
Height 2:         06                 13                   21                 28                   37                 44                   52
                 / \                 / \                 / \                 / \                 / \                 / \                 / \
                /   \               /   \               /   \               /   \               /   \               /   \               /   \
               /     \             /     \             /     \             /     \             /     \             /     \             /     \
              /       \           /       \           /       \           /       \           /       \           /       \           /       \
Height 1:    02       05         09       12         17       20         24       27         33       36         40       43         48       51         55
            / \       / \       / \       / \       / \       / \       / \       / \       / \       / \       / \       / \       / \       / \       / \
           /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \     /   \ 
Height 0: 00   01   03   04   07   08   10   11   15   16   18   19   22   23   25   26   31   32   34   35   38   39   41   42   46   47   49   50   53   54   56
          ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||   ||
Leaves:   00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30

Peaks:    30, 45, 52, 55, 56
```

## Glossary

* Leaf - An item to commit to
* Leaf Index - 0-based insertion position of leaves, represented as a 64-bit unsigned integer
* Node - A leaf or the parent of leaves or nodes
* Position - 0-based index of nodes, represented as a 64-bit unsigned integer
* Size - The number of nodes in an unpruned MMR. In our example with 31 leaves (L0-L30), there are 57 nodes (N0-N56)
* Sibling - The other leaf or node that shares the same parent (e.g. N14 & N29)
* Prune - When a leaf is spent, it shall be marked as pruned. A parent node whose children are both pruned shall also be marked as pruned.
* Compact - To remove or "forget" a node
* Peak - A node with no parent
* HASH(x || y) - The blake3 hash of `x` concatenated with `y`

## Insertion

To append a new leaf `L` to an MMR, add a node at height 0 to the right of the last leaf. If the leaf to the left of it has no parent, add a parent node to the 2 leaves. If the node to the left of the newly-added parent node also has no parent, add a parent node for them. Repeat until the last node added does not have a parent-less sibling.

Let\'s walk through an example. Start with an MMR with 3 leaves (L00, L01, and L02):

```
   N02
   / \
  /   \ 
N00   N01  N03
 |     |    |
L00   L01  L02
```

To add a 4th leaf (L03), append a node (N04) to right of the last leaf node (N03):

```
   N02
   / \
  /   \ 
N00   N01  N03   N04
 |     |    |     |
L00   L01  L02   L03
```

The node to the left (N03) of our added node (N04) does not have a parent. So let\'s give them a parent node (N05).

```
   N02        N05
   / \        / \
  /   \      /   \ 
N00   N01  N03   N04
 |     |    |     |
L00   L01  L02   L03
```

The node to the left (N02) of our added parent node (N05) also does not have a parent. So let\'s add another parent node (N06).

```
         N06
        /  \
       /    \
      /      \
     /        \
   N02        N05
   / \        / \
  /   \      /   \ 
N00   N01  N03   N04
 |     |    |     |
L00   L01  L02   L03
```

There is no node to the left of node our added parent node (N06), so we are finished updating the MMR.

## Hashing

All nodes in an MMR are just hashes that commit to an item or a tree of items (in the case of parent nodes).

To calculate the hash of a leaf `L` containing data `D`:

```
Node(L) = HASH(L.position || D)
```

For leaf `L03` with data `0x0102030405`, the hash would be:

```
HASH(uint64_t(4) || 0x0102030405) == blake3(0x00000000000000040102030405)
```

To calculate the hash for a parent node `P` with left child `C_L` and right child `C_R`:

```
Node(P) = HASH(P.position || Node(C_L) || Node(C_R))
```

For parent node `N05` of left child `N03` with data `0x0102030405` and right child `N04` with data `0xaabbccdd`, the hash would be:

```
HASH(uint64_t(05) || N03 || N04) == blake3(0x0000000000000005 || blake3(0x00000000000000030102030405) || blake3(0x0000000000000004aabbccdd))
```

## PMMR Root

Similar to how a single hash, the merkle root, can commit to a merkle tree, a single hash, the PMMR root, can be calculated that commits to an entire PMMR. Since there's not a single peak for MMRs the way there is with merkle trees, calculating the root is a little more complicated.

To calculate the root hash, we start with the rightmost (lowest) peak. We\'ll call this `root_hash`. Then, we work up the mountain range until we reach the highest peak, each step calculating `root_hash = HASH(mmr_size || higher_peak_hash || root_hash)`, where `||` is just the concatenate symbol. This process is called "bagging the peaks."

In *example 1*, we have 5 disparate trees, and therefore 5 different peaks. So we start by calculating the `mmr_size`, which in our case would be 57 (N0-N56). Then, we assign `root_hash` as the lowest peak (56), and then bag peak 55, then 52, then 45, and finally peak 30:

```
root_hash = N56.hash;
root_hash = HASH(57 || N55.hash || root_hash);
root_hash = HASH(57 || N52.hash || root_hash);
root_hash = HASH(57 || N45.hash || root_hash);
root_hash = HASH(57 || N30.hash || root_hash);
```

Without temporary variables, this would be:
```
root_hash = HASH(57 || N30.hash || HASH(57 || N45.hash || HASH(57 || N52.hash || HASH(57 || N55.hash || N56.hash))));
```

## TODO

1. Describe difference between pruning and compacting
2. Document PruneList
3. Explain merkle proofs
4. Document `Segment`s and how they\'re built & verified