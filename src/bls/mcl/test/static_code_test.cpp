#include <cybozu/test.hpp>
#include <mcl/bls12_381.hpp>

using namespace mcl::bn;

CYBOZU_TEST_AUTO(init)
{
	initPairing(mcl::BLS12_381);
}

CYBOZU_TEST_AUTO(Fr)
{
	Fr x, y;
	x = 3;
	y = 5;
	CYBOZU_TEST_EQUAL(x + y, 8);
	CYBOZU_TEST_EQUAL(x - y, -2);
	CYBOZU_TEST_EQUAL(x * y, 15);
}

CYBOZU_TEST_AUTO(Fp)
{
	Fp x, y;
	x = 3;
	y = 5;
	CYBOZU_TEST_EQUAL(x + y, 8);
	CYBOZU_TEST_EQUAL(x - y, -2);
	CYBOZU_TEST_EQUAL(x * y, 15);
}

CYBOZU_TEST_AUTO(Fp2)
{
	Fp2 x, y;
	x.a = 3;
	x.b = 2;
	y.a = 1;
	y.b = 4;
	/*
		(3+2i)(1+4i)=3-8+(12+2)i
	*/
	CYBOZU_TEST_EQUAL(x + y, Fp2(4, 6));
	CYBOZU_TEST_EQUAL(x - y, Fp2(2, -2));
	CYBOZU_TEST_EQUAL(x * y, Fp2(-5, 14));
}

CYBOZU_TEST_AUTO(G1)
{
	G1 P, Q;
	hashAndMapToG1(P, "abc", 3);
	Fr r1, r2;
	r1.setHashOf("abc", 3);
	r2 = -r1;
	G1::mul(Q, P, r1);
	Q = -Q;
	P *= r2;
	CYBOZU_TEST_EQUAL(P, Q);
}

CYBOZU_TEST_AUTO(G2)
{
	G2 P, Q;
	hashAndMapToG2(P, "abc", 3);
	Fr r1, r2;
	r1.setHashOf("abc", 3);
	r2 = -r1;
	G2::mul(Q, P, r1);
	Q = -Q;
	P *= r2;
	CYBOZU_TEST_EQUAL(P, Q);
}
