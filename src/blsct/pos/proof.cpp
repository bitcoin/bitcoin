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
using Scalars = Elements<Scalar>;
using RangeProof = bulletproofs::RangeProof<Arith>;
using RangeProver = bulletproofs::RangeProofLogic<Arith>;
using SetProof = SetMemProof<Arith>;
using SetProver = SetMemProofProver<Arith>;

namespace blsct {
ProofOfStake::ProofOfStake(const Points& stakedCommitments, const Scalar& eta_fiat_shamir, const blsct::Message& eta_phi, const Scalar& m, const Scalar& f, const uint32_t& prev_time, const uint64_t& stake_modifier, const uint32_t& time, const unsigned int& next_target)
{
    range_proof::GeneratorsFactory<Mcl> gf;
    range_proof::Generators<Arith> gen = gf.GetInstance(TokenId());

    Point sigma = gen.G * m + gen.H * f;

    auto setup = SetMemProofSetup<Arith>::Get();

    std::cout << __func__ << ": Creating with:\n\tList of commitments: " << stakedCommitments.GetString() << "\n\tSigma: " << HexStr(sigma.GetVch()) << "\n\tEta fiat shamir: " << eta_fiat_shamir.GetString() << "\n\tEta phi: " << HexStr(eta_phi) << "\n";

    setMemProof = SetProver::Prove(setup, stakedCommitments, sigma, m, f, eta_fiat_shamir, eta_phi);

    std::cout << __func__ << ": Verification of proof:\n\tList of commitments: " << stakedCommitments.GetString() << "\n\tEta fiat shamir: " << eta_fiat_shamir.GetString() << "\n\tEta phi: " << HexStr(eta_phi) << "\n\tResult:" << SetProver::Verify(setup, stakedCommitments, eta_fiat_shamir, eta_phi, setMemProof) << "\n ";

    auto kernel_hash = CalculateKernelHash(prev_time, stake_modifier, setMemProof.phi, time);
    uint256 min_value = ArithToUint256(UintToArith256(kernel_hash) / arith_uint256().SetCompact(next_target));

    range_proof::GammaSeed<Arith> gamma_seed(Scalars({f}));
    RangeProver rp;

    rangeProof = rp.Prove(Scalars({m}), gamma_seed, {}, eta_phi, min_value);
    rangeProof.Vs.Clear();
}

bool ProofOfStake::Verify(const Points& stakedCommitments, const Scalar& eta_fiat_shamir, const blsct::Message& eta_phi, const uint256& kernel_hash, const unsigned int& next_target) const
{
    auto setup = SetMemProofSetup<Arith>::Get();

    uint256 min_value = CalculateMinValue(kernel_hash, next_target);

    return SetProver::Verify(setup, stakedCommitments, eta_fiat_shamir, eta_phi, setMemProof) && ProofOfStake::VerifyKernelHash(rangeProof, kernel_hash, next_target, eta_phi, min_value, setMemProof.phi);
}

bool ProofOfStake::VerifyKernelHash(const RangeProof& rangeProof, const uint256& kernel_hash, const unsigned int& next_target, const blsct::Message& eta_phi, const uint256& min_value, const Point& phi)
{
    auto range_proof_with_value = rangeProof;

    range_proof_with_value.Vs.Clear();
    range_proof_with_value.Vs.Add(phi);

    RangeProver rp;
    std::vector<bulletproofs::RangeProofWithSeed<Arith>> proofs;
    proofs.push_back({range_proof_with_value, eta_phi, min_value});

    return rp.Verify(proofs);
}

uint256 ProofOfStake::CalculateMinValue(const uint256& kernel_hash, const unsigned int& next_target)
{
    return ArithToUint256(UintToArith256(kernel_hash) / arith_uint256().SetCompact(next_target));
}
} // namespace blsct