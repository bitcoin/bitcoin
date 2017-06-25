// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_bitcoin.h"
#include "pow/cuckoo_cycle/cuckoo_cycle.h"

#include <boost/test/unit_test.hpp>

class cc_collector : public powa::callback {
public:
	bool finished;
	powa::solution_ref sol;
	cc_collector(powa::solution_ref _sol) : finished(false), sol(_sol) {}
	bool found_solution(const powa::pow& p, powa::challenge_ref c, powa::solution_ref s) override {
		sol = s;
		finished = true;
		return true;
	}
};

BOOST_FIXTURE_TEST_SUITE(pow_cuckoo_cycle_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(pow_cuckoo_cycle_tests)
{
	powa::fZeroStartingNonce = true;
	// zero header case
	{
		uint32_t nonces[42];
		memset(nonces, 0, sizeof(nonces));
		powa::cuckoo_cycle::cc_challenge_ref chlg(new powa::cuckoo_cycle::cc_challenge());
		powa::solution_ref sol(new powa::solution());
		sol->params.resize(4 * 43);
		memset(&sol->params[0], 0, 4 * 43);
		cc_collector* coll = new cc_collector(sol);
		powa::callback_ref collref(coll);
		powa::cuckoo_cycle::cuckoo_cycle alg(chlg, collref);
		// the zero bit solution should not solve the zero header challenge
		BOOST_CHECK(!alg.is_valid(*coll->sol));
		// now solve the challenge; it should find a solution at nonce 2
		alg.next_nonce = 2;
		alg.solve(0, false, 1);
		// we should have solved the challenge
		BOOST_CHECK(coll->finished == true);
		// the solution should now solve the challenge
		BOOST_CHECK(alg.is_valid(*coll->sol));
	}

	// HEADERLEN byte header case
	{
		uint32_t nonces[42];
		memset(nonces, 0, sizeof(nonces));
		powa::cuckoo_cycle::cc_challenge_ref chlg = powa::cuckoo_cycle::cc_challenge::random_challenge(HEADERLEN);
		powa::solution_ref sol(new powa::solution());
		sol->params.resize(4 * 43);
		memset(&sol->params[0], 0, 4 * 43);
		cc_collector* coll = new cc_collector(sol);
		powa::callback_ref collref(coll);
		powa::cuckoo_cycle::cuckoo_cycle alg(chlg, collref);
		// the zero bit solution should not solve the random header challenge
		BOOST_CHECK(!alg.is_valid(*coll->sol));
		// now solve the challenge; it should find a solution before nonce 5
		alg.solve(0, false, 5);
		// we should have solved the challenge
		BOOST_CHECK(coll->finished == true);
		// the solution should now solve the challenge
		BOOST_CHECK(alg.is_valid(*coll->sol));
	}
}

BOOST_AUTO_TEST_SUITE_END()
