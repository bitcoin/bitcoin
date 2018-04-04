// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ravenscroll/ravenscroll.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ravenscroll_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ravenscroll_test1)
{
  RavenScrollPaymentRef paymentRef = RavenScrollPaymentRefRandom();
  BOOST_CHECK(paymentRef > 0);
}

BOOST_AUTO_TEST_SUITE_END()
