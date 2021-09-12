// Copyright (c) 2013-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <hash.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <test/data/ctvhash.json.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <version.h>

#include <iostream>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(ctvhash_tests, BasicTestingSetup)

// Goal: check that CTV Hash Functions generate correct hash
BOOST_AUTO_TEST_CASE(ctvhash_from_data)
{
    UniValue tests = read_json(std::string(json_tests::ctvhash, json_tests::ctvhash + sizeof(json_tests::ctvhash)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        // comment
        if (test.isStr())
            continue;
        else if (test.isObject()) {
            std::string raw_tx;
            std::vector<uint256> hash;
            std::vector<uint32_t> spend_index;
            try {
                auto& hash_arr = test["result"].get_array();
                for (size_t i = 0; i < hash_arr.size(); ++i) {
                    hash.emplace_back();
                    hash.back().SetHex(hash_arr[i].get_str());
                    // reverse because python's sha256().digest().hex() is
                    // backwards
                    std::reverse(hash.back().begin(), hash.back().end());
                }
            } catch (...) {
                BOOST_ERROR("Bad test: Results could not be deserialized" << strTest);
                break;
            }
            try {
                auto& spend_index_arr = test["spend_index"].get_array();
                for (size_t i = 0; i < spend_index_arr.size(); ++i) {
                    spend_index.emplace_back(static_cast<uint32_t>(spend_index_arr[i].get_int64()));
                }
            } catch (...) {
                BOOST_ERROR("Bad test: spend_index could not be deserialized" << strTest);
                break;
            }
            if (spend_index.size() != hash.size()) {
                BOOST_ERROR("Bad test: Spend Indexes not same length as Result Hashes: " << strTest);
                break;
            }
            CMutableTransaction tx;
            try {
                raw_tx = test["hex_tx"].get_str();
                // deserialize test data
                CDataStream stream(ParseHex(raw_tx), SER_NETWORK, PROTOCOL_VERSION);
                tx = CMutableTransaction(deserialize, stream);
            } catch (...) {
                BOOST_ERROR("Bad test, couldn't deserialize hex_tx: " << strTest);
                continue;
            }
            PrecomputedTransactionData data{tx};
            for (size_t i = 0; i < hash.size(); ++i) {
                uint256 sh = GetDefaultCheckTemplateVerifyHash(tx, data.m_outputs_single_hash, data.m_sequences_single_hash, spend_index[i]);
                if (sh != hash[i]) {
                    BOOST_ERROR("Expected: " << sh << " Got: " << hash[i] << " For:\n"
                                             << strTest);
                }
            }
            // Change all of the outpoints and there should be no difference.
            FastRandomContext fr;

            for (auto i = 0; i < 200; ++i) {
                CMutableTransaction txc = tx;
                bool hash_will_change = false;
                // do n mutations, 50% of being 1, 50% chance of being 2-11
                const uint64_t n_mutations = fr.randbool()? (fr.randrange(10)+2) : 1;
                for (uint64_t j = 0; j < n_mutations; ++j) {
                    // on the first 50 passes, modify in ways that will not change hash
                    const int mutate_field = i < 50 ? fr.randrange(2) : fr.randrange(10);
                    switch (mutate_field) {
                    case 0: {
                        // no need to rejection sample on 256 bits
                        auto which = fr.randrange(tx.vin.size());
                        tx.vin[which].prevout.hash = fr.rand256();
                        tx.vin[which].prevout.n = fr.rand32();
                        break;
                    }
                    case 1: {
                        auto which = fr.randrange(tx.vin.size());
                        tx.vin[which].scriptWitness.stack.push_back(fr.randbytes(500));
                        break;
                    }
                    case 2: {
                        // Mutate a scriptSig
                        txc.vin[0].scriptSig.push_back('x');
                        hash_will_change = true;
                        break;
                    }
                    case 3: {
                        // Mutate a sequence
                        do {
                            txc.vin.back().nSequence = fr.rand32();
                        } while (txc.vin.back().nSequence == tx.vin.back().nSequence);
                        hash_will_change = true;
                        break;
                    }
                    case 4: {
                        // Mutate nVersion
                        do {
                            txc.nVersion = static_cast<int32_t>(fr.rand32());
                        } while (txc.nVersion == tx.nVersion);
                        hash_will_change = true;
                        break;
                    }
                    case 5: {
                        // Mutate output amount
                        auto which = fr.randrange(tx.vout.size());
                        txc.vout[which].nValue += 1;
                        hash_will_change = true;
                        break;
                    }
                    case 6: {
                        // Mutate output script
                        auto which = fr.randrange(tx.vout.size());
                        txc.vout[which].scriptPubKey.push_back('x');
                        hash_will_change = true;
                        break;
                    }
                    case 7: {
                        // Mutate nLockTime
                        do {
                            txc.nLockTime = fr.rand32();
                        } while (txc.nLockTime == tx.nLockTime);
                        hash_will_change = true;
                        break;
                    }
                    case 8: {
                        break;
                        // don't add and remove for a mutation otherwise it may end up valid
                        if (i % 2) {
                            txc.vout.emplace_back();
                            hash_will_change = true;
                        } else if (txc.vout.size()!=1) {
                            txc.vout.pop_back();
                            hash_will_change = true;
                        }
                        break;
                    }
                    case 9: {
                        break;
                        // don't add and remove for a mutation otherwise it may end up valid
                        if (i % 2) {
                            txc.vin.emplace_back();
                            hash_will_change = true;
                        } else if (txc.vin.size()!=1) {
                            txc.vin.pop_back();
                            hash_will_change = true;
                        }
                        break;
                    }
                    default:
                        assert(0);
                    }
                }
                PrecomputedTransactionData data_txc{txc};
                // iterate twice, one time with the correct spend indexes, one time with incorrect.
                for (auto use_random_index = 0; use_random_index < 2; ++use_random_index) {
                    hash_will_change |= use_random_index != 0;
                    for (size_t i = 0; i < hash.size(); ++i) {
                        uint32_t index{spend_index[i]};
                        while (use_random_index && index == spend_index[i]) {
                            index = fr.rand32();
                        }
                        uint256 sh = GetDefaultCheckTemplateVerifyHash(txc, data_txc.m_outputs_single_hash, data_txc.m_sequences_single_hash, index);
                        const bool hash_equals = sh == hash[i];
                        if (hash_will_change && hash_equals) {
                            BOOST_ERROR("Expected: NOT " << hash[i] << " Got: " << sh << " For:\n"
                                                         << strTest);
                        } else if (!hash_will_change && !hash_equals) {
                            BOOST_ERROR("Expected: " << hash[i] << " Got: " << sh << " For:\n"
                                                     << strTest);
                        }
                    }
                }
            }


        } else {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
