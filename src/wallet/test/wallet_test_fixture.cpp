// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/test/wallet_test_fixture.h"

#include "rpc/server.h"
#include "wallet/db.h"
#include "wallet/wallet.h"

WalletTestingSetup::WalletTestingSetup(const std::string& chainName):
    TestingSetup(chainName)
{
    bitdb.MakeMock();

    bool fFirstRun;
    CWallets::addWallet(new CWallet("wallet_test.dat"));
    CWallets::defaultWallet()->LoadWallet(fFirstRun);
    RegisterValidationInterface(CWallets::defaultWallet());

    RegisterWalletRPCCommands(tableRPC);
}

WalletTestingSetup::~WalletTestingSetup()
{
    UnregisterValidationInterface(CWallets::defaultWallet());
    CWallets::destruct();

    bitdb.Flush(true);
    bitdb.Reset();
}
