// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <test/data/trivially_invalid.json.h>
#include <test/data/trivially_valid.json.h>

#include <evo/providertx.h>
#include <primitives/transaction.h>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(evo_trivialvalidation, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(trivialvalidation_valid)
{
    //TODO: Provide raw data for basic scheme as well
    bls::bls_legacy_scheme.store(true);
    UniValue vectors = read_json(
        std::string(json_tests::trivially_valid, json_tests::trivially_valid + sizeof(json_tests::trivially_valid))
    );

    for (uint8_t idx = 1; idx < vectors.size(); idx++) {
        UniValue test = vectors[idx];
        uint256 txHash;
        std::string txType;
        CMutableTransaction tx;
        try {
            // Additional data
            int32_t txHeight = test[0].get_int();
            txHash = uint256S(test[1].get_str());
            txType = test[2].get_str();
            // Raw transaction
            CDataStream stream(ParseHex(test[3].get_str()), SER_NETWORK, PROTOCOL_VERSION);
            stream >> tx;
            // Sanity check
            BOOST_CHECK(txHeight > 1);
            BOOST_CHECK_EQUAL(tx.nVersion, 3);
            BOOST_CHECK_EQUAL(tx.GetHash(), txHash);
            // Deserialization based on transaction nType
            TxValidationState dummy_state;
            switch (tx.nType) {
                case TRANSACTION_PROVIDER_REGISTER: {
                    BOOST_CHECK_EQUAL(txType, "proregtx");
                    CProRegTx ptx;
                    BOOST_CHECK(GetTxPayload(tx, ptx, false));
                    BOOST_CHECK(ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                    break;
                }
                case TRANSACTION_PROVIDER_UPDATE_SERVICE: {
                    BOOST_CHECK_EQUAL(txType, "proupservtx");
                    CProUpServTx ptx;
                    BOOST_CHECK(GetTxPayload(tx, ptx, false));
                    BOOST_CHECK(ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                    break;
                }
                case TRANSACTION_PROVIDER_UPDATE_REGISTRAR: {
                    BOOST_CHECK_EQUAL(txType, "proupregtx");
                    CProUpRegTx ptx;
                    BOOST_CHECK(GetTxPayload(tx, ptx, false));
                    BOOST_CHECK(ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                    break;
                }
                case TRANSACTION_PROVIDER_UPDATE_REVOKE: {
                    BOOST_CHECK_EQUAL(txType, "prouprevtx");
                    CProUpRevTx ptx;
                    BOOST_CHECK(GetTxPayload(tx, ptx, false));
                    BOOST_CHECK(ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                    break;
                }
                default:
                    // TRANSACTION_COINBASE and TRANSACTION_NORMAL
                    // are not subject to trivial validation checks
                    BOOST_CHECK(false);
            }
        } catch (...) {
            BOOST_ERROR("Bad test, couldn't deserialize data: " << test.write());
            continue;
        }
    }
}

BOOST_AUTO_TEST_CASE(trivialvalidation_invalid)
{
    //TODO: Provide raw data for basic scheme as well
    bls::bls_legacy_scheme.store(true);
    UniValue vectors = read_json(
        std::string(json_tests::trivially_invalid, json_tests::trivially_invalid + sizeof(json_tests::trivially_invalid))
    );

    for (uint8_t idx = 1; idx < vectors.size(); idx++) {
        UniValue test = vectors[idx];
        uint256 txHash;
        std::string txType;
        CMutableTransaction tx;
        try {
            // Additional data
            txHash = uint256S(test[0].get_str());
            txType = test[1].get_str();
            // Raw transaction
            CDataStream stream(ParseHex(test[2].get_str()), SER_NETWORK, PROTOCOL_VERSION);
            stream >> tx;
            // Sanity check
            BOOST_CHECK_EQUAL(tx.nVersion, 3);
            BOOST_CHECK_EQUAL(tx.GetHash(), txHash);
            // Deserialization based on transaction nType
            TxValidationState dummy_state;
            if (txType == "proregtx") {
                CProRegTx ptx;
                if (GetTxPayload(tx, ptx, false)) {
                    BOOST_CHECK(!ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                } else {
                    BOOST_CHECK(tx.nType != TRANSACTION_PROVIDER_REGISTER || ptx.nVersion == 0);
                }
            }
            else if (txType == "proupservtx") {
                CProUpServTx ptx;
                if (GetTxPayload(tx, ptx, false)) {
                    BOOST_CHECK(!ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                } else {
                    BOOST_CHECK(tx.nType != TRANSACTION_PROVIDER_UPDATE_SERVICE || ptx.nVersion == 0);
                }
            }
            else if (txType == "proupregtx") {
                CProUpRegTx ptx;
                if (GetTxPayload(tx, ptx, false)) {
                    BOOST_CHECK(!ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                }
                else {
                    BOOST_CHECK(tx.nType != TRANSACTION_PROVIDER_UPDATE_REGISTRAR || ptx.nVersion == 0);
                }
            }
            else if (txType == "prouprevtx") {
                CProUpRevTx ptx;
                if (GetTxPayload(tx, ptx, false)) {
                    BOOST_CHECK(!ptx.IsTriviallyValid(bls::bls_legacy_scheme.load(), dummy_state));
                } else {
                    BOOST_CHECK(tx.nType != TRANSACTION_PROVIDER_UPDATE_REVOKE || ptx.nVersion == 0);
                }
            }
            else {
                // TRANSACTION_COINBASE and TRANSACTION_NORMAL
                // are not subject to trivial validation checks
                BOOST_CHECK(false);
            }
        } catch (...) {
            BOOST_ERROR("Bad test, couldn't deserialize data: " << test.write());
            continue;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
