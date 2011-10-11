#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"

CWallet* pwalletMain;

void Shutdown(void* parg)
{
  exit(0);
}
