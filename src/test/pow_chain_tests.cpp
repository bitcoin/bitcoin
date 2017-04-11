// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_bitcoin.h"
#include "pow/sha256/sha256.h"
#include "pow/cuckoo_cycle/cuckoo_cycle.h"

#include <boost/test/unit_test.hpp>

class collector_chain : public powa::callback {
public:
	bool finished;
	powa::solution_ref sol;
	collector_chain(powa::solution_ref sol_in) : finished(false), sol(sol_in) {}
	bool found_solution(const powa::pow& p, powa::challenge_ref c, powa::solution_ref s) override {
		printf("found solution\n");
		sol = s;
		finished = true;
		return true;
	}
};

BOOST_FIXTURE_TEST_SUITE(pow_chain_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(pow_chain_tests)
{
	powa::fZeroStartingNonce = true;
	// zero header case
	{
		powa::challenge_ref chlg(new powa::sha256challenge(0x207fffff, 0, 0, 0));
		powa::cuckoo_cycle::cc_challenge_ref chlg2(new powa::cuckoo_cycle::cc_challenge());
		chlg2->params.resize(80);
		memset(&chlg2->params[0], 0, 80);
		powa::solution_ref sol(new powa::solution());
		sol->params.resize(4 * 43);
		memset(&sol->params[0], 0, 4 * 43);
		collector_chain* coll = new collector_chain(sol);
		powa::callback_ref collref(coll);
		powa::pow_ref alg(new powa::sha256(chlg));
		powa::pow_ref alg2(new powa::cuckoo_cycle::cuckoo_cycle(chlg2));
		powa::powchain chain;
		chain.add_pow(alg);
		chain.add_pow(alg2);
		chain.cb = collref;
		// the zero bit solution should not solve the zero header challenge
		BOOST_CHECK(!chain.is_valid(*coll->sol));
		// now solve the challenge; it should be solved within 5 attempts
		chain.solve(0, false, 200);
		// we should have solved the challenge
		BOOST_CHECK(coll->finished == true);
		// the solution should now solve the challenge
		BOOST_CHECK(chain.is_valid(*coll->sol));
		printf("done zero header case\n");
	}
}

BOOST_AUTO_TEST_SUITE_END()
