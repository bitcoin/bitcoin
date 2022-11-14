// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H

#include <cstddef>

class Config
{
public:
    /**
     * Returns power of 2 that is greater or equal to input_value_len
     * throws exception if such a number exceeds the maximum
     */
    static size_t GetFirstPowerOf2GreaterOrEqTo(const size_t& input_value_vec_len);

    // maximum # of retries allowed for RangeProof::Prove function
    inline static const size_t max_prove_func_retries = 100;

    // size of each input value in bits. N in old code
    inline static const size_t m_input_value_bits = 64;

    // maximum # of input values
    inline static const size_t m_max_input_values = 16;

    // 23 and 31 bytes for message 1 and message 2 respectively
    inline static const size_t m_max_message_size = 54;

    inline static const size_t m_max_input_value_vec_len = m_max_input_values * m_input_value_bits;

    // maximum number of tries in Prove() function
    inline static const size_t m_max_prove_tries = 100;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H