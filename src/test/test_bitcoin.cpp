#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE uint160
#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"

CWallet* pwalletMain;

void Shutdown(void* parg)
{
	exit(0);
}
