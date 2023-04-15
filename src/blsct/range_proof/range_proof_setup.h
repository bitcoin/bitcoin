// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_CONFIG_H
#define NAVCOIN_BLSCT_RANGE_PROOF_CONFIG_H

#include <cstddef>
#include <cmath>

class RangeProofSetup
{
public:
    // maximum # of retries allowed for RangeProofLogic::Prove function
    inline static const size_t max_prove_func_retries = 100;

    // size of each input value in bits. N in old code
    inline static const size_t m_input_value_bits = 64;

    inline static const size_t m_inupt_value_bits_log2 = std::log2(m_input_value_bits);

    // maximum # of input values
    inline static const size_t m_max_input_values = 16;

    // maximum size of the first half of the message
    inline static const size_t m_message_1_max_size = 23;

    // 23 and 31 bytes for message 1 and message 2 respectively
    inline static const size_t m_max_message_size = 54;

    inline static const size_t m_max_input_value_vec_len = m_max_input_values * m_input_value_bits;
};

#endif // NAVCOIN_BLSCT_RANGE_PROOF_CONFIG_H