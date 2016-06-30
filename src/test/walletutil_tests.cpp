#include "main.h"
#include "test/test_bitcoin.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/fstream.hpp>

#ifdef ENABLE_WALLET
#include "wallet/db.h"
#include "wallet/wallet.h"
#endif

using namespace std;

BOOST_FIXTURE_TEST_SUITE(walletutil_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(walletutil_test)
{
	/*
	 * addresses and private keys in test/data/wallet.dat
	 */
	string expected_addr = "[ \"13EngsxkRi7SJPPqCyJsKf34U8FoX9E9Av\", \"1FKCLGTpPeYBUqfNxktck8k5nqxB8sjim8\", \"13cdtE9tnNeXCZJ8KQ5WELgEmLSBLnr48F\" ]\n";
	string expected_addr_pkeys = "[ {\"addr\" : \"13EngsxkRi7SJPPqCyJsKf34U8FoX9E9Av\", \"pkey\" : \"5Jz5BWE2WQxp1hGqDZeisQFV1mRFR2AVBAgiXCbNcZyXNjD9aUd\"}, {\"addr\" : \"1FKCLGTpPeYBUqfNxktck8k5nqxB8sjim8\", \"pkey\" : \"5HsX2b3v2GjngYQ5ZM4mLp2b2apw6aMNVaPELV1YmpiYR1S4jzc\"}, {\"addr\" : \"13cdtE9tnNeXCZJ8KQ5WELgEmLSBLnr48F\", \"pkey\" : \"5KCWAs1wX2ESiL4PfDR8XYVSSETHFd2jaRGxt1QdanBFTit4XcH\"} ]\n";

#ifdef WIN32
	string strCmd = "wallet-utility -datadir=test/data/ > test/data/op.txt";
#else
	string strCmd = "./wallet-utility -datadir=test/data/ > test/data/op.txt";
#endif
	int ret = system(strCmd.c_str());
	BOOST_CHECK(ret == 0);

	boost::filesystem::path opPath = "test/data/op.txt";
	boost::filesystem::ifstream fIn;
	fIn.open(opPath, std::ios::in);

	if (!fIn)
	{
		std::cerr << "Could not open the output file" << std::endl;
	}

	stringstream ss_addr;
	ss_addr << fIn.rdbuf();
	fIn.close();
	boost::filesystem::remove(opPath);

	string obtained = ss_addr.str();
	BOOST_CHECK_EQUAL(obtained, expected_addr);

#ifdef WIN32
	strCmd = "wallet-utility -datadir=test/data/ -dumppass > test/data/op.txt";
#else
	strCmd = "./wallet-utility -datadir=test/data/ -dumppass > test/data/op.txt";
#endif

	ret = system(strCmd.c_str());
	BOOST_CHECK(ret == 0);

	fIn.open(opPath, std::ios::in);

	if (!fIn)
	{
		std::cerr << "Could not open the output file" << std::endl;
	}

	stringstream ss_addr_pkeys;
	ss_addr_pkeys << fIn.rdbuf();
	fIn.close();
	boost::filesystem::remove(opPath);

	obtained = ss_addr_pkeys.str();
	BOOST_CHECK_EQUAL(obtained, expected_addr_pkeys);
}

BOOST_AUTO_TEST_SUITE_END()
