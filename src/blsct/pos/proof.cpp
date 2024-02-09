// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/proof.h>
#include <blsct/range_proof/generators.h>
#include <util/strencodings.h>

using Arith = Mcl;
using Point = Arith::Point;
using Scalar = Arith::Scalar;
using Points = Elements<Point>;
using Prover = SetMemProofProver<Arith>;

namespace blsct {
ProofOfStake::ProofOfStake(const Points& stakedCommitments, const std::vector<unsigned char>& eta, const Scalar& m, const Scalar& f)
{
    auto setup = SetMemProofSetup<Arith>::Get();

    range_proof::GeneratorsFactory<Mcl> gf;
    range_proof::Generators<Arith> gen = gf.GetInstance(TokenId());

    Point sigma = gen.G * m + gen.H * f;

    setMemProof = Prover::Prove(setup, stakedCommitments, sigma, m, f, eta);
}

bool ProofOfStake::Verify(const Points& stakedCommitments, const std::vector<unsigned char>& eta, const uint256& kernelHash, const unsigned int& posTarget) const
{
    auto setup = SetMemProofSetup<Arith>::Get();
    auto res = Prover::Verify(setup, stakedCommitments, eta, setMemProof);

    return res && VerifyKernelHash(kernelHash, posTarget);
}

bool ProofOfStake::VerifyKernelHash(const uint256& kernelHash, const unsigned int& posTarget)
{
    arith_uint256 posTargetBn;
    posTargetBn.SetCompact(posTarget);

    arith_uint256 kernelHashBn = UintToArith256(kernelHash);

    return kernelHashBn < posTargetBn;
}
} // namespace blsct