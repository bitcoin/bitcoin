#define BOOST_TEST_MODULE Gameunits Test Suite
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "state.h"
#include "db.h"
#include "main.h"
#include "wallet.h"


CWallet* pwalletMain;
CClientUIInterface uiInterface;

extern bool fPrintToConsole;
extern void noui_connect();

boost::filesystem::path pathTemp;

struct TestingSetup {
    TestingSetup() {
        fPrintToDebugLog = false; // don't want to write to debug.log file
        noui_connect();
        bitdb.MakeMock();
        pathTemp = GetTempPath() / strprintf("test_bitcoin_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
        boost::filesystem::create_directories(pathTemp);
        mapArgs["-datadir"] = pathTemp.string();
        LoadBlockIndex(true);
        bool fFirstRun;
        pwalletMain = new CWallet("walletUT.dat");
        pwalletMain->LoadWallet(fFirstRun);
        RegisterWallet(pwalletMain);
    }
    ~TestingSetup()
    {
        delete pwalletMain;
        pwalletMain = NULL;
        bitdb.Flush(true);
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

