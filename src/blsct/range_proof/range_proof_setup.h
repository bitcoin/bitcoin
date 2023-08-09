// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_SETUP_H
#define NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_SETUP_H

#include <cstddef>
#include <cmath>

class RangeProofSetup
{
public:
    // maximum # of retries allowed for RangeProofLogic::Prove function
    inline static const size_t max_prove_func_retries = 100;

    // size of each input value in bits. N in old code
    inline static const size_t num_input_value_bits = 64;

    // maximum # of input values
    inline static const size_t max_input_values = 16;

    // maximum size of the first half of the message
    inline static const size_t message_1_max_size = 23;

    // 23 and 31 bytes for message 1 and message 2 respectively
    inline static const size_t max_message_size = 54;

    inline static const size_t max_input_value_vec_len =
        max_input_values * num_input_value_bits;
};

#endif // NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_SETUP_H