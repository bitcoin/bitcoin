#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "db.h"
#include "txdb.h"
#include "main.h"
#include "wallet.h"

CWallet* pwalletMain;
CClientUIInterface uiInterface;

extern bool fPrintToConsole;
extern void noui_connect();

struct TestingSetup {
    TestingSetup() {
        fPrintToDebugger = true; // don't want to write to debug.log file
        noui_connect();
        bitdb.MakeMock();  // Sets up BDB for wallets and addresses.
        MakeMockTXDB();    // Sets up the chosen txdb layer (bdb = noop, or leveldb)
        LoadBlockIndex();
        bool fFirstRun;
        pwalletMain = new CWallet("wallet.dat");
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

