#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"

CWallet* pwalletMain;

extern bool fPrintToConsole;
struct TestingSetup {
    TestingSetup() {
        fPrintToConsole = true; // don't want to write to debug.log file
        pwalletMain = new CWallet();
        pblockstore = new CBlockStore(true);
        // TODO: have to make sure util_tests are run before blockstore_tests to
        //   identify thread issues before they show up as blockstore issues
        CreateThread(ProcessCallbacks, pblockstore);
        //pwalletMain->RegisterWithBlockStore(pblockstore);
    }
    ~TestingSetup()
    {
        delete pwalletMain;
        pwalletMain = NULL;
    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

void Shutdown(void* parg)
{
  exit(0);
}
