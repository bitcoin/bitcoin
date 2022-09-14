// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H

#include <cstddef>

class Config
{
public:
    // maximum # of retries allowed for RangeProof::Prove function
    inline static const size_t max_prove_func_retries = 100;

    // size of each input value in bits
    inline static const size_t m_input_value_bits = 64;  // ex m_bit_size

    // maximum # of input values
    inline static const size_t m_max_num_values = 16;  // ex max_value_len

    inline static const size_t m_max_message_size = 54;
    inline static const size_t m_max_input_value_vec_len = m_max_num_values * m_input_value_bits;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_CONFIG_H