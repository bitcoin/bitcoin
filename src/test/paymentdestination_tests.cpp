// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <common/bip352.h>
#include <common/paymentdestination.h>
#include <key_io.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <variant>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(paymentdestination_tests, BasicTestingSetup)

static const std::string SP_ADDRESS{"sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68gdjvfn2"};
static const std::string P2WPKH_ADDRESS{"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"};

BOOST_AUTO_TEST_CASE(default_destination)
{
    const PaymentDestination dest;
    BOOST_CHECK(!dest.IsSilentPayment());
    BOOST_CHECK(!dest.GetSilentPaymentsDestination());
    BOOST_REQUIRE(dest.GetTxDestination());
    BOOST_CHECK(std::holds_alternative<CNoDestination>(*dest.GetTxDestination()));
    BOOST_CHECK(dest == PaymentDestination{CNoDestination{}});
}

BOOST_AUTO_TEST_CASE(from_string_silent_payments)
{
    const auto sp_dest{bip352::DecodeSilentPaymentsAddress(SP_ADDRESS, Params())};
    BOOST_REQUIRE(sp_dest.has_value());

    // Bech32m is case-insensitive, so an all-uppercase address parses to the same destination
    for (const auto& address : {SP_ADDRESS, ToUpper(SP_ADDRESS)}) {
        const auto dest{PaymentDestination::FromString(address)};
        BOOST_REQUIRE_MESSAGE(dest.has_value(), address);
        BOOST_CHECK(dest->IsSilentPayment());
        BOOST_CHECK(!dest->GetTxDestination());
        BOOST_REQUIRE(dest->GetSilentPaymentsDestination());
        BOOST_CHECK(*dest->GetSilentPaymentsDestination() == *sp_dest);
        // A silent payments destination has no fixed scriptPubKey
        BOOST_CHECK(!dest->GetStaticScript());
        BOOST_CHECK(*dest == PaymentDestination{*sp_dest});
        BOOST_CHECK_EQUAL(bip352::EncodeSilentPaymentsAddress(*dest->GetSilentPaymentsDestination(), Params()), SP_ADDRESS);
    }
}

BOOST_AUTO_TEST_CASE(from_string_tx_destination)
{
    const CTxDestination tx_dest{DecodeDestination(P2WPKH_ADDRESS)};
    BOOST_REQUIRE(IsValidDestination(tx_dest));

    const auto dest{PaymentDestination::FromString(P2WPKH_ADDRESS)};
    BOOST_REQUIRE(dest.has_value());
    BOOST_CHECK(!dest->IsSilentPayment());
    BOOST_CHECK(!dest->GetSilentPaymentsDestination());
    BOOST_REQUIRE(dest->GetTxDestination());
    BOOST_CHECK(*dest->GetTxDestination() == tx_dest);
    BOOST_REQUIRE(dest->GetStaticScript());
    BOOST_CHECK(*dest->GetStaticScript() == GetScriptForDestination(tx_dest));
    BOOST_CHECK(*dest == PaymentDestination{tx_dest});
    BOOST_CHECK(*dest != PaymentDestination{*bip352::DecodeSilentPaymentsAddress(SP_ADDRESS, Params())});
}

BOOST_AUTO_TEST_CASE(from_string_errors)
{
    struct InvalidVector {
        std::string address;
        std::string expected_error;
        std::vector<int> expected_error_locations;
    };
    const InvalidVector invalid_vectors[]{
        // Silent payments address with a bad checksum: the error is located
        {"sp1qq22l5s6l9460ww6t4tkzsy2a7zejurcmzz35pt0ffrzk5erlaykdcqugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68gdjvfn3",
         "Invalid Bech32m checksum", {115}},
        // Silent payments address with a valid checksum but an invalid scan pubkey
        {"sp1qqgqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq2qugecjjnjqf7ggq39vl6wexjlm00n66z94v675n7wcux6d2krr68g25havg",
         "Invalid Silent payments address", {}},
        // Silent payments address for another network
        {"tsp1qqthpye3hdcnydp9temp7yduy6uw5h2nw8u9fz677ccrna280qwj3uq60zeqs3zfpj3age62h4ljq2lyawwdecmk8a545yysk4x3tu3skjqm2thu6",
         "Invalid or unsupported Segwit (Bech32) or Base58 encoding.", {}},
        // Regular address with a bad checksum: the error is located
        {"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5", "Invalid Bech32 checksum", {41}},
    };

    for (const auto& vec : invalid_vectors) {
        std::vector<int> error_locations;
        const auto dest{PaymentDestination::FromString(vec.address, &error_locations)};
        BOOST_REQUIRE_MESSAGE(!dest.has_value(), vec.address);
        BOOST_CHECK_EQUAL(dest.error(), vec.expected_error);
        BOOST_CHECK_EQUAL_COLLECTIONS(error_locations.begin(), error_locations.end(),
                                      vec.expected_error_locations.begin(), vec.expected_error_locations.end());
        // error_locations is optional
        BOOST_CHECK(!PaymentDestination::FromString(vec.address).has_value());
    }
}

BOOST_AUTO_TEST_SUITE_END()
