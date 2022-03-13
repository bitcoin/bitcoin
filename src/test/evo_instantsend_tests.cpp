// Copyright (c) 2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <llmq/instantsend.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_instantsend_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(instantsend_CInstantSendLock_tests)
{

    enum {
        DET = true,
        NON_DET = false
    };

    {
        llmq::CInstantSendLock lock;
        BOOST_CHECK_EQUAL(lock.IsDeterministic(), NON_DET);
    }

    std::vector<std::pair<int, bool>> version_det_pair = {
            {llmq::CInstantSendLock::isdlock_version, DET},
            {llmq::CInstantSendLock::islock_version, NON_DET},
            {2, DET},
            {100, DET},
            {std::numeric_limits<decltype(llmq::CInstantSendLock::isdlock_version)>::max(), DET},
    };

    for (const auto& [version, det] : version_det_pair) {
        llmq::CInstantSendLock lock(version);
        BOOST_CHECK_EQUAL(lock.IsDeterministic(), det);
    }
}

BOOST_AUTO_TEST_SUITE_END()
