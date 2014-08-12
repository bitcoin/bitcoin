// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Bitcoin Test Suite
#include "test_bitcoin.h"
#include <boost/test/unit_test.hpp>

TestingSetupManager testingSetupManager;

CClientUIInterface uiInterface;
CWallet* pwalletMain;

extern bool fPrintToConsole;
extern void noui_connect();

bool keepTestEvidence = false;
    
	void TestingSetupManager::SetupGenesisBlockChain()
	{
		if (chainActive.Tip()->GetBlockHash()!=Params().HashGenesisBlock()) {
			DestroyBlockChain();
			CreateBlockChain();
		}
	
	}
	
	void TestingSetupManager::CreateBlockChain()
	{
	#ifdef ENABLE_WALLET
        bitdb.MakeMock();
	#endif
		fReopenDebugLog =true; // Important when switching between two test block-chains, to move the debug.log to the new location
	    pathTemp = GetTempPath() / strprintf("test_bitcoin_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
        boost::filesystem::create_directories(pathTemp);
        mapArgs["-datadir"] = pathTemp.string();
        pblocktree = new CBlockTreeDB(1 << 20, true);
        pcoinsdbview = new CCoinsViewDB(1 << 23, true);
        pcoinsTip = new CCoinsViewCache(*pcoinsdbview);
        InitBlockIndex();
		#ifdef ENABLE_WALLET
        bool fFirstRun;
        pwalletMain = new CWallet("wallet.dat");
        pwalletMain->LoadWallet(fFirstRun);
        RegisterWallet(pwalletMain);
		#endif
	}
	
	void ResetBlockIndex();
	
	void TestingSetupManager::DestroyBlockChain()
	{
		// Clear the mempool so there are no transactions with missing inputs left.
		mempool.clear();
		
	#ifdef ENABLE_WALLET
	    UnregisterWallet(pwalletMain);
        delete pwalletMain;
        pwalletMain = NULL;
	#endif
	 
		UnloadBlockIndex();
		ResetBlockIndex();
	
	#ifdef ENABLE_WALLET
        bitdb.Flush(true);
	#endif
	
	    delete pcoinsTip;
        delete pcoinsdbview;
        delete pblocktree;		
		ClearDatadirCache();
		
		if (!keepTestEvidence)
			boost::filesystem::remove_all(pathTemp);
		
	}
	
    void TestingSetupManager::Start() {
        //fPrintToDebugLog = false; // don't want to write to debug.log file
        SelectParams(CBaseChainParams::MAIN);
        noui_connect();

        CreateBlockChain();

        nScriptCheckThreads = 3;
        for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
        RegisterNodeSignals(GetNodeSignals());		
    }
	
    void TestingSetupManager::Stop()
    {
        threadGroup.interrupt_all();
        threadGroup.join_all();
        UnregisterNodeSignals(GetNodeSignals());

        DestroyBlockChain();
    }
	
	TestingSetupManager::TestingSetupManager() 
	{
        keepTestEvidence =false;
    }


struct TestingSetup {
    
    TestingSetup() 
	{
        testingSetupManager.Start();
    }
	
    ~TestingSetup()
    {
        testingSetupManager.Stop();
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
