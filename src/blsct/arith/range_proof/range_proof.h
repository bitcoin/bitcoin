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

struct AmountRecoveryRequest
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

    static AmountRecoveryRequest of(Proof& proof, size_t& index, G1Point& nonce);
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

struct AmountRecoveryResult
{
    bool is_completed;  // done doesn't mean recovery success
    std::vector<RecoveredAmount> amounts;

    static AmountRecoveryResult failure();
};

// implementation of range proof described in Bulletproofs
// based on the paper: https://eprint.iacr.org/2017/1066.pdf
class RangeProof
{
public:
    RangeProof();

    Scalar GetUint64Max() const;

    G1Point GenerateBaseG1PointH(
        const G1Point& p,
        size_t index,
        TokenId token_id
    ) const;

    bool InnerProductArgument(
        const size_t input_value_vec_len,
        G1Points& Gi,
        G1Points& Hi,
        const G1Point& u,
        const Scalar& cx_factor,  // factor to multiply to cL and cR
        Scalars& a,
        Scalars& b,
        const Scalar& y,
        Proof& proof,
        CHashWriter& transcript_gen
    );

    Proof Prove(
        Scalars& vs,
        G1Point& nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id
    );

    bool Verify(
        const std::vector<Proof>& proofs,
        const TokenId& token_id
    ) const;

    void ValidateProofsBySizes(
        const std::vector<Proof>& proofs
    ) const;

    G1Point VerifyLoop2(
        const std::vector<ProofWithTranscript>& proof_transcripts,
        const Generators& gens,
        const size_t& max_mn
    ) const;

    AmountRecoveryResult RecoverAmounts(
        const std::vector<AmountRecoveryRequest>& reqs,
        const TokenId& token_id
    ) const;

private:
    // using pointers for Scalar and GeneratorsFactory to avoid default constructors to be called before mcl initialization
    static GeneratorsFactory* m_gf;

    static Scalar* m_one;
    static Scalar* m_two;
    static Scalars* m_two_pows_64;
    static Scalar* m_inner_prod_1x2_pows_64;
    static Scalar* m_uint64_max;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H