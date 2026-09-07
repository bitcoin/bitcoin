// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/ecc_init.h>
#include <ecc_context.h>
#include <key.h>
#include <secp256k1.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
