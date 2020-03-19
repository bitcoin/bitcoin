// Copyright (c) 2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_dash.h>

#include <amount.h>
#include <privatesend/privatesend.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(privatesend_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ps_collatoral_tests)
{
    // Good collateral values
    BOOST_CHECK(CPrivateSend::IsCollateralAmount(0.00010000 * COIN));
    BOOST_CHECK(CPrivateSend::IsCollateralAmount(0.00012345 * COIN));
    BOOST_CHECK(CPrivateSend::IsCollateralAmount(0.00032123 * COIN));
    BOOST_CHECK(CPrivateSend::IsCollateralAmount(0.00019000 * COIN));

    // Bad collateral values
    BOOST_CHECK(!CPrivateSend::IsCollateralAmount(0.00009999 * COIN));
    BOOST_CHECK(!CPrivateSend::IsCollateralAmount(0.00040001 * COIN));
    BOOST_CHECK(!CPrivateSend::IsCollateralAmount(0.00100000 * COIN));
    BOOST_CHECK(!CPrivateSend::IsCollateralAmount(0.00100001 * COIN));
}

BOOST_AUTO_TEST_SUITE_END()
