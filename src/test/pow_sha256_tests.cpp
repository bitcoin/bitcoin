// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_bitcoin.h"
#include "pow/sha256/sha256.h"

#include <boost/test/unit_test.hpp>

int WriteHex(uint8_t* dst, const char* hex) {
	int i = 0;
	while (hex[i]) {
		char l = hex[i++];
		char r = hex[i++];
		uint8_t v = (l >= 'a' && l <= 'f' ? 10 + l - 'a' : l - '0');
		*(dst++) = (v << 4) | (r >= 'a' && r <= 'f' ? 10 + r - 'a' : r - '0');
	}
	return i;
}

class collector_sha256 : public powa::callback {
public:
	bool finished;
	powa::solution_ref sol;
	collector_sha256(powa::solution_ref sol_in) : finished(false), sol(sol_in) {}
	bool found_solution(const powa::pow& p, powa::challenge_ref c, powa::solution_ref s) override {
		printf("found solution\n");
		sol = s;
		finished = true;
		return true;
	}
};

BOOST_FIXTURE_TEST_SUITE(pow_sha256_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(pow_sha256_tests)
{
	powa::fZeroStartingNonce = true;
	// zero header case
	{
		powa::challenge_ref chlg(new powa::sha256challenge(0x200fffff, 4, 0, 0));
		collector_sha256* coll = new collector_sha256(powa::solution_ref(new powa::solution((uint32_t)0)));
		powa::callback_ref collref(coll);
		powa::sha256 alg(chlg, collref);
		// the zero bit solution should not solve the zero header challenge
		BOOST_CHECK(!alg.is_valid(*coll->sol));
		// now solve the challenge; it should be solved within 5 attempts
		alg.solve(0, false, 5);
		// we should have solved the challenge
		BOOST_CHECK(coll->finished == true);
		// the solution should now solve the challenge
		BOOST_CHECK(alg.is_valid(*coll->sol));
		printf("done zero header case\n");
	}

	// HEADERLEN byte header case
	{
		uint8_t randomness[80];
		WriteHex(randomness, "43fcd0d3ee002389ff95c7e2c568aa0d5206688dd05589d5d5b040fd4bf837081911b814c02405e88c49bc52dc8a77ea48d73dc42d195740db2fa90498613fdfe96da5c2e3ba8deca6c65b9635");
		powa::challenge_ref chlg(new powa::sha256challenge(0x200fffff, 4, 76, 80));
		collector_sha256* coll = new collector_sha256(powa::solution_ref(new powa::solution((uint32_t)0)));
		powa::callback_ref collref(coll);
		chlg->set_params(randomness, 80);
		powa::sha256 alg(chlg, collref);
		// the zero bit solution does not solve the random header challenge (it isn't random, or we wouldn't be sure about that)
		BOOST_CHECK(!alg.is_valid(*coll->sol));
		// now solve the challenge; it should be solved within 11 attempts
		alg.solve(0, false, 11);
		// we should have solved the challenge
		BOOST_CHECK(coll->finished == true);
		// the solution should now solve the challenge
		BOOST_CHECK(alg.is_valid(*coll->sol));
	}
}

BOOST_AUTO_TEST_SUITE_END()
