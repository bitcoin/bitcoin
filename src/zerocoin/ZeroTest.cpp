/**
* @file       Tests.cpp
*
* @brief      Test routines for Zerocoin.
*
* @author     Ian Miers, Christina Garman and Matthew Green
* @date       June 2013
*
* @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
* @license    This project is released under the MIT license.
**/

using namespace std;

#include <string>
#include <iostream>
#include <fstream>
#include <exception>
#include "Zerocoin.h"
#include "../util.h"

using namespace libzerocoin;
extern Params* ZCParams;


#define TESTS_COINS_TO_ACCUMULATE   10

// Global test counters
uint32_t    gNumTests        = 0;
uint32_t    gSuccessfulTests = 0;

// Proof size
uint32_t    gProofSize			= 0;
uint32_t    gCoinSize			= 0;
uint32_t	gSerialNumberSize	= 0;

// Global coin array
PrivateCoin    *gCoins[TESTS_COINS_TO_ACCUMULATE];

// Global params
Params *g_Params;

//////////
// Utility routines
//////////

void
LogTestResult(string testName, bool (*testPtr)())
{
	printf("Testing if %s ...\n", testName.c_str());

	bool testResult = testPtr();

	if (testResult == true) {
		printf("\t[PASS]\n");
		gSuccessfulTests++;
	} else {
		printf("\t[FAIL]\n");
	}

	gNumTests++;
}

Bignum
GetTestModulus()
{
	static Bignum testModulus(0);

	// TODO: should use a hard-coded RSA modulus for testing
	if (!testModulus) {
		Bignum p, q;

		// Note: we are NOT using safe primes for testing because
		// they take too long to generate. Don't do this in real
		// usage. See the paramgen utility for better code.
		p = Bignum::generatePrime(1024, false);
		q = Bignum::generatePrime(1024, false);
		testModulus = p * q;
	}

	return testModulus;
}

//////////
// Test routines
//////////

bool
Test_GenRSAModulus()
{
	Bignum result = GetTestModulus();

	if (!result) {
		return false;
	}
	else {
		return true;
	}
}

bool
Test_CalcParamSizes()
{
	bool result = true;
#if 0

	uint32_t pLen, qLen;

	try {
		calculateGroupParamLengths(4000, 80, &pLen, &qLen);
		if (pLen < 1024 || qLen < 256) {
			result = false;
		}
		calculateGroupParamLengths(4000, 96, &pLen, &qLen);
		if (pLen < 2048 || qLen < 256) {
			result = false;
		}
		calculateGroupParamLengths(4000, 112, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
		calculateGroupParamLengths(4000, 120, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
		calculateGroupParamLengths(4000, 128, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
	} catch (exception &e) {
		result = false;
	}
#endif

	return result;
}

bool
Test_GenerateGroupParams()
{
	int32_t pLen = 1024, qLen = 256, count;
	IntegerGroupParams group;

	for (count = 0; count < 1; count++) {

		try {
			group = deriveIntegerGroupParams(calculateSeed(GetTestModulus(), "test", ZEROCOIN_DEFAULT_SECURITYLEVEL, "TEST GROUP"), pLen, qLen);
		} catch (std::runtime_error e) {
			printf("Caught exception %s\n", e.what());
			return false;
		}

		// Now perform some simple tests on the resulting parameters
		if (group.groupOrder.bitSize() < qLen || group.modulus.bitSize() < pLen) {
			return false;
		}

		Bignum c = group.g.pow_mod(group.groupOrder, group.modulus);
		if (!(c.isOne())) return false;

		// Try at multiple parameter sizes
		pLen = pLen * 1.5;
		qLen = qLen * 1.5;
	}

	return true;
}

bool
Test_ParamGen()
{
	bool result = true;

	try {
		// Instantiating testParams runs the parameter generation code
		Params testParams(GetTestModulus(),ZEROCOIN_DEFAULT_SECURITYLEVEL);
	} catch (runtime_error e) {
		printf("ParamGen exception %s\n", e.what());
		result = false;
	}

	return result;
}

bool
Test_Accumulator()
{
	// This test assumes a list of coins were generated during
	// the Test_MintCoin() test.
	if (gCoins[0] == NULL) {
		return false;
	}
	try {
		// Accumulate the coin list from first to last into one accumulator
		Accumulator accOne(&g_Params->accumulatorParams);
		Accumulator accTwo(&g_Params->accumulatorParams);
		Accumulator accThree(&g_Params->accumulatorParams);
		Accumulator accFour(&g_Params->accumulatorParams);
		AccumulatorWitness wThree(g_Params, accThree, gCoins[0]->getPublicCoin());

		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			accOne += gCoins[i]->getPublicCoin();
			accTwo += gCoins[TESTS_COINS_TO_ACCUMULATE - (i+1)]->getPublicCoin();
			accThree += gCoins[i]->getPublicCoin();
			wThree += gCoins[i]->getPublicCoin();
			if(i != 0) {
				accFour += gCoins[i]->getPublicCoin();
			}
		}

		// Compare the accumulated results
		if (accOne.getValue() != accTwo.getValue() || accOne.getValue() != accThree.getValue()) {
			printf("Accumulators don't match\n");
			return false;
		}

		if(accFour.getValue() != wThree.getValue()) {
			printf("Witness math not working,\n");
			return false;
		}

		// Verify that the witness is correct
		if (!wThree.VerifyWitness(accThree, gCoins[0]->getPublicCoin()) ) {
			printf("Witness not valid\n");
			return false;
		}

		// Serialization test: see if we can serialize the accumulator
		CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
		ss << accOne;

		// Deserialize it into a new object
		Accumulator newAcc(g_Params, ss);

		// Compare the results
		if (accOne.getValue() != newAcc.getValue()) {
			return false;
		}

	} catch (runtime_error e) {
		return false;
	}

	return true;
}

bool
Test_EqualityPoK()
{
	// Run this test 10 times
	for (uint32_t i = 0; i < 10; i++) {
		try {
			// Generate a random integer "val"
			Bignum val = Bignum::randBignum(g_Params->coinCommitmentGroup.groupOrder);

			// Manufacture two commitments to "val", both
			// under different sets of parameters
			Commitment one(&g_Params->accumulatorParams.accumulatorPoKCommitmentGroup, val);

			Commitment two(&g_Params->serialNumberSoKCommitmentGroup, val);

			// Now generate a proof of knowledge that "one" and "two" are
			// both commitments to the same value
			CommitmentProofOfKnowledge pok(&g_Params->accumulatorParams.accumulatorPoKCommitmentGroup,
			                               &g_Params->serialNumberSoKCommitmentGroup,
			                               one, two);

			// Serialize the proof into a stream
			CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
			ss << pok;

			// Deserialize back into a PoK object
			CommitmentProofOfKnowledge newPok(&g_Params->accumulatorParams.accumulatorPoKCommitmentGroup,
			                                  &g_Params->serialNumberSoKCommitmentGroup,
			                                  ss);

			if (newPok.Verify(one.getCommitmentValue(), two.getCommitmentValue()) != true) {
				return false;
			}

			// Just for fun, deserialize the proof a second time
			CDataStream ss2(SER_NETWORK, PROTOCOL_VERSION);
			ss2 << pok;

			// This time tamper with it, then deserialize it back into a PoK
			ss2[15] = 0;
			CommitmentProofOfKnowledge newPok2(&g_Params->accumulatorParams.accumulatorPoKCommitmentGroup,
			                                   &g_Params->serialNumberSoKCommitmentGroup,
			                                   ss2);

			// If the tampered proof verifies, that's a failure!
			if (newPok2.Verify(one.getCommitmentValue(), two.getCommitmentValue()) == true) {
				return false;
			}

		} catch (runtime_error &e) {
			return false;
		}
	}

	return true;
}

bool
Test_MintCoin()
{
	gCoinSize = 0;

	try {
		// Generate a list of coins
		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			gCoins[i] = new PrivateCoin(g_Params);

			PublicCoin pc = gCoins[i]->getPublicCoin();
			CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
			ss << pc;
			gCoinSize += ss.size();
		}

		gCoinSize /= TESTS_COINS_TO_ACCUMULATE;

	} catch (exception &e) {
		return false;
	}

	return true;
}

bool
Test_MintAndSpend()
{
	try {
		// This test assumes a list of coins were generated in Test_MintCoin()
		if (gCoins[0] == NULL)
		{
			// No coins: mint some.
			Test_MintCoin();
			if (gCoins[0] == NULL) {
				return false;
			}
		}

		// Accumulate the list of generated coins into a fresh accumulator.
		// The first one gets marked as accumulated for a witness, the
		// others just get accumulated normally.
		Accumulator acc(&g_Params->accumulatorParams);
		AccumulatorWitness wAcc(g_Params, acc, gCoins[0]->getPublicCoin());

		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			acc += gCoins[i]->getPublicCoin();
			wAcc +=gCoins[i]->getPublicCoin();
		}

		// Now spend the coin
		SpendMetaData m(1,1);

		CoinSpend spend(g_Params, *(gCoins[0]), acc, wAcc, m);

		// Serialize the proof and deserialize into newSpend
		CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
		ss << spend;
		gProofSize = ss.size();
		CoinSpend newSpend(g_Params, ss);

		// See if we can verify the deserialized proof (return our result)
		bool ret =  newSpend.Verify(acc, m);
		
		// Extract the serial number
		Bignum serialNumber = newSpend.getCoinSerialNumber();
		gSerialNumberSize = ceil((double)serialNumber.bitSize() / 8.0);
		
		return ret;
	} catch (runtime_error &e) {
		printf("MintAndSpend exception %s\n", e.what());
		return false;
	}

	return false;
}

void
Test_RunAllTests()
{
	printf("ZeroCoin v%s self-test routine\n", ZEROCOIN_VERSION_STRING);

	// Make a new set of parameters from a random RSA modulus
	//g_Params = new Params(GetTestModulus());
	g_Params = ZCParams;

	gNumTests = gSuccessfulTests = gProofSize = 0;
	for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
		gCoins[i] = NULL;
	}

	// Run through all of the Zerocoin tests
	LogTestResult("an RSA modulus can be generated", Test_GenRSAModulus);
	LogTestResult("parameter sizes are correct", Test_CalcParamSizes);
	LogTestResult("group/field parameters can be generated", Test_GenerateGroupParams);
	LogTestResult("parameter generation is correct", Test_ParamGen);
	LogTestResult("coins can be minted", Test_MintCoin);
	LogTestResult("the accumulator works", Test_Accumulator);
	LogTestResult("the commitment equality PoK works", Test_EqualityPoK);
	LogTestResult("a minted coin can be spent", Test_MintAndSpend);

	printf("\nAverage coin size is %d bytes.\n", gCoinSize);
	printf("Serial number size is %d bytes.\n", gSerialNumberSize);
	printf("Spend proof size is %d bytes.\n", gProofSize);

	// Summarize test results
	if (gSuccessfulTests < gNumTests) {
		printf("\nERROR: SOME TESTS FAILED\n");
	}

	// Clear any generated coins
	for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
		delete gCoins[i];
	}

	printf("\n%d out of %d tests passed.\n\n", gSuccessfulTests, gNumTests);
	delete g_Params;
}
