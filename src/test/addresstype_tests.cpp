// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <optional.h>

BOOST_FIXTURE_TEST_SUITE(addresstype_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(addresstype_test)
{
    // Check parsing
    BOOST_CHECK_EQUAL(ParseAddressType("invalid"), nullopt);
    BOOST_CHECK_EQUAL(ParseAddressType("legacy"), AddressType::BASE58);
    BOOST_CHECK_EQUAL(ParseAddressType("bech32"), AddressType::BECH32);

    // Check formatting
    BOOST_CHECK_EQUAL(FormatAddressType(AddressType::BASE58), "legacy");
    BOOST_CHECK_EQUAL(FormatAddressType(AddressType::BECH32), "bech32");
}

BOOST_AUTO_TEST_SUITE_END()
