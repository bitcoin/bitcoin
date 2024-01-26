// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_PROOF_H
#define BLSCT_POS_PROOF_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/set_mem_proof/set_mem_proof.h>
#include <blsct/set_mem_proof/set_mem_proof_prover.h>

using Arith = Mcl;
using Point = Arith::Point;
using Scalar = Arith::Scalar;
using Points = Elements<Point>;
using Prover = SetMemProofProver<Arith>;

namespace blsct {

class ProofOfStake
{
public:
    ProofOfStake()
    {
    }

    ProofOfStake(SetMemProof<Mcl> setMemProof) : setMemProof(setMemProof)
    {
    }

    ProofOfStake(const Points& stakedCommitments, const std::vector<unsigned char>& eta, const Scalar& m, const Scalar& f);

    bool Verify(const Points& stakedCommitments, const std::vector<unsigned char>& eta) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ::Serialize(s, setMemProof);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        ::Unserialize(s, setMemProof);
    }

    SetMemProof<Mcl> setMemProof;
};
} // namespace blsct

#endif // BLSCT_POS_PROOF_H