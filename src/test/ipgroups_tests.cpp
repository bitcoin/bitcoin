// Copyright (c) 2015- The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test_suite.hpp>
#include <ipgroups.h>
#include <boost/test/test_tools.hpp>

BOOST_AUTO_TEST_SUITE(ipgroups_tests);

BOOST_AUTO_TEST_CASE(ipgroup)
{
    InitIPGroups();
    BOOST_CHECK(FindGroupForIP(CNetAddr("0.0.0.0")) == NULL);
    CIPGroup *group = FindGroupForIP(CNetAddr("0.1.2.3"));
    BOOST_CHECK_EQUAL(group->name, "tor");

    // If/when we have the ability to load labelled subnets from a file or persisted datastore, test that here.
}

BOOST_AUTO_TEST_SUITE_END()

