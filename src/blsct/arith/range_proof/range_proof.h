// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H

#include <optional>
#include <vector>

#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <blsct/arith/range_proof/proof_with_transcript.h>
#include <blsct/arith/scalar.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>

struct AmountRecoveryReq
{
    size_t index;
    Scalar x;
    Scalar z;
    G1Points Vs;
    G1Points Ls;
    G1Points Rs;
    Scalar mu;
    Scalar tau_x;
    G1Point nonce;
};

struct RecoveredAmount
{
    RecoveredAmount(
        const size_t& index,
        const CAmount& amount,
        const Scalar& gamma,
        const std::string& message
    ): index{index}, amount{amount}, gamma{gamma}, message{message} {}

    size_t index;
    CAmount amount;
    Scalar gamma;
    std::string message;
};

// implementation of range proof described in Bulletproofs
// based on the paper: https://eprint.iacr.org/2017/1066.pdf
class RangeProof
{
public:
    RangeProof();

    void Init();

    G1Point GenerateBaseG1PointH(
        const G1Point& p,
        size_t index,
        TokenId token_id
    ) const;

    bool InnerProductArgument(
        const size_t input_value_vec_len,
        const G1Points& Gi,
        const G1Points& Hi,
        const G1Point& u,
        const Scalar& cx_factor,  // factor to multiply to cL and cR
        const Scalars& l,
        const Scalars& r,
        const Scalar& y,
        Proof& proof,
        CHashWriter& transcript_gen
    );

    /**
     * Returns power of 2 that is greater or equal to input_value_len
     * throws exception if such a number exceeds the maximum
     */
    size_t GetFirstPowerOf2GreaterOrEqTo(const size_t& input_value_len) const;

    /**
     * Take a log2 of the size of concatinated input values in bits
     * to get to the number of rounds required to get a single element
     * by halving the size in each round
     */
    size_t GetInnerProdArgRounds(const size_t& num_input_values) const;

    Proof Prove(
        Scalars vs,
        G1Point nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id
    );

    bool Verify(
        const std::vector<Proof>& proofs,
        const TokenId& token_id
    ) const;

    bool ValidateProofsBySizes(
        const std::vector<Proof>& proofs,
        const size_t& num_rounds
    ) const;

    G1Point VerifyLoop2(
        const std::vector<ProofWithTranscript>& proof_transcripts,
        const Generators& gens,
        const size_t& max_mn
    ) const;

    std::vector<RecoveredAmount> RecoverAmounts(
        const std::vector<AmountRecoveryReq>& reqs,
        const TokenId& token_id
    ) const;

private:
    static GeneratorsFactory m_gf;

    static Scalar m_one;
    static Scalar m_two;
    static Scalars m_two_pows;
    static Scalar m_inner_prod_ones_and_two_pows;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H