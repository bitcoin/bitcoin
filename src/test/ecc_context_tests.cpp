// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/ecc_init.h>
#include <crypto/sha256.h>
#include <ecc_context.h>
#include <key.h>
#include <pubkey.h>
#include <secp256k1.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

BOOST_AUTO_TEST_SUITE(ecc_context_tests)

BOOST_AUTO_TEST_CASE(context_lifecycle)
{
    BOOST_CHECK(GetSecp256k1SignContext() == nullptr);
    BOOST_CHECK(GetSecp256k1VerifyContext() == secp256k1_context_static);
    {
        const auto ecc_context{MakeContextECC()};
        BOOST_CHECK(GetSecp256k1SignContext() == ecc_context->SignContext());
        BOOST_CHECK(GetSecp256k1VerifyContext() == ecc_context->VerifyContext());
        BOOST_CHECK(ecc_context->SignContext() != nullptr);
        BOOST_CHECK(ecc_context->VerifyContext() != nullptr);
        BOOST_CHECK(ecc_context->VerifyContext() != ecc_context->SignContext());
        BOOST_CHECK(ecc_context->VerifyContext() != secp256k1_context_static);
        std::vector<unsigned char> sig;
        BOOST_CHECK(GenerateRandomKey().Sign(uint256::ONE, sig));
    }
    BOOST_CHECK(GetSecp256k1SignContext() == nullptr);
    BOOST_CHECK(GetSecp256k1VerifyContext() == secp256k1_context_static);
}

int g_sha256_transform_calls{0};
void CountingSHA256Transform(uint32_t* state, const unsigned char* blocks64, size_t n_blocks)
{
    ++g_sha256_transform_calls;
    SHA256Transform(state, blocks64, n_blocks);
}

BOOST_AUTO_TEST_CASE(verify_context_sha256)
{
    const auto ecc_context{MakeContextECC()};
    const CKey key{GenerateRandomKey()};
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignSchnorr(uint256::ONE, sig, /*merkle_root=*/nullptr, uint256::ZERO));

    secp256k1_context_set_sha256_compression(const_cast<secp256k1_context*>(ecc_context->VerifyContext()), CountingSHA256Transform);

    g_sha256_transform_calls = 0;
    BOOST_CHECK(XOnlyPubKey{key.GetPubKey()}.VerifySchnorr(uint256::ONE, sig));
    BOOST_CHECK_GT(g_sha256_transform_calls, 0);
}

BOOST_AUTO_TEST_SUITE_END()
