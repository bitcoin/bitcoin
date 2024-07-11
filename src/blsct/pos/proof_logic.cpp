// Copyright (c) 2024 The Navio Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/pos.h>
#include <blsct/pos/proof_logic.h>
#include <logging.h>
#include <util/strencodings.h>

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

    LogPrint(BCLog::POPS, "Creating PoPS:\n    Eta fiat shamir: %s\n   Eta phi: %s\n   Next Target: %d\n   Staked Commitments:%s\n", HexStr(eta_fiat_shamir), HexStr(eta_phi), next_target, staked_commitments.GetString());

    return ProofOfStake(staked_commitments, eta_fiat_shamir, eta_phi, m, f, pindexPrev->nTime, pindexPrev->nStakeModifier, block.nTime, next_target);
}

bool ProofOfStakeLogic::Verify(const CCoinsViewCache& cache, const CBlockIndex* pindexPrev, const CBlock& block, const Consensus::Params& params)
{
    auto staked_commitments = cache.GetStakedCommitments().GetElements();

    if (staked_commitments.Size() < 2) {
        LogPrint(BCLog::POPS, "PoPS rejected. Staked commitments size is %d\n", staked_commitments.Size());
        return false;
    }

    auto eta_fiat_shamir = blsct::CalculateSetMemProofRandomness(pindexPrev);
    auto eta_phi = blsct::CalculateSetMemProofGeneratorSeed(pindexPrev);

    auto kernel_hash = blsct::CalculateKernelHash(pindexPrev, block);
    auto next_target = blsct::GetNextTargetRequired(pindexPrev, &block, params);

    LogPrint(BCLog::POPS, "Verifying PoPS:\n   Eta fiat shamir: %s\n   Eta phi: %s\n   Kernel Hash: %s\n   Next Target: %d\n   Staked Commitments:%s\n", HexStr(eta_fiat_shamir), HexStr(eta_phi), kernel_hash.ToString(), next_target, staked_commitments.GetString());

    auto res = block.posProof.Verify(staked_commitments, eta_fiat_shamir, eta_phi, kernel_hash, next_target);

    LogPrint(BCLog::POPS, "Result: %s\n", VerificationResultToString(res));

    return res == blsct::ProofOfStake::VerificationResult::VALID;
}
} // namespace blsct