// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_core.h"
#include "bitcoin_main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bitcoin_main_tests)

BOOST_AUTO_TEST_CASE(claim_bounds_test)
{
	Bitcoin_CClaimCoinsViewCache claim_inputs1(*bitcoin_pclaimCoinsTip, 1000, true);

	claim_inputs1.SetTotalClaimedCoins(BITCREDIT_MAX_BITCOIN_CLAIM);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 500000));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 1, 500000));

	claim_inputs1.SetTotalClaimedCoins(0);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 1));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 1000, 1));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 45 * COIN, 1));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 45 * COIN + 1, 1));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 100 * COIN, 1));

	claim_inputs1.SetTotalClaimedCoins(0);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 2));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 1000, 2));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 90 * COIN, 2));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 90 * COIN + 1, 2));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 200 * COIN, 2));

	int64_t allowedLimit= (210000 * 50 * 9) /10;
	claim_inputs1.SetTotalClaimedCoins(0);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 210000));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN, 210000));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN  + 1 , 210000));

	allowedLimit= (210000 * 50 * 9) /10;
	claim_inputs1.SetTotalClaimedCoins(500 * COIN);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 210000));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN - 500 * COIN, 210000));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN - 500 * COIN  + 1 , 210000));

	allowedLimit= ((210000 * 50 + 210000 * 25) * 9) /10;
	claim_inputs1.SetTotalClaimedCoins(0);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 420000));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN, 420000));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN  + 1 , 420000));

	allowedLimit= ((210000 * 50 + 210000 * 25) * 9) /10;
	claim_inputs1.SetTotalClaimedCoins(1050000 * COIN);
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, 0, 420000));
	BOOST_CHECK(Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN - 1050000 * COIN, 420000));
	BOOST_CHECK(!Bitcredit_CheckClaimsAreInBounds(claim_inputs1, allowedLimit * COIN - 1050000 * COIN + 1 , 420000));


}

BOOST_AUTO_TEST_CASE(bitcoin_gettotalmonetarybase)
{
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(0) == uint64_t(0));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(1) == uint64_t(50 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(2) == uint64_t(100 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(209999) == uint64_t(10499950 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(210000) == uint64_t(10500000 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(210001) == uint64_t(10500025 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(419999) == uint64_t(15749975 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(420000) == uint64_t(15750000 * COIN));
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(420001) == uint64_t(157500125 * COIN) / 10);
	BOOST_CHECK(Bitcoin_GetTotalMonetaryBase(850003) == uint64_t(50 * COIN) * 210000 + int64_t(25 * COIN) * 210000 + int64_t((125 * COIN) / 10) * 210000  + int64_t((625 * COIN)/100) * 210000  + int64_t((3125 * COIN)/1000) * 10003 );
}


BOOST_AUTO_TEST_SUITE_END()
