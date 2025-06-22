// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/sphincsplus.h>
#include <uint256.h>
#include <vector>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(sphincsplus_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sphincsplus_key_generation_stub)
{
    CSPHINCSKey key;
    BOOST_CHECK(key.GenerateNewKey());
    BOOST_CHECK(!key.privkey.empty());
    BOOST_CHECK(!key.pubkey.empty());
}

BOOST_AUTO_TEST_CASE(sphincsplus_sign_verify_stub)
{
    CSPHINCSKey key;
    BOOST_CHECK(key.GenerateNewKey());

    uint256 hash = uint256S("c3d2e1f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8");
    std::vector<unsigned char> sig;

    // Test signing
    BOOST_CHECK(key.Sign(hash, sig));
    BOOST_CHECK(!sig.empty());

    // Test verification
    BOOST_CHECK(key.Verify(hash, sig));

    // Test verification failure with wrong hash
    uint256 wrong_hash = uint256S("0000000000000000000000000000000000000000000000000000000000000001");
    BOOST_CHECK(!key.Verify(wrong_hash, sig));

    // Test verification failure with wrong signature
    std::vector<unsigned char> wrong_sig = sig;
    if (!wrong_sig.empty()) {
        wrong_sig[0] ^= 0xFF; // Flip a bit
    } else {
        wrong_sig.push_back(0x00); // Make it non-empty if it was
    }
    BOOST_CHECK(!key.Verify(hash, wrong_sig));
}

BOOST_AUTO_TEST_CASE(sphincsplus_sign_empty_privkey_stub)
{
    CSPHINCSKey key;
    // key.GenerateNewKey() not called, so privkey is empty
    uint256 hash = uint256S("c3d2e1f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8");
    std::vector<unsigned char> sig;
    BOOST_CHECK(!key.Sign(hash, sig));
}

BOOST_AUTO_TEST_CASE(sphincsplus_verify_empty_pubkey_stub)
{
    CSPHINCSKey key;
    // Call GenerateNewKey to have a valid signature from a valid (dummy) privkey
    CSPHINCSKey signing_key;
    BOOST_CHECK(signing_key.GenerateNewKey());
    uint256 hash = uint256S("c3d2e1f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8");
    std::vector<unsigned char> sig;
    BOOST_CHECK(signing_key.Sign(hash, sig));

    // Now try to verify with a key that has an empty pubkey
    BOOST_CHECK(!key.Verify(hash, sig));
}

BOOST_AUTO_TEST_SUITE_END()
