#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "../main.h"
#include "../wallet.h"

#include "uint160_tests.cpp"
#include "uint256_tests.cpp"
#include "script_tests.cpp"
#include "transaction_tests.cpp"
#include "DoS_tests.cpp"
#include "base64_tests.cpp"
#include "util_tests.cpp"
#include "base58_tests.cpp"
#include "miner_tests.cpp"

CWallet* pwalletMain;

void Shutdown(void* parg)
{
  exit(0);
}
