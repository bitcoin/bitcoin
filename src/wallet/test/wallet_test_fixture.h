// Copyright (c) 2016 The Flow Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FLOW_WALLET_TEST_FIXTURE_H
#define FLOW_WALLET_TEST_FIXTURE_H

#include "test/test_flow.h"

/** Testing setup and teardown for wallet.
 */
struct WalletTestingSetup: public TestingSetup {
    WalletTestingSetup(const std::string& chainName = CBaseChainParams::MAIN);
    ~WalletTestingSetup();
};

#endif

