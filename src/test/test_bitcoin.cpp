#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"

extern bool fPrintToConsole;
struct TestingSetup {
    TestingSetup() {
        fPrintToConsole = true; // don't want to write to debug.log file
    }
    ~TestingSetup() { }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

CWallet* pwalletMain;

void Shutdown(void* parg)
{
  exit(0);
}
