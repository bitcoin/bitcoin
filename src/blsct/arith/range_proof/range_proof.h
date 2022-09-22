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
#include <blsct/arith/range_proof/enriched_proof.h>
#include <blsct/arith/scalar.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>

struct RecoveredTxInput
{
    size_t index;
    CAmount amount;
    Scalar gamma;
    std::string message;
};

struct VerifyLoop1Result
{
    std::vector<EnrichedProof> proofs;
    Scalars to_invert;
    size_t max_LR_len = 0;
    size_t Vs_size_sum = 0;
    size_t to_invert_idx_offset = 0;
};

struct VerifyLoop2Result
{
    G1Points multi_exp;   // TODO give a better name
    Scalar y0 = 0;
    Scalar y1 = 0;
    Scalar z1 = 0;
    Scalar z3 = 0;
    Scalars z4;
    Scalars z5;
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
        const Generators& gens,
        const Scalar& x_ip,
        const Scalars& l,
        const Scalars& r,
        const Scalar& y,
        Proof& st,
        CHashWriter& transcript
    );

    /**
     * Calculates the size of the vector that holds input values
     * the vector size needs o be a power of 2 that is equal to or larger than 2
     * throws exception if the number exceeds the maximum
     */
    static size_t GetInputValueVecLen(const size_t& input_value_len);

    Proof Prove(
        Scalars vs,
        G1Point nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id,
        const size_t max_num_of_tries = 100,
        const std::optional<Scalars> gammas_override = std::nullopt
    );

    /**
     * returns serialization of Scalar w/ preceeding 0's trimmed
     */
    static std::vector<uint8_t> GetTrimmedVch(Scalar& s);

    bool Verify(
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const G1Points& nonces,
        const TokenId& token_id
    );

    std::optional<VerifyLoop1Result> VerifyLoop1(
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const Generators& gens,
        const G1Points& nonces
    );

    std::optional<VerifyLoop2Result> VerifyLoop2(
        const std::vector<EnrichedProof>& proofs
    );

    std::vector<RecoveredTxInput> RecoverTxIns(
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const G1Points& nonces,
        const TokenId& token_id
    );

private:
    static GeneratorsFactory m_gf;

    static Scalar m_one;
    static Scalar m_two;
    static Scalars m_two_pows;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H