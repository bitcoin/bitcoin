// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blsct/double_public_key.h"
#include <bech32_mod.h>
#include <test/util/str.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <random>
#include <set>
#include <string>
#include <iostream>

/** The Bech32 and Bech32m character set for encoding. */
const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

/** The Bech32 and Bech32m character set for decoding. */
const int8_t CHARSET_REV[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};
BOOST_AUTO_TEST_SUITE(bech32_mod_tests)

// randomly embed errors to the given string
void embed_errors(std::string& s, const size_t num_errors) {
    // build a list of indices to change
    std::random_device rd;
    std::mt19937 gen(rd());

    // randonly select indices to change
    std::set<size_t> indices;

    auto sep_idx = s.rfind('1');
    while (indices.size() < num_errors) {
        std::uniform_int_distribution<size_t> dist(sep_idx + 1, s.size() - 9);
        indices.insert(dist(gen));
    }

    // change characters at the indices
    for (auto it = indices.begin(); it != indices.end(); ++it) {
        auto from_idx = CHARSET_REV[static_cast<size_t>(s[*it])];
        auto to_idx = (from_idx + 1) % 32;
        s[*it] = CHARSET[to_idx];
    }
}

std::string gen_random_byte_str(const size_t size) {
    std::string s;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, 255);

    for (size_t i = 0; i < size; ++i) {
        s += dist(gen);
    }
    return s;
}

size_t test_error_detection(
    const size_t num_errors,
    const size_t num_tests,
    const bool expect_errors
) {
    size_t unexpected_results = 0;

    for (size_t i=0; i<num_tests; ++i) {
        for (auto encoding : {bech32_mod::Encoding::BECH32, bech32_mod::Encoding::BECH32M}) {
            // generate random double public key
            std::string dpk = gen_random_byte_str(blsct::DoublePublicKey::SIZE);

            // convert 8-bit vector to 5-bit vector
            std::vector<uint8_t> dpk_v8(dpk.begin(), dpk.end());
            std::vector<uint8_t> dpk_v5;
            ConvertBits<8, 5, true>([&](uint8_t c) { dpk_v5.push_back(c); }, dpk_v8.begin(), dpk_v8.end());

            auto dpk_bech32 = bech32_mod::Encode(encoding, "nv", dpk_v5);
            embed_errors(dpk_bech32, num_errors);

            auto res = bech32_mod::Decode(dpk_bech32);
            std::vector<uint8_t> dpk_v8r;
            ConvertBits<5, 8, false>([&](uint8_t c) { dpk_v8r.push_back(c); }, res.data.begin(), res.data.end());

            if (expect_errors) {
                if (dpk_v8r == dpk_v8) ++unexpected_results;
            } else {
                if (dpk_v8r != dpk_v8) ++unexpected_results;
            }
        }
    }
    return unexpected_results;
}

BOOST_AUTO_TEST_CASE(bech32_mod_test_detecting_errors)
{
    bool failed = false;

    for (size_t num_errors = 0; num_errors <= 5; ++num_errors) {
        size_t unexpected_results =
            test_error_detection(num_errors, 10000, num_errors > 0);

        if (unexpected_results > 0) {
            std::cout << num_errors << "-error cases failed " << unexpected_results << " times" << std::endl;
            failed = true;
        }
    }
    BOOST_CHECK(!failed);
}

BOOST_AUTO_TEST_SUITE_END()

