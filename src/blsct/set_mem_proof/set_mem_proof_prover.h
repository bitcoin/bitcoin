// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROVER_H
#define NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROVER_H

#include <vector>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>
#include <blsct/set_mem_proof/set_mem_proof.h>
#include <blsct/building_block/imp_inner_prod_arg.h>
#include <hash.h>

class SetMemProofProver {
public:
    using Scalar = Mcl::Scalar;
    using Point = Mcl::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    SetMemProof Prove(
        const Points& Ys, // N Pedersen Commitment Y^n
        const Point& sigma,  // Commitment of the set member
        const Scalar& f,  // Mask f used for the commitment of the set member
        const Scalar& m,  // Message used for the commitment of the set member
        const Scalar& eta  // Entropy
    ) const;

    const SetMemProofSetup& GenSetup() const;

    bool Verify(
        const Points& Ys,  // Same as Ys in Prove()
        const Scalar& eta,  // Same as eta in Prove()
        const SetMemProof& proof  // The output of Prove()
    ) const;

#ifndef BOOST_UNIT_TEST
private:
#endif
    Scalar ComputeX(
        const Scalar& omega,
        const Scalar& y,
        const Scalar& z,
        const Point& T1,
        const Point& T2
    ) const;

    std::vector<uint8_t> ComputeStr(
        Points Ys,
        Point A1,
        Point A2,
        Point S1,
        Point S2,
        Point S3,
        Point phi,
        Scalar eta
    ) const;

    Points ExtendYs(
        const Points& Ys_src,
        const size_t& new_size
    ) const;

    const SetMemProofSetup& setup = GenSetup();
    static const Scalar& one();
};

#endif // NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROVER_H
