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
#include <blsct/arith/scalar.h>
#include <ctokens/tokenid.h>

struct proof_data_t
{
    Scalar x, y, z, x_ip;
    Scalars ws;   // originally w
    G1Points Vs;  // originally V
    size_t log_m, inv_offset;
};

struct RangeProofState {
    G1Points Vs;
    G1Point A;
    G1Point S;
    G1Point T1;
    G1Point T2;
    Scalar t_hat;
    Scalar a;
    Scalar b;
    G1Points Ls;
    G1Points Rs;
    Scalar mu;
    Scalar tau_x;
    Scalar t;
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
        const G1Point& H,
        const size_t& mn,
        const Scalar& x_ip,
        const Scalars& l,
        const Scalars& r,
        const Scalar& y,
        RangeProofState& st,
        CHashWriter& transcript
    );

    /**
     * Calculates the size of the vector that holds input values
     * the vector size needs o be a power of 2 that is equal to or larger than 2
     * throws exception if the number exceeds the maximum
     */
    size_t GetInputValueVecLen(size_t input_value_len);

    RangeProofState Prove(
        Scalars vs,
        G1Point nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id,
        const size_t max_num_of_tries = 100,
        const std::optional<Scalars> gammas_override = std::nullopt
    );

    // size of input value is fixed to 64-bit
    inline static const size_t m_bit_size = 64;

    // values are assigned in the first constructor call
    static Scalar m_one;
    static Scalar m_two;
    static Scalars m_two_pow_bit_size;

private:
    //static Generators m_gens;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_static_values_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H