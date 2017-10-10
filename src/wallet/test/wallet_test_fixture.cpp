// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/test/wallet_test_fixture.h"

#include "rpc/server.h"
#include "wallet/db.h"
#include "wallet/wallet.h"

CWallet *pwalletMain;

WalletTestingSetup::WalletTestingSetup(const std::string& chainName):
    TestingSetup(chainName)
{
    bitdb.MakeMock();

    bool fFirstRun;
    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, "wallet_test.dat"));
    pwalletMain = new CWallet(std::move(dbw));
    pwalletMain->LoadWallet(fFirstRun);
    RegisterValidationInterface(pwalletMain);

    RegisterWalletRPCCommands(tableRPC);
}

WalletTestingSetup::~WalletTestingSetup()
{
    UnregisterValidationInterface(pwalletMain);
    delete pwalletMain;
    pwalletMain = nullptr;

    bitdb.Flush(true);
    bitdb.Reset();
}
