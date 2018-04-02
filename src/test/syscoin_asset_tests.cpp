// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "asset.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <chrono>
#include "ranges.h"
using namespace boost::chrono;
using namespace std;
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

void printRangeVector (vector<CRange> &vecRange, string tag) {
	printf("Printing vector range %s: ", tag.c_str());
	for(int index = 0; index < vecRange.size(); index++) {
		printf("{%i,%i} ", vecRange[index].start, vecRange[index].end);
	}
	printf("\n");
}
void addToRangeVector (vector<CRange> &vecRange, int range_single) { 
	CRange range(range_single, range_single);
	vecRange.push_back(range);
}
void addToRangeVector (vector<CRange> &vecRange, int range_start, int range_end) { 
	CRange range(range_start, range_end);
	vecRange.push_back(range);
}

BOOST_FIXTURE_TEST_SUITE (syscoin_asset_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE(generate_range_merge)
{
	printf("Running generate_range_merge...\n");
	// start with {0,0} {2,3} {6,8}, add {4,5} to it and expect {0,0} {2,8}
	CheckRangeMerge("{0,0} {2,3} {6,8}", "{4,5}", "{0,0} {2,8}");

	CheckRangeMerge("{0,0} {2,3} {6,8}", "{4,5}", "{0,0} {2,8}");
	CheckRangeMerge("{2,3} {6,8}", "{0,0} {4,5}", "{0,0} {2,8}");
	CheckRangeMerge("{2,3}", "{0,0} {4,5} {6,8}", "{0,0} {2,8}");
	CheckRangeMerge("{0,0} {4,5} {6,8}", "{2,3}", "{0,0} {2,8}");

	CheckRangeMerge("{0,0} {2,2} {4,4} {6,6} {8,8}", "{1,1} {3,3} {5,5} {7,7} {9,9}", "{0,9}");
	CheckRangeMerge("{0,8}","{9,9}","{0,9}");
	CheckRangeMerge("{0,8}","{10,10}","{0,8} {10,10}");
	CheckRangeMerge("{0,0} {2,2} {4,4} {6,6} {8,8} {10,10} {12,12} {14,14} {16,16} {18,18} {20,20} {22,22} {24,24} {26,26} {28,28} {30,30} {32,32} {34,34} {36,36} {38,38} {40,40} {42,42} {44,44} {46,46} {48,48}", "{1,1} {3,3} {5,5} {7,7} {9,9} {11,11} {13,13} {15,15} {17,17} {19,19} {21,21} {23,23} {25,25} {27,27} {29,29} {31,31} {33,33} {35,35} {37,37} {39,39} {41,41} {43,43} {45,45} {47,47} {49,49}", "{0,49}");  

}
BOOST_AUTO_TEST_CASE(generate_range_subtract)
{
	printf("Running generate_range_subtract...\n");
	// start with {0,9}, subtract {0,0} {2,3} {6,8} from it and expect {1,1} {4,5} {9,9}
	CheckRangeSubtract("{0,9}", "{0,0} {2,3} {6,8}", "{1,1} {4,5} {9,9}");

	CheckRangeSubtract("{1,2} {3,3} {6,10}", "{0,0} {2,2} {3,3}", "{1,1} {6,10}");
	CheckRangeSubtract("{1,2} {3,3} {6,10}", "{0,0} {2,2}", "{1,1} {3,3} {6,10}");
}
BOOST_AUTO_TEST_CASE(generate_range_contain)
{
	printf("Running generate_range_contain...\n");
	// does {0,9} contain {0,0}?
	BOOST_CHECK(DoesRangeContain("{0,9}", "{0,0}"));
	BOOST_CHECK(DoesRangeContain("{0,2}", "{1,2}"));
	BOOST_CHECK(DoesRangeContain("{0,3}", "{2,2}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{2,2}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{2,3}"));
	BOOST_CHECK(DoesRangeContain("{0,0} {2,3} {6,8}", "{6,7}"));
	BOOST_CHECK(DoesRangeContain("{0,8}", "{0,0} {2,3} {6,8}"));
	BOOST_CHECK(DoesRangeContain("{0,8}", "{0,1} {2,4} {6,6}"));

	BOOST_CHECK(!DoesRangeContain("{1,9}", "{0,0}"));
	BOOST_CHECK(!DoesRangeContain("{1,9}", "{1,10}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{1,3}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{0,2}"));
	BOOST_CHECK(!DoesRangeContain("{1,2}", "{0,3}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{1,2}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{0,1}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{4,4}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{4,5}"));
	BOOST_CHECK(!DoesRangeContain("{0,0} {2,3} {6,8}", "{0,8}"));
	BOOST_CHECK(!DoesRangeContain("{0,8}", "{0,1} {2,4} {6,9}"));
	BOOST_CHECK(!DoesRangeContain("{0,8}", "{0,9} {2,4} {6,8}"));
}
BOOST_AUTO_TEST_CASE(generate_range_complex)
{
	/* Test 1:  Generate two large input, 1 all even number 1 all odd and merge them */
	/* This test uses Range Test Library that contains addition vector operations    */
	printf("Running generate_range_complex...\n");
	string input1="", input2="", expected_output="";
	int total_range = 10000;
	int64_t ms1 = 0, ms2 = 0;

	printf("ExpectedOutput: range from 0-%i\n", total_range);
	printf("Input1: range from 0-%i\n Even number only\n", total_range-2);
	printf("Input2: range from 1-%i\n Odd number only\n", total_range-1);

	for (int index = 0; index < total_range; index=index+2) {
		input1 = input1 + "{" + to_string(index) + "," + to_string(index) +"} ";
		input2 = input2 + "{" + to_string(index+1) + "," + to_string(index+1) +"} ";
	}
	expected_output = "{0," + to_string(total_range-1) + "}"; 
	// Remove the last space from the string
	input1.pop_back();
	input2.pop_back();
	printf("Rangemerge Test: input1 + input2 = ExpectedOutput\n");
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeMerge(input1, input2, expected_output);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeMerge Completed %ldms\n", ms2-ms1);

	/* Test 2: Reverse of Test 1 (expected_output - input = 2) */
 	printf("RangeSubstract Test: ExpectedOutput - input1 = input2\n");	
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeSubtract(expected_output, input1, input2);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeSubtract1 Completed %ldms\n", ms2-ms1);

 	printf("RangeSubstract Test: ExpectedOutput - input2 = input1\n");	
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	CheckRangeSubtract(expected_output, input2, input1);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("CheckRangeSubtract2 Completed %ldms\n", ms2-ms1);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_merge1) 
{
	// Test: merge1 
	// range1: {0-10m} even only
	// range2: {1-9999999} odd only
	// output:  range1 + range2
	printf("Running generate_range_stress_merge1:...\n");
	int total_range = 10000000;
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o, vecRange_expected;
	int64_t ms1 = 0, ms2 = 0;

	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index, index);
		addToRangeVector(vecRange2_i, index+1, index+1);
	}
	
	// Set expected outcome
	addToRangeVector(vecRange_expected, 0, total_range-1);
	
	// combine the two vectors of ranges
	vecRange1_i.insert(std::end(vecRange1_i), std::begin(vecRange2_i), std::end(vecRange2_i));

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	mergeRanges(vecRange1_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	printf("\noutput range 1+2: merge time: %ldms\n", ms2-ms1);

	BOOST_CHECK(vecRange_o.size() == vecRange_expected.size());
	BOOST_CHECK(vecRange_o.back() == vecRange_expected.back());
	printf("\n Stress Test done \n");

}
BOOST_AUTO_TEST_CASE(generate_range_stress_subtract1) 
{
	// Test: subtract1 
	// range1: {0-1m} 
	// range2: {1m, 4m ,7m}
	// output:  range1 - range2
	printf("Running generate_range_stress_subtract1...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o, vecRange_expected;
	vector<CRange> vecRange2_i_copy;
	int64_t ms1 = 0, ms2 = 0;

	
	// Set input range 1 {0,10m}
	addToRangeVector(vecRange1_i, 0, 10000000);

	printRangeVector(vecRange1_i, "vecRange 1 input");
	// Set input range 2 {1m,4m,7m}
	addToRangeVector(vecRange2_i, 1000000);
	addToRangeVector(vecRange2_i, 4000000);
	addToRangeVector(vecRange2_i, 7000000);
	printRangeVector(vecRange2_i, "vecRange 2 input");
	
	// Set expected output {(1-999999),(1000001-3999999)...}
	addToRangeVector(vecRange_expected, 0, 999999);
	addToRangeVector(vecRange_expected, 1000001, 3999999);
	addToRangeVector(vecRange_expected, 4000001, 6999999);
	addToRangeVector(vecRange_expected, 7000001, 10000000);
	printRangeVector(vecRange_expected, "vecRange_expected");
	
	// Deep copy for test #2 since the vector will get modified
	vecRange2_i_copy = vecRange2_i;
	

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	subtractRanges(vecRange1_i, vecRange2_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	BOOST_CHECK(vecRange_o.size() == vecRange_expected.size());
	BOOST_CHECK(vecRange_o.back() == vecRange_expected.back());

	vecRange2_i = vecRange2_i_copy;
	vector<CRange> vecRange2_o;
	vecRange2_i.insert(std::end(vecRange2_i), std::begin(vecRange_expected), std::end(vecRange_expected));
	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	mergeRanges(vecRange2_i, vecRange2_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	printf("\noutput range expected+2: merge time: %ld\n", ms2-ms1);

	BOOST_CHECK_EQUAL(vecRange2_o.size(), vecRange1_i.size());
	BOOST_CHECK_EQUAL(vecRange2_o.back().start, 0);
	BOOST_CHECK_EQUAL(vecRange2_o.back().end, 10000000);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_merge2) 
{
	// Test: merge2
	// range1: {0-1m} odd only
	// range2: {100000 200000, ..., 900000 }
	// output:  range1 + range2
	printf("Running generate_range_stress_merge2...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o;
	int64_t ms1 = 0, ms2 = 0;
	int total_range = 1000000;

	// Create vector range 1 that's 0-1mill odd only
	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index+1, index+1);
	}
	// Create vector range 2 that's 100k,200k,300k...,900k
	for (int index = 100000; index < total_range; index=index+100000) {
		addToRangeVector(vecRange2_i, index, index);
	}

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	vecRange1_i.insert(std::end(vecRange1_i), std::begin(vecRange2_i), std::end(vecRange2_i));
	mergeRanges(vecRange1_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	//HARDCODED checks
	BOOST_CHECK_EQUAL(vecRange_o.size(), (total_range/2) - 9);
	BOOST_CHECK_EQUAL(vecRange_o[49999].start, 99999); 
	BOOST_CHECK_EQUAL(vecRange_o[49999].end, 100001); 
	BOOST_CHECK_EQUAL(vecRange_o[99998].start, 199999); 
	BOOST_CHECK_EQUAL(vecRange_o[99998].end, 200001); 
	BOOST_CHECK_EQUAL(vecRange_o[449991].start, 899999); 
	BOOST_CHECK_EQUAL(vecRange_o[449991].end, 900001); 
	printf("CheckRangeSubtract Completed %ldms\n", ms2-ms1);
}
BOOST_AUTO_TEST_CASE(generate_range_stress_subtract2) 
{
	// Test: subtract2
	// range1: {0-1m} odd only
	// range2: {100001 200001, ..., 900001 }
	// output:  range1 - range2
	printf("Running generate_range_stress_subtract3...\n");
	vector<CRange> vecRange1_i, vecRange2_i, vecRange_o;
	int64_t ms1 = 0, ms2 = 0;
	int total_range = 1000000;

	// Create vector range 1 that's 0-1mill odd only
	for (int index = 0; index < total_range; index=index+2) {
		addToRangeVector(vecRange1_i, index+1,index+1);
	}
	// Create vector range 2 that's 100k,200k,300k...,900k
	for (int index = 100000; index < total_range; index=index+100000) {
		addToRangeVector(vecRange2_i, index+1,index+1);
	}

	ms1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	subtractRanges(vecRange1_i, vecRange2_i, vecRange_o);
	ms2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	//HARDCODED checks
	BOOST_CHECK_EQUAL(vecRange_o.size(), 499991);
	printf("CheckRangeSubtract Completed %ldms\n", ms2-ms1);
}

BOOST_AUTO_TEST_CASE(generate_big_assetdata)
{
	ECC_Start();
	GenerateSpendableCoins("node1");
	GenerateSpendableCoins("node2");
	GenerateSpendableCoins("node3");
	printf("Running generate_big_assetdata...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetbig1", "data");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 257 bytes long
	string baddata = gooddata + "a";
	AssetNew("node1", "newasset", "jagassetbig1", gooddata);
	//"assetnew [name] [alias] [public] [category=assets] [supply] [max_supply] [use_inputranges] [interest_rate] [can_adjust_interest_rate] [witness]\n"
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew newasset jagassetbig1 " + baddata + " assets 1 1 false 0 false ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest)
{
	UniValue r;
	printf("Running generate_asset_collect_interest...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollection", "data");
	AliasNew("node1", "jagassetcollectionreceiver", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually)
	AssetNew("node1", "newassetcollection", "jagassetcollection", "data", "10000", "-1", "false", "0.05");
	AssetSend("node1", "newassetcollection", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionreceiver\\\",\\\"amount\\\":5000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo newassetcollection jagassetcollectionreceiver false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());
	// 10 hours later
	GenerateBlocks(60 * 10);
	// calc interest expect 5000 (1 + 0.05 / 60) ^ (60(10)) = ~8248
	AssetClaimInterest("node1", "newassetcollection", "jagassetcollectionreceiver");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo newassetcollection jagassetcollectionreceiver false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 824875837095);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_average_balance)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_average_balance...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollectionavg", "data");
	AliasNew("node1", "jagassetcollectionrcveravg", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually)
	AssetNew("node1", "avgbalanceasset", "jagassetcollectionavg", "data", "10000", "-1", "false", "0.05");
	AssetSend("node1", "avgbalanceasset", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceasset jagassetcollectionrcveravg false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 1000 * COIN);
	int claimheight = find_value(r.get_obj(), "height").get_int();
	// 3 hours later send 1k more
	GenerateBlocks((60 * 3) - 1);

	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", "avgbalanceasset", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":3000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceasset jagassetcollectionrcveravg false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 4000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	// 2 hours later send 3k more
	GenerateBlocks((60 * 2) - 1);

	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", "avgbalanceasset", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravg\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceasset jagassetcollectionrcveravg false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);

	// 1 hour later send 1k more
	GenerateBlocks((60 * 1) - 1);

	// total interest (1000*180 + 4000*120 + 5000*60) / 360 = 2666.67 - average balance over 6hrs, calculate interest on that balance and apply it to 5k
	// formula is  ((averagebalance*pow((1 + ((double)asset.fInterestRate / 60)), (60*6)))) - averagebalance;
	//  ((2666.67*pow((1 + (0.05 / 60)), (60*6)))) - 2666.67 = 932.5 interest (total 5932.5 balance after interest)
	AssetClaimInterest("node1", "avgbalanceasset", "jagassetcollectionrcveravg");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceasset jagassetcollectionrcveravg false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 593250716124);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_update_with_average_balance)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_update_with_average_balance...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollectionavgu", "data");
	AliasNew("node1", "jagassetcollectionrcveravgu", "data");
	// setup asset with 5% interest hourly (unit test mode calculates interest hourly not annually), can adjust the rate
	AssetNew("node1", "avgbalanceassetu", "jagassetcollectionavgu", "data", "10000", "-1", "false", "0.05", "true");
	AssetSend("node1", "avgbalanceassetu", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceassetu jagassetcollectionrcveravgu false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 1000 * COIN);
	int claimheight = find_value(r.get_obj(), "height").get_int();
	// 3 hours later send 1k more
	GenerateBlocks((60 * 3)-11);
	// update interest rate to 10%
	AssetUpdate("node1", "avgbalanceassetu", "pub", "0", "0.1");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", "avgbalanceassetu", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":3000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceassetu jagassetcollectionrcveravgu false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 4000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	// 2 hours later send 3k more
	GenerateBlocks((60 * 2) - 11);

	// interest rate to back to 5%
	AssetUpdate("node1", "avgbalanceassetu", "pub", "0", "0.05");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);
	AssetSend("node1", "avgbalanceassetu", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionrcveravgu\\\",\\\"amount\\\":1000}]\"", "memoassetinterest");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceassetu jagassetcollectionrcveravgu false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), claimheight);

	// 1 hour later send 1k more
	GenerateBlocks((60 * 1) - 11);

	// at the end set rate to 50% but this shouldn't affect the result since we set this rate recently
	AssetUpdate("node1", "avgbalanceassetu", "pub", "0", "0.5");
	// total interest (1000*180 + 4000*120 + 5000*60) / 360 = 2666.67 - average balance over 6hrs, calculate interest on that balance and apply it to 5k
	// total interest rate (0.05*180 + 0.1*120 + 0.05*60) / 360 = 0.0667% - average interest over 6hrs
	// formula is  ((averagebalance*pow((1 + ((double)asset.fInterestRate / 60)), (60*6)))) - averagebalance;
	//  ((2666.67*pow((1 + (0.0667 / 60)), (60*6)))) - 2666.67 = 1310.65 interest (total about 6310.65 balance after interest)
	AssetClaimInterest("node1", "avgbalanceassetu", "jagassetcollectionrcveravgu");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo avgbalanceassetu jagassetcollectionrcveravgu false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 631064931803);
}
BOOST_AUTO_TEST_CASE(generate_asset_collect_interest_every_block)
{
	UniValue r;
	printf("Running generate_asset_collect_interest_every_block...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetcollection1", "data");
	AliasNew("node1", "jagassetcollectionreceiver1", "data");
	// setup asset with 10% interest hourly (unit test mode calculates interest hourly not annually)
	AssetNew("node1", "newassetcollection1", "jagassetcollection1", "data", "10000", "-1", "false", "0.05");
	AssetSend("node1", "newassetcollection1", "\"[{\\\"aliasto\\\":\\\"jagassetcollectionreceiver1\\\",\\\"amount\\\":5000}]\"", "memoassetinterest1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo newassetcollection1 jagassetcollectionreceiver1 false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 5000 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());
	// 10 hours later
	// calc interest expect 5000 (1 + 0.05 / 60) ^ (60(10)) = ~8248
	for (int i = 0; i <= 60*10; i+=25) {
		AssetClaimInterest("node1", "newassetcollection1", "jagassetcollectionreceiver1");
		GenerateBlocks(24);
		printf("Claiming interest %d of out %d...\n", i, 60 * 10);
	}
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo newassetcollection1 jagassetcollectionreceiver1 false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 824875837081);

}

BOOST_AUTO_TEST_CASE(generate_big_assetname)
{
	printf("Running generate_big_assetname...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetbig2", "data");
	// 20 bytes long
	string goodname = "12345678901234567890";
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 21 bytes long
	string badname = goodname + "1";
	AssetNew("node1", goodname, "jagassetbig2", gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew " + badname + " jagassetbig2 " + gooddata + " assets 1 1 false 0 false ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetupdate)
{
	printf("Running generate_assetupdate...\n");
	AliasNew("node1", "jagassetupdate", "data");
	AliasNew("node2", "jagassetupdate1", "data");
	AssetNew("node1", "assetupdatename", "jagassetupdate", "data");
	// update an asset that isn't yours
	UniValue r;
	//"assetupdate [asset] [public] [category=assets] [supply] [interest_rate] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetupdate assetupdatename jagassetupdate assets 1 0 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());

	AssetUpdate("node1", "assetupdatename", "pub1");
	// shouldnt update data, just uses prev data because it hasnt changed
	AssetUpdate("node1", "assetupdatename");
	// update supply, ensure balance gets updated properly, 5+1, 1 comes from the initial assetnew, 1 above doesn't actually get set because asset wasn't yours so total should be 6
	AssetUpdate("node1", "assetupdatename", "pub12", "5");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetupdatename false"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 6*COIN);
	// update interest rate
	AssetNew("node1", "assetupdateinterest", "jagassetupdate", "data", "1", "10", "false", "0.1", "true");
	AssetUpdate("node1", "assetupdateinterest", "pub12", "0", "0.25");
	// set can adjust rate to false and ensure can't update interest rate (use initial asset which has can adjust rate set to false)
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate assetupdatename jagassetupdate assets 1 0.11 ''"), runtime_error);
	// can't change supply > max supply (current balance already 6, max is 10)
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate assetupdatename jagassetupdate assets 5 0 ''"), runtime_error);
	// if max supply is -1 ensure supply can goto 10b max
	AssetNew("node1", "assetupdatemaxsupply", "jagassetupdate", "data", "1", "-1");
	string maxstr = ValueFromAmount(MAX_ASSET-COIN).write();
	AssetUpdate("node1", "assetupdatemaxsupply", "pub12", maxstr);
	// can't go above 10b max
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate assetupdatemaxsupply jagassetupdate assets 1 0 ''"), runtime_error);
	// can't create asset with more than 10b balance or max supply
	string maxstrplusone = ValueFromAmount(MAX_ASSET+1).write();
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew assetupdatename2 assetupdatename pub assets " + maxstrplusone + " -1 false 0 false ''"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew assetupdatename2 assetupdatename pub assets 1 " + maxstrplusone + " false 0 false ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetsend)
{
	UniValue r;
	printf("Running generate_assetsend...\n");
	AliasNew("node1", "jagassetsend", "data");
	AliasNew("node2", "jagassetsend1", "data");
	AssetNew("node1", "assetsendname", "jagassetsend", "data", "10", "20");
	// [{\"aliasto\":\"aliasname\",\"amount\":amount},...]
	AssetSend("node1", "assetsendname", "\"[{\\\"aliasto\\\":\\\"jagassetsend1\\\",\\\"amount\\\":7}]\"", "memoassetsend");
	// ensure amounts are correct
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendname true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 3 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 10 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo assetsendname jagassetsend1 true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 7 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memoassetsend");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	// add balances
	AssetUpdate("node1", "assetsendname", "pub12", "1");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendname true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 4 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 11 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	AssetUpdate("node1", "assetsendname", "pub12", "9");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendname true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 13 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 20 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK(inputsArray.size() == 0);
	// can't go over 20 supply
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate assetsendname jagassetsend assets 1 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetsend_ranges)
{
	UniValue r;
	printf("Running generate_assetsend_ranges...\n");
	AliasNew("node1", "jagassetsendranges", "data");
	AliasNew("node2", "jagassetsendranges1", "data");
	// if use input ranges update supply and ensure adds to end of allocation, ensure balance gets updated properly
	AssetNew("node1", "assetsendnameranges", "jagassetsendranges", "data", "10", "20", "true");
	// send range 1-2, 4-6, 8-9 and then add 1 balance and expect it to add to 10, add 9 more and expect it to add to 11, try to add one more and won't let you due to max 20 supply
	// [{\"aliasto\":\"aliasname\",\"ranges\":[{\"start\":index,\"end\":index},...]},...]
	// break ranges into 0, 3, 7
	AssetSend("node1", "assetsendnameranges", "\"[{\\\"aliasto\\\":\\\"jagassetsendranges1\\\",\\\"ranges\\\":[{\\\"start\\\":1,\\\"end\\\":2},{\\\"start\\\":4,\\\"end\\\":6},{\\\"start\\\":8,\\\"end\\\":9}]}]\"", "memoassetsendranges");
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo assetsendnameranges jagassetsendranges1 true"));
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 2);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 6);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 8);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 9);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 7 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memoassetsendranges");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "interest_claim_height").get_int(), find_value(r.get_obj(), "height").get_int());

	// ensure ranges are correct
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendnameranges true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 3 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 10 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	// add balances, expect to add to range 10 because total supply is 10 (0-9 range)
	AssetUpdate("node1", "assetsendnameranges", "pub12", "1");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendnameranges true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 4 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 11 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "start").get_int(), 10);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "end").get_int(), 10);
	AssetUpdate("node1", "assetsendnameranges", "pub12", "9");
	// check balance is added to end, last range expected 10-19
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo assetsendnameranges true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 13 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 20 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 20 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size() , 4);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "start").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[1].get_obj(), "end").get_int(), 3);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "start").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[2].get_obj(), "end").get_int(), 7);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "start").get_int(), 10);
	BOOST_CHECK_EQUAL(find_value(inputsArray[3].get_obj(), "end").get_int(), 19);
	// can't go over 20 supply
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetupdate assetsendnameranges jagassetsendranges assets 1 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_assetsend_ranges2)
{
	// create an asset, assetsend the initial allocation to myself, assetallocationsend the entire allocation to 5 other aliases, 5 other aliases send all those allocations back to me, then mint 1000 new tokens
	UniValue r;
	printf("Running generate_assetsend_ranges2...\n");
	AliasNew("node1", "jagassetsendrangesowner", "data");
	AliasNew("node1", "jagassetsendrangesownerallocation", "data");
	AliasNew("node2", "jagassetsendrangesa", "data");
	AliasNew("node2", "jagassetsendrangesb", "data");
	AliasNew("node2", "jagassetsendrangesc", "data");
	AliasNew("node2", "jagassetsendrangesd", "data");
	AliasNew("node2", "jagassetsendrangese", "data");
	// if use input ranges update supply and ensure adds to end of allocation, ensure balance gets updated properly
	AssetNew("node1", "ASSET", "jagassetsendrangesowner", "ASSET", "1000", "1000000", "true");
	AssetSend("node1", "ASSET", "\"[{\\\"aliasto\\\":\\\"jagassetsendrangesownerallocation\\\",\\\"ranges\\\":[{\\\"start\\\":0,\\\"end\\\":999}]}]\"", "memo1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo ASSET true"));
	UniValue inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	UniValue inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 0);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 0);

	AssetAllocationTransfer(true, "node1", "ASSET", "jagassetsendrangesownerallocation", "\"[{\\\"aliasto\\\":\\\"jagassetsendrangesa\\\",\\\"ranges\\\":[{\\\"start\\\":0,\\\"end\\\":199}]},{\\\"aliasto\\\":\\\"jagassetsendrangesb\\\",\\\"ranges\\\":[{\\\"start\\\":200,\\\"end\\\":399}]},{\\\"aliasto\\\":\\\"jagassetsendrangesc\\\",\\\"ranges\\\":[{\\\"start\\\":400,\\\"end\\\":599}]},{\\\"aliasto\\\":\\\"jagassetsendrangesd\\\",\\\"ranges\\\":[{\\\"start\\\":600,\\\"end\\\":799}]},{\\\"aliasto\\\":\\\"jagassetsendrangese\\\",\\\"ranges\\\":[{\\\"start\\\":800,\\\"end\\\":999}]}]\"", "memo");
	// ensure receiver get's it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangesa true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 199);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangesa true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 0);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 199);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangesb true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 200);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 399);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangesc true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 400);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 599);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangesd true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 600);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 799);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo ASSET jagassetsendrangese true"));
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 800);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 999);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 200 * COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "memo").get_str(), "memo");


	AssetUpdate("node1", "ASSET", "ASSET", "1000");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo ASSET true"));
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "balance")), 1000 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "total_supply")), 2000 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(find_value(r.get_obj(), "max_supply")), 1000000 * COIN);
	inputs = find_value(r.get_obj(), "inputs");
	BOOST_CHECK(inputs.isArray());
	inputsArray = inputs.get_array();
	BOOST_CHECK_EQUAL(inputsArray.size(), 1);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "start").get_int(), 1000);
	BOOST_CHECK_EQUAL(find_value(inputsArray[0].get_obj(), "end").get_int(), 1999);

}
BOOST_AUTO_TEST_CASE(generate_assettransfer)
{
	printf("Running generate_assettransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "jagasset1", "changeddata1");
	AliasNew("node2", "jagasset2", "changeddata2");
	AliasNew("node3", "jagasset3", "changeddata3");

	AssetNew("node1", "asset1", "jagasset1", "pubdata");
	AssetNew("node1",  "asset2", "jagasset1", "pubdata");
	AssetUpdate("node1", "asset1", "pub3");
	UniValue r;
	AssetTransfer("node1", "node2", "asset1", "jagasset2");
	AssetTransfer("node1", "node3", "asset2", "jagasset3");

	// xfer an asset that isn't yours
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assettransfer asset1 jagasset2 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());
	// update xferred asset
	AssetUpdate("node2", "asset1", "public");

	// retransfer asset
	AssetTransfer("node2", "node3", "asset1", "jagasset3");
}
BOOST_AUTO_TEST_CASE(generate_assetpruning)
{
	// asset's should not expire or be pruned
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_assetpruning...\n");
	AliasNew("node1", "jagprunealias1", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	AssetNew("node1", "jagprune1", "jagprunealias1", "pubdata");
	// we can find it as normal first
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));
	// make sure our offer alias doesn't expire
	AssetUpdate("node1", "jagprune1");
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprunealias1");
	StartNode("node2");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));

	// shouldn't be pruned
	BOOST_CHECK_NO_THROW(CallRPC("node2", "assetinfo jagprune1 false"));

	// stop node3
	StopNode("node3");
	
	AliasNew("node1", "jagprunealias1", "changeddata1");
	AssetUpdate("node1", "jagprune1");

	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetinfo jagprune1 false"));

	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasinfo jagprunealias1"));

	// try to create asset with same name
	BOOST_CHECK_THROW(CallRPC("node1", "assetnew jagprune1 jagprunealias1 pubdata assets 1 1 false 0 false ''"), runtime_error);
	StartNode("node3");
}
BOOST_AUTO_TEST_SUITE_END ()
