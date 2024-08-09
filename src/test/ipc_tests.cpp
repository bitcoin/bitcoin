// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/ipc_test.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ipc_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(ipc_tests)
{
    IpcTest();
}
BOOST_AUTO_TEST_SUITE_END()
