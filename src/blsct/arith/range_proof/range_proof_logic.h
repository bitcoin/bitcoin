// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H

#include <optional>
#include <vector>

#include <blsct/arith/range_proof/generators.h>
#include <blsct/arith/range_proof/range_proof_with_transcript.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>

struct AmountRecoveryRequest
{
    size_t id;
    Scalar x;
    Scalar z;
    G1Points Vs;
    G1Points Ls;
    G1Points Rs;
    Scalar mu;
    Scalar tau_x;
    G1Point nonce;

    static AmountRecoveryRequest of(RangeProof& proof, size_t& index, G1Point& nonce);
};

struct RecoveredAmount
{
    RecoveredAmount(
        const size_t& id,
        const CAmount& amount,
        const Scalar& gamma,
        const std::string& message
    ): id{id}, amount{amount}, gamma{gamma}, message{message} {}

    size_t id;
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

// implementation of range proof algorithms described
// based on the paper: https://eprint.iacr.org/2017/1066.pdf
class RangeProofLogic
{
public:
    RangeProofLogic();

    RangeProof Prove(
        Scalars& vs,
        G1Point& nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id
    );

    bool Verify(
        const std::vector<RangeProof>& proofs,
        const TokenId& token_id
    ) const;

    G1Point VerifyProofs(
        const std::vector<RangeProofWithTranscript>& proof_transcripts,
        const Generators& gens,
        const size_t& max_mn
    ) const;

    AmountRecoveryResult RecoverAmounts(
        const std::vector<AmountRecoveryRequest>& reqs,
        const TokenId& token_id
    ) const;

    static void ValidateProofsBySizes(
        const std::vector<RangeProof>& proofs
    );

private:
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
        RangeProof& proof,
        CHashWriter& transcript_gen
    );

    // using pointers for Scalar and GeneratorsFactory to avoid default constructors to be called before mcl initialization
    // these variables are meant to be constant. do not make changes after initialization.
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