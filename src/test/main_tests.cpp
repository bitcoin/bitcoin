// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
	vector<SubsidyLevel> subsidyLevels = Bitcredit_Params().getSubsidyLevels();

	//Test with full deposit
    uint64_t nSum = 0;
    for (int nHeight = 0; nHeight <= 4300006; nHeight++) {
    	uint64_t nSubsidy = Bitcredit_GetAllowedBlockSubsidy(nSum, 2000 * COIN, nSum);
        nSum += nSubsidy;

        BOOST_CHECK(Credits_MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 3000000000000000ULL);

	//Test with full deposit
    nSum = 0;
    for (int nHeight = 0; nHeight <= 4300000; nHeight += 10000) {
    	uint64_t nSubsidy = Bitcredit_GetAllowedBlockSubsidy(nSum, 2000 * COIN, nSum);
        BOOST_CHECK(nSubsidy <= subsidyLevels[nHeight/10000/2].nSubsidyUpdateTo);
        nSum += nSubsidy * 10000;

        BOOST_CHECK(Credits_MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 3000000000000000ULL);

    //Test with half deposit
    nSum = 0;
    for (int nHeight = 0; nHeight <= 4300000; nHeight++) {
    	uint64_t halfRequiredDeposit = nSum / (2 * 15000 * 2);
    	int64_t nSubsidy = Bitcredit_GetAllowedBlockSubsidy(nSum, halfRequiredDeposit, nSum);
    	//Special case for first loop
    	if(nHeight == 0) {
    		nSubsidy = 2000000000;
    	}
    	if(nSum <= BITCREDIT_ENFORCE_SUBSIDY_REDUCTION_AFTER) {
    		BOOST_CHECK(nSubsidy <= Bitcredit_GetMaxBlockSubsidy(nSum));
    	} else {
    		BOOST_CHECK(nSubsidy <= Bitcredit_GetMaxBlockSubsidy(nSum) / 4);
    	}
        nSum += nSubsidy;

        BOOST_CHECK(Credits_MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 2992237038460326ULL);

    //Test with quarter deposit
    nSum = 0;
    for (int nHeight = 0; nHeight <= 4300000; nHeight++) {
    	uint64_t quarterRequiredDeposit = nSum / (2 * 15000 * 4);
    	uint64_t nSubsidy = Bitcredit_GetAllowedBlockSubsidy(nSum, quarterRequiredDeposit, nSum);
    	//Special case for first loop
    	if(nHeight == 0) {
    		nSubsidy = 1000000000;
    	}
    	if(nSum <= BITCREDIT_ENFORCE_SUBSIDY_REDUCTION_AFTER) {
    		BOOST_CHECK(nSubsidy <= Bitcredit_GetMaxBlockSubsidy(nSum));
    	} else {
    		BOOST_CHECK(nSubsidy <= Bitcredit_GetMaxBlockSubsidy(nSum) / 16);
    	}
        nSum += nSubsidy;

        BOOST_CHECK(Credits_MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 2592251022607323ULL);
}


BOOST_AUTO_TEST_CASE(bitcredit_reducebyreqdepositlevel)
{
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(0 * COIN), int64_t(0 * COIN)).GetLow64(), int64_t(10));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(1000 * COIN), int64_t(30000000 * COIN)).GetLow64(), int64_t(10));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(500 * COIN), int64_t(30000000 * COIN)).GetLow64(), int64_t(2));

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), int64_t(10));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/59000), int64_t(1 * COIN)).GetLow64(), int64_t(5));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), int64_t(4));

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t((9000000*COIN)/30000), int64_t(9000000 * COIN)).GetLow64(), int64_t(10));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(20), int64_t((9000000*COIN)/59000), int64_t(9000000 * COIN)).GetLow64(), int64_t(5));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t((9000000*COIN)/60000), int64_t(9000000 * COIN)).GetLow64(), int64_t(2));

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), int64_t(100*COIN));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/31000), int64_t(1 * COIN)).GetLow64(), 9675967596ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), 4998499849ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), int64_t(30000000*COIN));
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/31000), int64_t(1 * COIN)).GetLow64(), 2902790279027902ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), 1499549954995499ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/(30000*10)), int64_t(1 * COIN)).GetLow64(), 999099909ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/(30000*100)), int64_t(1 * COIN)).GetLow64(), 99009900ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/(30000*1000)), int64_t(1 * COIN)).GetLow64(), 9000900ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(0 * COIN), int64_t(0 * COIN)).GetLow64(), 10ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(1000 * COIN), int64_t(30000000 * COIN)).GetLow64(), 10ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(500 * COIN), int64_t(30000000 * COIN)).GetLow64(), 2ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), 10ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/59000), int64_t(1 * COIN)).GetLow64(), 5ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(10), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), 4ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), 10000000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/31000), int64_t(1 * COIN)).GetLow64(), 9675967596ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), 4998499849ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/30000), int64_t(1 * COIN)).GetLow64(), 3000000000000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/31000), int64_t(1 * COIN)).GetLow64(), 2902790279027902ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(30000000*COIN), int64_t(COIN/60000), int64_t(1 * COIN)).GetLow64(), 1499549954995499ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1349999*COIN)/(30000*10)), int64_t(1349999 * COIN)).GetLow64(), 999999998ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1349999*COIN)/(30000*100)), int64_t(1349999 * COIN)).GetLow64(), 99999998ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1349999*COIN)/(30000*1000)), int64_t(1349999 * COIN)).GetLow64(), 9999998ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350000*COIN)/(30000*10)), int64_t(1350000 * COIN)).GetLow64(), 1000000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350000*COIN)/(30000*100)), int64_t(1350000 * COIN)).GetLow64(), 100000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350000*COIN)/(30000*1000)), int64_t(1350000 * COIN)).GetLow64(), 10000000ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350001*COIN)/(30000*10)), int64_t(1350001 * COIN)).GetLow64(), 999999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350001*COIN)/(30000*100)), int64_t(1350001 * COIN)).GetLow64(), 99999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((1350001*COIN)/(30000*1000)), int64_t(1350001 * COIN)).GetLow64(), 9999999ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((2000000*COIN)/(30000*10)), int64_t(2000000 * COIN)).GetLow64(), 999999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((2000000*COIN)/(30000*100)), int64_t(2000000 * COIN)).GetLow64(), 99999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((2000000*COIN)/(30000*1000)), int64_t(2000000 * COIN)).GetLow64(), 9999999ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((4000000*COIN)/(30000*10)), int64_t(4000000 * COIN)).GetLow64(), 999999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((4000000*COIN)/(30000*100)), int64_t(4000000 * COIN)).GetLow64(), 99999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((4000000*COIN)/(30000*1000)), int64_t(4000000 * COIN)).GetLow64(), 9999999ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((6000000*COIN)/(30000*10)), int64_t(6000000 * COIN)).GetLow64(), 1000000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((6000000*COIN)/(30000*100)), int64_t(6000000 * COIN)).GetLow64(), 100000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((6000000*COIN)/(30000*1000)), int64_t(6000000 * COIN)).GetLow64(), 10000000ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8000000*COIN)/(30000*10)), int64_t(8000000 * COIN)).GetLow64(), 999999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8000000*COIN)/(30000*100)), int64_t(8000000 * COIN)).GetLow64(), 99999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8000000*COIN)/(30000*1000)), int64_t(8000000 * COIN)).GetLow64(), 9999999ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8159999*COIN)/(30000*10)), int64_t(8159999 * COIN)).GetLow64(), 999999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8159999*COIN)/(30000*100)), int64_t(8159999 * COIN)).GetLow64(), 99999999ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8159999*COIN)/(30000*1000)), int64_t(8159999 * COIN)).GetLow64(), 9999999ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8160000*COIN)/(30000*10)), int64_t(8160000 * COIN)).GetLow64(), 100000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8160000*COIN)/(30000*100)), int64_t(8160000 * COIN)).GetLow64(), 1000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((8160000*COIN)/(30000*1000)), int64_t(8160000 * COIN)).GetLow64(), 10000ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((9000000*COIN)/(30000*10)), int64_t(9000000 * COIN)).GetLow64(), 100000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((9000000*COIN)/(30000*100)), int64_t(9000000 * COIN)).GetLow64(), 1000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((9000000*COIN)/(30000*1000)), int64_t(9000000 * COIN)).GetLow64(), 10000ULL);

	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((30000000*COIN)/(30000*10)), int64_t(30000000 * COIN)).GetLow64(), 100000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((30000000*COIN)/(30000*100)), int64_t(30000000 * COIN)).GetLow64(), 1000000ULL);
	BOOST_CHECK_EQUAL(Bitcredit_ReduceByReqDepositLevel(uint256(100*COIN), int64_t((30000000*COIN)/(30000*1000)), int64_t(30000000 * COIN)).GetLow64(), 10000ULL);

	uint256 tmp;
	tmp.SetCompact(0x1d00ffff, false, false);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(COIN/30000), int64_t(1 * COIN)) == tmp);
	//Approx tmp / 4 (a little more)
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t((9000000*COIN)/59000), int64_t(9000000*COIN)) == uint256("00000000422fd685ae0572bb807295d3dcd36ee7c4610e7232975c0beb61e772"));
	//Also approx tmp / 4 (a little less)
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t((9000000*COIN)/60000), int64_t(9000000*COIN)) == uint256("000000003fffc000000000000000000000000000000000000000000000000000"));

	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(1000*COIN), int64_t(30000000 * COIN)) == tmp);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(500*COIN), int64_t(30000000 * COIN)) == tmp / 4);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(10*COIN), int64_t(30000000 * COIN)) == tmp / 10000);

	tmp.SetCompact(0x207fffff, false, false);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(1000*COIN), int64_t(30000000 * COIN)) == tmp);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(500*COIN),  int64_t(30000000 * COIN)) == tmp / 4);
	BOOST_CHECK(Bitcredit_ReduceByReqDepositLevel(tmp, int64_t(10*COIN), int64_t(30000000 * COIN)) == tmp / 10000);
}

BOOST_AUTO_TEST_CASE(bitcredit_getallowedblocksubsidy)
{
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(0 * COIN), uint64_t(0 * COIN), uint64_t(0 * COIN)), uint64_t(40 * COIN));

	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(40 * COIN), uint64_t(40 * COIN), uint64_t(40 * COIN)), uint64_t(40 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(40 * COIN), uint64_t(20 * COIN), uint64_t(40 * COIN)), uint64_t(40 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(40 * COIN), uint64_t(40 * COIN) / 2*15000, uint64_t(40 * COIN)), uint64_t(40 * COIN));
	//The two values that appears after the test is what would happen if reduction of reward where done from the beginning and not from half the monetary base
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(40 * COIN), uint64_t(40 * COIN / (2*2*15000)), uint64_t(40 * COIN)), uint64_t(40*COIN)); //uint64_t(1999984999));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(40 * COIN), uint64_t(40 * COIN / (2*2*2*15000)), uint64_t(40 * COIN)), uint64_t(40*COIN)); //uint64_t(999992499));

	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN)), uint64_t(69 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN / (2*15000)), uint64_t(151 * 100000 * COIN)), uint64_t(69 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN / (2*2*15000)), uint64_t(151 * 100000 * COIN)), uint64_t(1724999999));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN / (10*2*15000)), uint64_t(151 * 100000 * COIN)), uint64_t(68999999));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(151 * 100000 * COIN / (2*16000)), uint64_t(151 * 100000 * COIN)), uint64_t(6064453125));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(151 * 100000 * COIN), uint64_t(0 * COIN), uint64_t(151 * 100000 * COIN)), uint64_t(0 * COIN));

	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(30 * 1000000 * COIN), uint64_t(30 * 1000000 * COIN), uint64_t(30 * 1000000 * COIN)), uint64_t(0 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(30 * 1000000 * COIN), uint64_t(30 * 1000000 * COIN / (2*2*15000)), uint64_t(30 * 1000000 * COIN)), uint64_t(0 * COIN));
	BOOST_CHECK_EQUAL(Bitcredit_GetAllowedBlockSubsidy(uint64_t(30 * 1000000 * COIN), uint64_t(0 * COIN), uint64_t(30 * 1000000 * COIN)), uint64_t(0 * COIN));
}


BOOST_AUTO_TEST_SUITE_END()
