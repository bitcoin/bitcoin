// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <key.h>
#include <key_io.h>
#include <musig.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <crypto/hex_base.h>

#include <string>
#include <vector>

namespace {

struct BIP328TestVector {
    std::vector<std::string> pubkeys;
    std::string expected_aggregate_pubkey;
    std::string expected_aggregate_xpub;
};

BOOST_FIXTURE_TEST_SUITE(bip328_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(valid_keys)
{
    // BIP 328 test vectors
    std::vector<BIP328TestVector> test_vectors = {
        // Test vector 0
        {
            .pubkeys = {
                "03935F972DA013F80AE011890FA89B67A27B7BE6CCB24D3274D18B2D4067F261A9",
                "02F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9"
            },
            .expected_aggregate_pubkey = "0354240c76b8f2999143301a99c7f721ee57eee0bce401df3afeaa9ae218c70f23",
            .expected_aggregate_xpub = "xpub661MyMwAqRbcFt6tk3uaczE1y6EvM1TqXvawXcYmFEWijEM4PDBnuCXwwXEKGEouzXE6QLLRxjatMcLLzJ5LV5Nib1BN7vJg6yp45yHHRbm"
        },
        // Test vector 1
        {
            .pubkeys = {
                "02F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9",
                "03DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659",
                "023590A94E768F8E1815C2F24B4D80A8E3149316C3518CE7B7AD338368D038CA66"
            },
            .expected_aggregate_pubkey = "0290539eede565f5d054f32cc0c220126889ed1e5d193baf15aef344fe59d4610c",
            .expected_aggregate_xpub = "xpub661MyMwAqRbcFt6tk3uaczE1y6EvM1TqXvawXcYmFEWijEM4PDBnuCXwwVk5TFJk8Tw5WAdV3DhrGfbFA216sE9BsQQiSFTdudkETnKdg8k"
        },
        // Test vector 2
        {
            .pubkeys = {
                "02DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659",
                "023590A94E768F8E1815C2F24B4D80A8E3149316C3518CE7B7AD338368D038CA66",
                "02F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9",
                "03935F972DA013F80AE011890FA89B67A27B7BE6CCB24D3274D18B2D4067F261A9"
            },
            .expected_aggregate_pubkey = "022479f134cdb266141dab1a023cbba30a870f8995b95a91fc8464e56a7d41f8ea",
            .expected_aggregate_xpub = "xpub661MyMwAqRbcFt6tk3uaczE1y6EvM1TqXvawXcYmFEWijEM4PDBnuCXwwUvaZYpysLX4wN59tjwU5pBuDjNrPEJbfxjLwn7ruzbXTcUTHkZ"
        }
    };

    // Iterate through all test vectors
    for (int i = 0; i < (int) test_vectors.size(); ++i) {
        const auto& test = test_vectors[i];

        // Parse public keys
        std::vector<CPubKey> pubkeys;
        for (const auto& hex_pubkey : test.pubkeys) {
            std::vector<unsigned char> data = ParseHex(hex_pubkey);
            CPubKey pubkey(data.begin(), data.end());
            pubkeys.push_back(pubkey);
        }

        // Aggregate public keys
        std::optional<CPubKey> m_aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
        BOOST_CHECK_MESSAGE(m_aggregate_pubkey.has_value(), "Test vector " << i << ": Failed to aggregate pubkeys");

        // Check aggregate pubkey
        std::string combined_keys = HexStr(m_aggregate_pubkey.value());
        BOOST_CHECK_MESSAGE(combined_keys == test.expected_aggregate_pubkey, "Test vector " << i << ": Aggregate pubkey mismatch");

        // Create extended public key
        CExtPubKey extpub = CreateMuSig2SyntheticXpub(m_aggregate_pubkey.value());

        // Check xpub
        std::string xpub = EncodeExtPubKey(extpub);
        BOOST_CHECK_MESSAGE(xpub == test.expected_aggregate_xpub, "Test vector " << i << ": Synthetic xpub mismatch");
    }
}

BOOST_AUTO_TEST_CASE(invalid_key)
{
    std::vector<std::string> test_vectors = {
        "00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "02DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659"
    };

    // Parse public keys
    std::vector<CPubKey> pubkeys;
    for (const auto& hex_pubkey : test_vectors) {
        std::vector<unsigned char> data = ParseHex(hex_pubkey);
        CPubKey pubkey(data.begin(), data.end());
        pubkeys.push_back(pubkey);
    }

    // Aggregate public keys
    std::optional<CPubKey> m_aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
    BOOST_CHECK_MESSAGE(!m_aggregate_pubkey.has_value(), "Aggregate key with an invalid public key is null");
}

BOOST_AUTO_TEST_SUITE_END()

}
