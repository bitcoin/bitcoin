// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/pos.h>
#include <blsct/pos/proof_logic.h>

using Arith = Mcl;
using Point = Arith::Point;
using Scalar = Arith::Scalar;
using Points = Elements<Point>;
using Prover = SetMemProofProver<Arith>;

namespace blsct {
ProofOfStake ProofOfStakeLogic::Create(const CCoinsViewCache& cache, const Scalar& m, const Scalar& f, const CBlockIndex* pindexPrev, const CBlock& block, const Consensus::Params& params)
{
    auto staked_commitments = cache.GetStakedCommitments().GetElements();
    auto eta_fiat_shamir = blsct::CalculateSetMemProofRandomness(pindexPrev);
    auto eta_phi = blsct::CalculateSetMemProofGeneratorSeed(pindexPrev);

    auto next_target = blsct::GetNextTargetRequired(pindexPrev, &block, params);
    return ProofOfStake(staked_commitments, eta_fiat_shamir, eta_phi, m, f, pindexPrev->nTime, pindexPrev->nStakeModifier, block.nTime, next_target);
}

bool ProofOfStakeLogic::Verify(const CCoinsViewCache& cache, const CBlockIndex* pindexPrev, const CBlock& block, const Consensus::Params& params)
{
    auto staked_commitments = cache.GetStakedCommitments().GetElements();

    if (staked_commitments.Size() < 2) {
        return false;
    }

    auto eta_fiat_shamir = blsct::CalculateSetMemProofRandomness(pindexPrev);
    auto eta_phi = blsct::CalculateSetMemProofGeneratorSeed(pindexPrev);

    auto kernel_hash = blsct::CalculateKernelHash(pindexPrev, block);
    auto next_target = blsct::GetNextTargetRequired(pindexPrev, &block, params);

    auto res = block.posProof.Verify(staked_commitments, eta_fiat_shamir, eta_phi, kernel_hash, next_target);

    return res;
}
} // namespace blsct