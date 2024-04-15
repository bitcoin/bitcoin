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
ProofOfStake::ProofOfStake(const Points& staked_commitments, const Scalar& eta_fiat_shamir, const blsct::Message& eta_phi, const Scalar& m, const Scalar& f, const uint32_t& prev_time, const uint64_t& stake_modifier, const uint32_t& time, const unsigned int& next_target)
{
    range_proof::GeneratorsFactory<Mcl> gf;
    range_proof::Generators<Arith> gen = gf.GetInstance(TokenId());

    Point sigma = gen.G * m + gen.H * f;

    auto setup = SetMemProofSetup<Arith>::Get();

    // std::cout << __func__ << ": Creating Setmem proof with"
    //           << "\n\t staked_commitments=" << staked_commitments.GetString()
    //           << "\n\t sigma=" << HexStr(sigma.GetVch())
    //           << "\n\t eta_fiat_shamir=" << eta_fiat_shamir.GetString()
    //           << "\n\t eta_phi=" << HexStr(eta_phi)
    //           << "\n\n";

    setMemProof = SetProver::Prove(setup, staked_commitments, sigma, m, f, eta_fiat_shamir, eta_phi);

    auto kernel_hash = CalculateKernelHash(prev_time, stake_modifier, setMemProof.phi, time);
    uint256 min_value = CalculateMinValue(kernel_hash, next_target);

    range_proof::GammaSeed<Arith> gamma_seed(Scalars({f}));
    RangeProver rp;

    // std::cout << __func__ << ": Creating Range proof with"
    //           << "\n\t m=" << m.GetUint64()
    //           << "\n\t kernel_hash=" << kernel_hash.ToString()
    //           << "\n\t next_target=" << next_target
    //           << "\n\t min_value=" << min_value.GetUint64(0)
    //           << "\n\n";

    rangeProof = rp.Prove(Scalars({m}), gamma_seed, {}, eta_phi, min_value.GetUint64(0));

    rangeProof.Vs.Clear();
}

bool ProofOfStake::Verify(const Points& staked_commitments, const Scalar& eta_fiat_shamir, const blsct::Message& eta_phi, const uint32_t& prev_time, const uint64_t& stake_modifier, const uint32_t& time, const unsigned int& next_target) const
{
    return Verify(staked_commitments, eta_fiat_shamir, eta_phi, CalculateKernelHash(prev_time, stake_modifier, setMemProof.phi, time), next_target);
}

bool ProofOfStake::Verify(const Points& staked_commitments, const Scalar& eta_fiat_shamir, const blsct::Message& eta_phi, const uint256& kernel_hash, const unsigned int& next_target) const
{
    auto setup = SetMemProofSetup<Arith>::Get();

    auto setmemres = SetProver::Verify(setup, staked_commitments, eta_fiat_shamir, eta_phi, setMemProof);

    // std::cout << __func__ << ": Verifying Setmem proof with"
    //           << "\n\t staked_commitments=" << staked_commitments.GetString()
    //           << "\n\t eta_fiat_shamir=" << eta_fiat_shamir.GetString()
    //           << "\n\t eta_phi=" << HexStr(eta_phi)
    //           << "\n\t setmemres=" << setmemres
    //           << "\n\n";

    auto kernelhashres = ProofOfStake::VerifyKernelHash(rangeProof, kernel_hash, next_target, eta_phi, setMemProof.phi);

    return setmemres && kernelhashres;
}

bool ProofOfStake::VerifyKernelHash(const RangeProof& range_proof, const uint256& kernel_hash, const unsigned int& next_target, const blsct::Message& eta_phi, const Point& phi)
{
    auto min_value = CalculateMinValue(kernel_hash, next_target);

    auto ret = VerifyKernelHash(range_proof, min_value, eta_phi, phi);

    // std::cout << __func__ << ": Verifying Range proof with"
    //           << "\n\t kernel_hash=" << kernel_hash.ToString()
    //           << "\n\t next_target=" << next_target
    //           << "\n\t kernelhashres=" << ret
    //           << "\n\t min_value=" << CalculateMinValue(kernel_hash, next_target).GetUint64(0)
    //           << "\n\n";

    return ret;
}

bool ProofOfStake::VerifyKernelHash(const RangeProof& range_proof, const uint256& min_value, const blsct::Message& eta_phi, const Point& phi)
{
    auto range_proof_with_value = range_proof;

    range_proof_with_value.Vs.Clear();
    range_proof_with_value.Vs.Add(phi);

    RangeProver rp;
    std::vector<bulletproofs::RangeProofWithSeed<Arith>> proofs;
    proofs.push_back({range_proof_with_value, eta_phi, (CAmount)min_value.GetUint64(0)});

    return rp.Verify(proofs);
}

uint256 ProofOfStake::CalculateMinValue(const uint256& kernel_hash, const unsigned int& next_target)
{
    return ArithToUint256(UintToArith256(kernel_hash) / arith_uint256().SetCompact(next_target));
}
} // namespace blsct