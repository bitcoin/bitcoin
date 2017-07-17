Surjection Proof Module
===========================

This module implements a scheme by which a given point can be proven to be
equal to one of a set of points, plus a known difference. This is used in
Confidential Assets when reblinding "asset commitments", which are NUMS
points, to prove that the underlying NUMS point does not change during
reblinding.

Assets are represented, in general, by a 32-byte seed (a hash of some
transaction data) which is hashed to form a NUMS generator, which appears
on the blockchain only in blinded form. We refer to the seed as an
"asset ID" and the blinded generator as an "(ephemeral) asset commitment".
These asset commitments are unique per-output, and their NUMS components
are in general known only to the holder of the output.

The result is that within a transaction, all outputs are able to have
a new uniformly-random asset commitment which cannot be associated with
any individual input asset id, but verifiers are nonetheless assured that
all assets coming out of a transaction are ones that went in.

### Terminology

Assets are identified by a 32-byte "asset ID". In this library these IDs
are used as input to a point-valued hash function `H`. We usually refer
to the hash output as `A`, since this output is the only thing that appears
in the algebra.

Then transaction outputs have "asset commitments", which are curvepoints
of the form `A + rG`, where `A` is the hash of the asset ID and `r` is
some random "blinding factor".

### Design Rationale

Confidential Assets essentially works by replacing the second NUMS generator
`H` in Confidental Transactions with a per-asset unique NUMS generator. This
allows the same verification equation (the sum of all blinded inputs must
equal the sum of all blinded outputs) to imply that quantity of *every* asset
type is preserved in each transaction.

It turns out that even if outputs are reblinded by the addition of `rG` for
some known `r`, this verification equation has the same meaning, with one
caveat: verifiers must be assured that the reblinding preserves the original
generators (and does not, for example, negate them).

This assurance is what surjection proofs provide.

### Limitations

The naive scheme works as follows: every output asset is shown to have come
from some input asset. However, the proofs scale with the number of input
assets, so for all outputs the total size of all surjection proofs is `O(mn)`
for `m`, `n` the number of inputs and outputs.

We therefore restrict the number of inputs that each output may have come
from to 3 (well, some fixed number, which is passed into the API), which
provides a weaker form of blinding, but gives `O(n)` scaling. Over many
transactions, the privacy afforded by this increases exponentially.

### Our Scheme

Our scheme works as follows. Proofs are generated in two steps, "initialization"
which selects a subset of inputs and "generation" which does the mathematical
part of proof generation.

Every input has an asset commitment for which we know the blinding key and
underlying asset ID.

#### Initialization

The initialization function takes a list of input asset IDs and one output
asset ID. It chooses an input subset of some fixed size repeatedly until it
the output ID appears at least once in its subset.

It stores a bitmap representing this subset in the proof object and returns
the number of iterations it needed to choose the subset. The reciprocal of
this represents the probability that a uniformly random input-output
mapping would correspond to the actual input-output mapping, and therefore
gives a measure of privacy. (Lower iteration counts are better.)

It also informs the caller the index of the input whose ID matches the output.

As the API works on only a single output at a time, the total probability
should be computed by multiplying together the counts for each output.

#### Generation

The generation function takes a list of input asset commitments, an output
asset commitment, the input index returned by the initialization step, and
blinding keys for (a) the output commitment, (b) the input commitment. Here
"the input commitment" refers specifically to the input whose index was
chosen during initialization.

Next, it computes a ring signature over the differences between the output
commitment and every input commitment chosen during initialization. Since
the discrete log of one of these is the difference between the output and
input blinding keys, it is possible to create a ring signature over every
differences will be the blinding factor of the output. We create such a
signature, which completes the proof.

#### Verification

Verification takes a surjection proof object, a list of input commitments,
and an output commitment. The proof object contains a ring signature and
a bitmap describing which input commitments to use, and verification
succeeds iff the signature verifies.


