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

template<class T>
void TestTxHelper(const CMutableTransaction& tx, bool expected_failure, const std::string& expected_error)
{
    T payload;

    const bool payload_to_fail = expected_failure && expected_error == "gettxpayload-fail";
    BOOST_CHECK_EQUAL(GetTxPayload(tx, payload, false), !payload_to_fail);

    // No need to check anything else if GetTxPayload() expected to fail
    if (payload_to_fail) return;

    TxValidationState dummy_state;
    BOOST_CHECK_EQUAL(payload.IsTriviallyValid(!bls::bls_legacy_scheme.load(), dummy_state), !expected_failure);
    if (expected_failure) {
        BOOST_CHECK_EQUAL(dummy_state.GetRejectReason(), expected_error);
    }
}

void trivialvalidation_runner(const std::string& json)
{
    const UniValue vectors = read_json(json);

    for (size_t idx = 1; idx < vectors.size(); idx++) {
        UniValue test = vectors[idx];
        uint256 txHash;
        std::string txType;
        CMutableTransaction tx;
        bool expected;
        std::string expected_err;
        try {
            // Additional data
            txHash = uint256S(test[0].get_str());
            BOOST_TEST_MESSAGE("tx: " << test[0].get_str());
            txType = test[1].get_str();
            // Raw transaction
            CDataStream stream(ParseHex(test[2].get_str()), SER_NETWORK, PROTOCOL_VERSION);
            stream >> tx;
            expected = test.size() > 3;
            if (expected) {
                expected_err = test[3].get_str();
            }
            // Sanity check
            BOOST_CHECK_EQUAL(tx.nVersion, 3);
            BOOST_CHECK_EQUAL(tx.GetHash(), txHash);
            // Deserialization based on transaction nType
            TxValidationState dummy_state;
            switch (tx.nType) {
            case TRANSACTION_PROVIDER_REGISTER: {
                BOOST_CHECK_EQUAL(txType, "proregtx");

                TestTxHelper<CProRegTx>(tx, expected, expected_err);
                break;
            }
            case TRANSACTION_PROVIDER_UPDATE_SERVICE: {
                BOOST_CHECK_EQUAL(txType, "proupservtx");

                TestTxHelper<CProUpServTx>(tx, expected, expected_err);
                break;
            }
            case TRANSACTION_PROVIDER_UPDATE_REGISTRAR: {
                BOOST_CHECK_EQUAL(txType, "proupregtx");

                TestTxHelper<CProUpRegTx>(tx, expected, expected_err);
                break;
            }
            case TRANSACTION_PROVIDER_UPDATE_REVOKE: {
                BOOST_CHECK_EQUAL(txType, "prouprevtx");

                TestTxHelper<CProUpRevTx>(tx, expected, expected_err);
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

BOOST_AUTO_TEST_CASE(trivialvalidation_valid)
{
    //TODO: Provide raw data for basic scheme as well
    bls::bls_legacy_scheme.store(true);

    const std::string json(json_tests::trivially_valid, json_tests::trivially_valid + sizeof(json_tests::trivially_valid));
    trivialvalidation_runner(json);
}

BOOST_AUTO_TEST_CASE(trivialvalidation_invalid)
{
    //TODO: Provide raw data for basic scheme as well
    bls::bls_legacy_scheme.store(true);

    const std::string json(json_tests::trivially_invalid, json_tests::trivially_invalid + sizeof(json_tests::trivially_invalid));
    trivialvalidation_runner(json);
}

BOOST_AUTO_TEST_SUITE_END()
