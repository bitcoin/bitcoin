#include <mcl/gmp_util.hpp>
#include <cybozu/test.hpp>
#include <iostream>

CYBOZU_TEST_AUTO(sqrt)
{
	const int tbl[] = { 3, 5, 7, 11, 13, 17, 19, 257, 997, 1031 };
	mcl::SquareRoot sq;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const mpz_class p = tbl[i];
		sq.set(p);
		for (mpz_class a = 0; a < p; a++) {
			mpz_class x;
			if (sq.get(x, a)) {
				mpz_class y;
				y = (x * x) % p;
				CYBOZU_TEST_EQUAL(a, y);
			}
		}
	}
}
