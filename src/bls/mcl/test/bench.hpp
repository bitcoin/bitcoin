#include <mcl/lagrange.hpp>

void benchAddDblG1()
{
	puts("benchAddDblG1");
#ifndef NDEBUG
	puts("skip in debug");
	return;
#endif
	const int C = 100000;
	G1 P1, P2, P3;
	hashAndMapToG1(P1, "a");
	hashAndMapToG1(P2, "b");
	P1 += P2;
	P2 += P1;
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G1::add(1)", C, G1::add, P3, P1, P2);
	P1.normalize();
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G1::add(2)", C, G1::add, P3, P1, P2);
	CYBOZU_BENCH_C("G1::add(3)", C, G1::add, P3, P2, P1);
	P2.normalize();
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G1::add(4)", C, G1::add, P3, P1, P2);
	P1 = P3;
	printf("z.isOne()=%d\n", P1.z.isOne());
	CYBOZU_BENCH_C("G1::dbl(1)", C, G1::dbl, P3, P1);
	P1.normalize();
	printf("z.isOne()=%d\n", P1.z.isOne());
	CYBOZU_BENCH_C("G1::dbl(2)", C, G1::dbl, P3, P1);
}

void benchAddDblG2()
{
	puts("benchAddDblG2");
#ifndef NDEBUG
	puts("skip in debug");
	return;
#endif
	const int C = 100000;
	G2 P1, P2, P3;
	hashAndMapToG2(P1, "a");
	hashAndMapToG2(P2, "b");
	P1 += P2;
	P2 += P1;
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G2::add(1)", C, G2::add, P3, P1, P2);
	P1.normalize();
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G2::add(2)", C, G2::add, P3, P1, P2);
	CYBOZU_BENCH_C("G2::add(3)", C, G2::add, P3, P2, P1);
	P2.normalize();
	printf("z.isOne()=%d %d\n", P1.z.isOne(), P2.z.isOne());
	CYBOZU_BENCH_C("G2::add(4)", C, G2::add, P3, P1, P2);
	P1 = P3;
	printf("z.isOne()=%d\n", P1.z.isOne());
	CYBOZU_BENCH_C("G2::dbl(1)", C, G2::dbl, P3, P1);
	P1.normalize();
	printf("z.isOne()=%d\n", P1.z.isOne());
	CYBOZU_BENCH_C("G2::dbl(2)", C, G2::dbl, P3, P1);
}

template<class T>
void invAdd(T& out, const T& x, const T& y)
{
	T::inv(out, x);
	out += y;
}

void testBench(const G1& P, const G2& Q)
{
#ifndef NDEBUG
	puts("testBench skip in debug");
	return;
#endif
	G1 Pa;
	G2 Qa;
	Fp12 e1, e2;
	pairing(e1, P, Q);
	Fp12::pow(e2, e1, 12345);
	Fp x, y;
	x.setHashOf("abc");
	y.setHashOf("xyz");
	const int C = 1000;
	const int C3 = 100000;
#if 1
	const int C2 = 3000;
	{
		mpz_class a = x.getMpz();
		CYBOZU_BENCH_C("G1::mulCT     ", C, G1::mulCT, Pa, P, a);
		CYBOZU_BENCH_C("G1::mul       ", C, G1::mul, Pa, Pa, a);
		CYBOZU_BENCH_C("G1::add       ", C, G1::add, Pa, Pa, P);
		CYBOZU_BENCH_C("G1::dbl       ", C, G1::dbl, Pa, Pa);
		CYBOZU_BENCH_C("G2::mulCT     ", C, G2::mulCT, Qa, Q, a);
		CYBOZU_BENCH_C("G2::mul       ", C, G2::mul, Qa, Qa, a);
		CYBOZU_BENCH_C("G2::add       ", C, G2::add, Qa, Qa, Q);
		CYBOZU_BENCH_C("G2::dbl       ", C, G2::dbl, Qa, Qa);
		CYBOZU_BENCH_C("GT::pow       ", C, GT::pow, e1, e1, a);
	}
//	CYBOZU_BENCH_C("GT::powGLV    ", C, BN::param.glv2.pow, e1, e1, a);
	G1 PP;
	G2 QQ;
	std::string s;
	s = P.getStr();
	verifyOrderG1(true);
	CYBOZU_BENCH_C("G1::setStr chk", C, PP.setStr, s);
	verifyOrderG1(false);
	CYBOZU_BENCH_C("G1::setStr    ", C, PP.setStr, s);
	s = Q.getStr();
	verifyOrderG2(true);
	CYBOZU_BENCH_C("G2::setStr chk", C, QQ.setStr, s);
	verifyOrderG2(false);
	CYBOZU_BENCH_C("G2::setStr    ", C, QQ.setStr, s);
	CYBOZU_BENCH_C("hashAndMapToG1", C, hashAndMapToG1, PP, "abc", 3);
	CYBOZU_BENCH_C("hashAndMapToG2", C, hashAndMapToG2, QQ, "abc", 3);
#endif
	CYBOZU_BENCH_C("Fp::add       ", C3, Fp::add, x, x, y);
	CYBOZU_BENCH_C("Fp::sub       ", C3, Fp::sub, x, x, y);
	CYBOZU_BENCH_C("Fp::add 2     ", C3, Fp::add, x, x, x);
	CYBOZU_BENCH_C("Fp::mul2      ", C3, Fp::mul2, x, x);
	CYBOZU_BENCH_C("Fp::mulSmall8 ", C3, Fp::mulSmall, x, x, 8);
	CYBOZU_BENCH_C("Fp::mulUnit8  ", C3, Fp::mulUnit, x, x, 8);
	CYBOZU_BENCH_C("Fp::mul9      ", C3, Fp::mul9, x, x);
	CYBOZU_BENCH_C("Fp::mulUnit9  ", C3, Fp::mulUnit, x, x, 9);
	CYBOZU_BENCH_C("Fp::neg       ", C3, Fp::neg, x, x);
	CYBOZU_BENCH_C("Fp::mul       ", C3, Fp::mul, x, x, y);
	CYBOZU_BENCH_C("Fp::sqr       ", C3, Fp::sqr, x, x);
	CYBOZU_BENCH_C("Fp::inv       ", C3, invAdd, x, x, y);
	CYBOZU_BENCH_C("Fp::pow       ", C3, Fp::pow, x, x, y);
	{
		Fr a, b, c;
		a.setHashOf("abc", 3);
		b.setHashOf("123", 3);
		CYBOZU_BENCH_C("Fr::add       ", C3, Fr::add, a, a, b);
		CYBOZU_BENCH_C("Fr::sub       ", C3, Fr::sub, a, a, b);
		CYBOZU_BENCH_C("Fr::neg       ", C3, Fr::neg, a, a);
		CYBOZU_BENCH_C("Fr::add 2     ", C3, Fr::add, a, a, b);
		CYBOZU_BENCH_C("Fr::mul2      ", C3, Fr::mul2, a, a);
		CYBOZU_BENCH_C("Fr::mul       ", C3, Fr::mul, a, a, b);
		CYBOZU_BENCH_C("Fr::sqr       ", C3, Fr::sqr, a, a);
		CYBOZU_BENCH_C("Fr::inv       ", C3, invAdd, a, a, b);
		CYBOZU_BENCH_C("Fr::pow       ", C3, Fr::pow, a, a, b);
	}
	Fp2 xx, yy;
	xx.a = x;
	xx.b = 3;
	yy.a = y;
	yy.b = -5;
	FpDbl d0, d1;
	x = 9;
	y = 3;
#if 1
	CYBOZU_BENCH_C("Fp2::add      ", C3, Fp2::add, xx, xx, yy);
	CYBOZU_BENCH_C("Fp2::sub      ", C3, Fp2::sub, xx, xx, yy);
	CYBOZU_BENCH_C("Fp2::neg      ", C3, Fp2::neg, xx, xx);
	CYBOZU_BENCH_C("Fp2::mul2     ", C3, Fp2::mul2, xx, xx);
	CYBOZU_BENCH_C("Fp2::mul      ", C3, Fp2::mul, xx, xx, yy);
	CYBOZU_BENCH_C("Fp2::mul_xi   ", C3, Fp2::mul_xi, xx, xx);
	CYBOZU_BENCH_C("Fp2::sqr      ", C3, Fp2::sqr, xx, xx);
	CYBOZU_BENCH_C("Fp2::inv      ", C3, invAdd, xx, xx, yy);
	CYBOZU_BENCH_C("FpDbl::addPre ", C3, FpDbl::addPre, d1, d1, d0);
	CYBOZU_BENCH_C("FpDbl::subPre ", C3, FpDbl::subPre, d1, d1, d0);
	CYBOZU_BENCH_C("FpDbl::add    ", C3, FpDbl::add, d1, d1, d0);
	CYBOZU_BENCH_C("FpDbl::sub    ", C3, FpDbl::sub, d1, d1, d0);
	CYBOZU_BENCH_C("FpDbl::mulPre ", C3, FpDbl::mulPre, d0, x, y);
	CYBOZU_BENCH_C("FpDbl::sqrPre ", C3, FpDbl::sqrPre, d1, x);
	CYBOZU_BENCH_C("FpDbl::mod    ", C3, FpDbl::mod, x, d0);
	Fp2Dbl D;
	CYBOZU_BENCH_C("Fp2Dbl::mulPre ", C3, Fp2Dbl::mulPre, D, xx, yy);
	CYBOZU_BENCH_C("Fp2Dbl::sqrPre ", C3, Fp2Dbl::sqrPre, D, xx);

	CYBOZU_BENCH_C("GT::add       ", C2, GT::add, e1, e1, e2);
	CYBOZU_BENCH_C("GT::mul       ", C2, GT::mul, e1, e1, e2);
	CYBOZU_BENCH_C("GT::sqr       ", C2, GT::sqr, e1, e1);
	CYBOZU_BENCH_C("GT::inv       ", C2, GT::inv, e1, e1);
#endif
	CYBOZU_BENCH_C("pairing       ", 3000, pairing, e1, P, Q);
	CYBOZU_BENCH_C("millerLoop    ", 3000, millerLoop, e1, P, Q);
	CYBOZU_BENCH_C("finalExp      ", 3000, finalExp, e1, e1);
//exit(1);
	std::vector<Fp6> Qcoeff;
	CYBOZU_BENCH_C("precomputeG2  ", C, precomputeG2, Qcoeff, Q);
	precomputeG2(Qcoeff, Q);
	CYBOZU_BENCH_C("precomputedML ", C, precomputedMillerLoop, e2, P, Qcoeff);
	const size_t n = 7;
	G1 Pvec[n];
	G2 Qvec[n];
	for (size_t i = 0; i < n; i++) {
		char d = (char)(i + 1);
		hashAndMapToG1(Pvec[i], &d, 1);
		hashAndMapToG2(Qvec[i], &d, 1);
	}
	e2 = 1;
	for (size_t i = 0; i < n; i++) {
		millerLoop(e1, Pvec[i], Qvec[i]);
		e2 *= e1;
	}
	CYBOZU_BENCH_C("millerLoopVec ", 3000, millerLoopVec, e1, Pvec, Qvec, n);
	CYBOZU_TEST_EQUAL(e1, e2);
}

inline void SquareRootPrecomputeTest(const mpz_class& p)
{
	mcl::SquareRoot sq1, sq2;
	bool b;
	sq1.set(&b, p, true);
	CYBOZU_TEST_ASSERT(b);
	CYBOZU_TEST_ASSERT(sq1.isPrecomputed());
	sq2.set(&b, p, false);
	CYBOZU_TEST_ASSERT(sq1 == sq2);
	if (sq1 != sq2) {
		puts("dump");
		puts("sq1");
		sq1.dump();
		puts("sq2");
		sq2.dump();
		puts("---");
	}
}

void testSquareRoot()
{
	if (BN::param.cp == mcl::BN254 || BN::param.cp == mcl::BLS12_381) {
		SquareRootPrecomputeTest(BN::param.p);
		SquareRootPrecomputeTest(BN::param.r);
	}
}

void testLagrange()
{
	puts("testLagrange");
	const int n = 5;
	const int k = 3;
	Fr c[k];
	Fr x[n], y[n];
	for (size_t i = 0; i < k; i++) {
		c[i].setByCSPRNG();
	}
	for (size_t i = 0; i < n; i++) {
		x[i].setByCSPRNG();
		mcl::evaluatePolynomial(y[i], c, k, x[i]);
	}
	Fr xs[k], ys[k];
	for (int i0 = 0; i0 < n; i0++) {
		xs[0] = x[i0];
		ys[0] = y[i0];
		for (int i1 = i0 + 1; i1 < n; i1++) {
			xs[1] = x[i1];
			ys[1] = y[i1];
			for (int i2 = i1 + 1; i2 < n; i2++) {
				xs[2] = x[i2];
				ys[2] = y[i2];
				Fr s;
				s.clear();
				mcl::LagrangeInterpolation(s, xs, ys, k);
				CYBOZU_TEST_EQUAL(s, c[0]);
			}
		}
	}
}
