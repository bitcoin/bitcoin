// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>

BOOST_FIXTURE_TEST_SUITE(asset_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(validation_tests)
{
	BOOST_CHECK(IsAssetUnitsValid(COIN));
	BOOST_CHECK(IsAssetUnitsValid(CENT));
}

BOOST_AUTO_TEST_SUITE_END()
