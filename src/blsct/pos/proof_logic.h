// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_PROOF_LOGIC_H
#define BLSCT_POS_PROOF_LOGIC_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/pos/proof.h>
#include <blsct/set_mem_proof/set_mem_proof.h>
#include <blsct/set_mem_proof/set_mem_proof_prover.h>
#include <chain.h>
#include <coins.h>

using Arith = Mcl;
using Point = Arith::Point;
using Scalar = Arith::Scalar;
using Points = Elements<Point>;
using Prover = SetMemProofProver<Arith>;

namespace blsct {
class ProofOfStakeLogic : public blsct::ProofOfStake
{
public:
    static ProofOfStake Create(const CCoinsViewCache& cache, const Scalar& m, const Scalar& f, const CBlockIndex& pindexPrev, const CBlock& block, const Consensus::Params& params);
    static bool Verify(const CCoinsViewCache& cache, const CBlockIndex& pindexPrev, const CBlock& block, const Consensus::Params& params);
};
} // namespace blsct

#endif // BLSCT_POS_PROOF_LOGIC_H