// Copyright (c) 2012-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <scheduler.h>
#include <validation.h>
#include <validationinterface.h>

#include <test/test_bitcoin.h>

#include <boost/thread.hpp>
#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, BasicTestingSetup)

/**
 * Ensure that calls known to pose risk of deadlock cannot be made from within
 * ValidationInterface threads.
 */
BOOST_AUTO_TEST_CASE(no_deadlocks_in_queue)
{
    CScheduler scheduler;
    CMainSignals main_signals;
    const CChainParams& chainparams = Params();

    boost::thread t = boost::thread(std::bind(&CScheduler::serviceQueue, &scheduler));
    main_signals.RegisterBackgroundSignalScheduler(scheduler);

    bool callback_executed = false;

    main_signals.AddToProcessQueue([chainparams, &callback_executed]() {
        CValidationState state;
#ifdef DEBUG_LOCKORDER
        BOOST_CHECK_THROW(ActivateBestChain(state, chainparams), std::logic_error);
#else
        ActivateBestChain(state, chainparams);
#endif
        callback_executed = true;
    });

    scheduler.stop(true);
    t.join();
    assert(callback_executed);
}


BOOST_AUTO_TEST_SUITE_END()
