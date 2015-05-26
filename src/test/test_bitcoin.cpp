// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Bitcoin Test Suite



#include "main.h"
#include "bitcoin_main.h"
#include "txdb.h"
#include "bitcoin_txdb.h"
#include "ui_interface.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "bitcoin_db.h"
#include "wallet.h"
#include "bitcoin_wallet.h"
#endif

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#ifdef ENABLE_WALLET
uint64_t bitcoin_nAccountingEntryNumber = 0;
//Bitcoin_CDBEnv bitcoin_bitdb;

uint64_t bitcredit_nAccountingEntryNumber = 0;
Credits_CDBEnv bitcredit_bitdb("credits_database", "credits_db.log");

uint64_t deposit_nAccountingEntryNumber = 0;
Credits_CDBEnv deposit_bitdb("deposit_database", "deposit_db.log");
#endif

Bitcoin_CWallet* bitcoin_pwalletMain;
Credits_CWallet* bitcredit_pwalletMain;
Credits_CWallet* deposit_pwalletMain;

extern void noui_connect();

struct TestingSetup {
    Credits_CCoinsViewDB *bitcredit_pcoinsdbview;
    Bitcoin_CCoinsViewDB *bitcoin_pcoinsdbview;
    boost::filesystem::path pathTemp;
    boost::thread_group threadGroup;

    TestingSetup() {
        fPrintToDebugLog = false; // don't want to write to debug.log file
        noui_connect();
#ifdef ENABLE_WALLET
        bitcoin_bitdb.MakeMock();
        bitcredit_bitdb.MakeMock();
        deposit_bitdb.MakeMock();
#endif
        pathTemp = GetTempPath() / strprintf("test_credits_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
        boost::filesystem::create_directories(pathTemp);
        mapArgs["-datadir"] = pathTemp.string();

        bitcredit_pblocktree = new Credits_CBlockTreeDB(1 << 20, true);
        bitcredit_pcoinsdbview = new Credits_CCoinsViewDB(1 << 23, true);
        credits_pcoinsTip = new Credits_CCoinsViewCache(*bitcredit_pcoinsdbview);

        bitcoin_pblocktree = new Bitcoin_CBlockTreeDB(1 << 20, true);
        bitcoin_pcoinsdbview = new Bitcoin_CCoinsViewDB(1 << 23, true);
        bitcoin_pcoinsTip = new Bitcoin_CCoinsViewCache(*bitcoin_pcoinsdbview);

        Bitcoin_InitBlockIndex();
        Credits_InitBlockIndex();

#ifdef ENABLE_WALLET
        bool fFirstRun;
        bitcoin_pwalletMain = new Bitcoin_CWallet("bitcoin_wallet.dat");
        bitcoin_pwalletMain->LoadWallet(fFirstRun);
        Bitcoin_RegisterWallet(bitcoin_pwalletMain);

        bitcredit_pwalletMain = new Credits_CWallet("credits_wallet.dat", &bitcredit_bitdb);
        bitcredit_pwalletMain->LoadWallet(fFirstRun, bitcredit_nAccountingEntryNumber);
        Bitcredit_RegisterWallet(bitcredit_pwalletMain);

        deposit_pwalletMain = new Credits_CWallet("deposit_wallet.dat", &deposit_bitdb);
        deposit_pwalletMain->LoadWallet(fFirstRun, deposit_nAccountingEntryNumber);
        Bitcredit_RegisterWallet(deposit_pwalletMain);
#endif
        bitcredit_nScriptCheckThreads = 3;
        for (int i=0; i < bitcredit_nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&Bitcredit_ThreadScriptCheck);
        Bitcredit_RegisterNodeSignals(Credits_NetParams()->GetNodeSignals());
    }
    ~TestingSetup()
    {
        threadGroup.interrupt_all();
        threadGroup.join_all();
        Bitcredit_UnregisterNodeSignals(Credits_NetParams()->GetNodeSignals());
#ifdef ENABLE_WALLET
        delete bitcoin_pwalletMain;
        bitcoin_pwalletMain = NULL;
        delete bitcredit_pwalletMain;
        bitcredit_pwalletMain = NULL;
        delete deposit_pwalletMain;
        deposit_pwalletMain = NULL;
#endif
        delete credits_pcoinsTip;
        delete bitcredit_pcoinsdbview;
        delete bitcredit_pblocktree;

        delete bitcoin_pcoinsTip;
        delete bitcoin_pcoinsdbview;
        delete bitcoin_pblocktree;
#ifdef ENABLE_WALLET
        bitcoin_bitdb.Flush(true);
        bitcredit_bitdb.Flush(true);
        deposit_bitdb.Flush(true);
#endif
        boost::filesystem::remove_all(pathTemp);
    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

void Shutdown(void* parg)
{
  exit(0);
}

void StartShutdown()
{
  exit(0);
}

bool ShutdownRequested()
{
  return false;
}

