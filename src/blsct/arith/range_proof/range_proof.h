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
#include <blsct/arith/range_proof/proof_with_derived_values.h>
#include <blsct/arith/scalar.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>

struct TxInToRecover
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

struct RecoveredTxInput
{
    RecoveredTxInput(
        const size_t& index,
        const CAmount& input_value0,
        const Scalar& input_value0_gamma,
        const std::string& message
    ): index{index}, input_value0{input_value0},
        input_value0_gamma{input_value0_gamma},
        message{message} {}

    size_t index;
    CAmount input_value0;
    Scalar input_value0_gamma;
    std::string message;
};

struct VerifyLoop1Result
{
    std::vector<ProofWithDerivedValues> proof_derivs;
    size_t max_num_rounds = 0;
    size_t Vs_size_sum = 0;
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
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const TokenId& token_id
    ) const;

    bool RangeProof::ValidateProofsBySizes(
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const size_t& num_rounds
    ) const;

    /**
     * Recover derived scalars used in Prove() from proof
     * while calculating total number of bits in cocatinated input value
     * and maximum size of L,R
     */
    VerifyLoop1Result VerifyLoop1(
        const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
        const size_t& num_rounds
    ) const;

    VerifyLoop2Result VerifyLoop2(
        const std::vector<ProofWithDerivedValues>& proof_derivs
    ) const;

    std::vector<RecoveredTxInput> RecoverTxIns(
        const std::vector<TxInToRecover>& tx_ins,
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