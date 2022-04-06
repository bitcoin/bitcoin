/*
	include from bn_if256_test.cpp and bn_if384_test.cpp
*/
#include <mcl/bn.h>
#include <mcl/ecparam.hpp>
#include <cybozu/test.hpp>
#include <iostream>
#include <mcl/gmp_util.hpp>

template<size_t N>
std::ostream& dump(std::ostream& os, const uint64_t (&x)[N])
{
	for (size_t i = 0; i < N; i++) {
		char buf[64];
		CYBOZU_SNPRINTF(buf, sizeof(buf), "%016llx", (long long)x[i]);
		os << buf;
	}
	return os;
}

CYBOZU_TEST_AUTO(init)
{
	int ret;
	CYBOZU_TEST_EQUAL(sizeof(mclBnFr), sizeof(Fr));
	CYBOZU_TEST_EQUAL(sizeof(mclBnG1), sizeof(G1));
	CYBOZU_TEST_EQUAL(sizeof(mclBnG2), sizeof(G2));
	CYBOZU_TEST_EQUAL(sizeof(mclBnGT), sizeof(Fp12));
	int curveType;

#if MCLBN_FP_UNIT_SIZE >= 4
	printf("test BN254 %d\n", MCLBN_FP_UNIT_SIZE);
	curveType = MCL_BN254;
#endif
#if MCLBN_FP_UNIT_SIZE >= 6 && MCLBN_FR_UNIT_SIZE >= 4
	printf("test BLS12_381 %d\n", MCLBN_FP_UNIT_SIZE);
	curveType = MCL_BLS12_381;
#endif
#if MCLBN_FP_UNIT_SIZE >= 6 && MCLBN_FR_UNIT_SIZE >= 6
	printf("test BN381_1 %d\n", MCLBN_FP_UNIT_SIZE);
	curveType = MCL_BN381_1;
#endif
#if MCLBN_FP_UNIT_SIZE == 8
	printf("test BN462 %d\n", MCLBN_FP_UNIT_SIZE);
	curveType = MCL_BN462;
#endif
	ret = mclBn_init(curveType, MCLBN_COMPILED_TIME_VAR);
	CYBOZU_TEST_EQUAL(ret, 0);
	if (ret != 0) exit(1);
	CYBOZU_TEST_EQUAL(curveType, mclBn_getCurveType());
}

CYBOZU_TEST_AUTO(Fr)
{
	mclBnFr x, y;
	memset(&x, 0xff, sizeof(x));
	CYBOZU_TEST_ASSERT(!mclBnFr_isValid(&x));
	CYBOZU_TEST_ASSERT(!mclBnFr_isZero(&x));

	mclBnFr_clear(&x);
	CYBOZU_TEST_ASSERT(mclBnFr_isZero(&x));

	mclBnFr_setInt(&x, 1);
	CYBOZU_TEST_ASSERT(mclBnFr_isOne(&x));

	mclBnFr_setInt(&y, -1);
	CYBOZU_TEST_ASSERT(!mclBnFr_isEqual(&x, &y));

	y = x;
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x, &y));

	mclBnFr_setHashOf(&x, "", 0);
	mclBnFr_setHashOf(&y, "abc", 3);
	CYBOZU_TEST_ASSERT(!mclBnFr_isEqual(&x, &y));
	mclBnFr_setHashOf(&x, "abc", 3);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x, &y));

	char buf[1024];
	mclBnFr_setInt(&x, 12345678);
	size_t size;
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 8);
	CYBOZU_TEST_EQUAL(buf, "12345678");

	mclBnFr_setInt(&x, -7654321);
	mclBnFr_neg(&x, &x);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 7);
	CYBOZU_TEST_EQUAL(buf, "7654321");

	mclBnFr_setInt(&y, 123 - 7654321);
	mclBnFr_add(&x, &x, &y);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 3);
	CYBOZU_TEST_EQUAL(buf, "123");

	mclBnFr_setInt(&y, 100);
	mclBnFr_sub(&x, &x, &y);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 2);
	CYBOZU_TEST_EQUAL(buf, "23");

	mclBnFr_mul(&x, &x, &y);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 4);
	CYBOZU_TEST_EQUAL(buf, "2300");

	mclBnFr_div(&x, &x, &y);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 2);
	CYBOZU_TEST_EQUAL(buf, "23");

	mclBnFr_mul(&x, &y, &y);
	mclBnFr_sqr(&y, &y);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x, &y));

	const char *s = "12345678901234567";
	CYBOZU_TEST_ASSERT(!mclBnFr_setStr(&x, s, strlen(s), 10));
	s = "20000000000000000";
	CYBOZU_TEST_ASSERT(!mclBnFr_setStr(&y, s, strlen(s), 10));
	mclBnFr_add(&x, &x, &y);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 17);
	CYBOZU_TEST_EQUAL(buf, "32345678901234567");

	mclBnFr_setInt(&x, 1);
	mclBnFr_neg(&x, &x);
	size = mclBnFr_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(!mclBnFr_setStr(&y, buf, size, 10));
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x, &y));

	for (int i = 0; i < 10; i++) {
		mclBnFr_setByCSPRNG(&x);
		mclBnFr_getStr(buf, sizeof(buf), &x, 16);
		printf("%s\n", buf);
	}
}

void G1test()
{
	mclBnG1 x, y, z;
	memset(&x, 0x1, sizeof(x));
	/*
		assert() of carry operation fails if use 0xff, so use 0x1
	*/
	CYBOZU_TEST_ASSERT(!mclBnG1_isValid(&x));
	mclBnG1_clear(&x);
	CYBOZU_TEST_ASSERT(mclBnG1_isValid(&x));
	CYBOZU_TEST_ASSERT(mclBnG1_isZero(&x));

	CYBOZU_TEST_ASSERT(!mclBnG1_hashAndMapTo(&y, "abc", 3));
	CYBOZU_TEST_ASSERT(mclBnG1_isValidOrder(&y));

	char buf[1024];
	size_t size;
	size = mclBnG1_getStr(buf, sizeof(buf), &y, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(!mclBnG1_setStr(&x, buf, strlen(buf), 10));
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&x, &y));

	mclBnG1_neg(&x, &x);
	mclBnG1_add(&x, &x, &y);
	CYBOZU_TEST_ASSERT(mclBnG1_isZero(&x));

	mclBnG1_dbl(&x, &y); // x = 2y
	mclBnG1_add(&z, &y, &y);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&x, &z));
	mclBnG1_add(&z, &z, &y); // z = 3y
	mclBnFr n;
	mclBnFr_setInt(&n, 3);
	mclBnG1_mul(&x, &y, &n); //  x = 3y
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&x, &z));
	mclBnG1_sub(&x, &x, &y); // x = 2y

	mclBnFr_setInt(&n, 2);
	mclBnG1_mul(&z, &y, &n); //  z = 2y
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&x, &z));
	mclBnG1_normalize(&y, &z);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&y, &z));
}

CYBOZU_TEST_AUTO(G1)
{
	G1test();
}

CYBOZU_TEST_AUTO(G2)
{
	mclBnG2 x, y, z;
	/*
		assert() of carry operation fails if use 0xff, so use 0x1
	*/
	memset(&x, 0x1, sizeof(x));
	CYBOZU_TEST_ASSERT(!mclBnG2_isValid(&x));
	mclBnG2_clear(&x);
	CYBOZU_TEST_ASSERT(mclBnG2_isValid(&x));
	CYBOZU_TEST_ASSERT(mclBnG2_isZero(&x));

	CYBOZU_TEST_ASSERT(!mclBnG2_hashAndMapTo(&x, "abc", 3));
	CYBOZU_TEST_ASSERT(mclBnG2_isValidOrder(&x));

	char buf[1024];
	size_t size;
	size = mclBnG2_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(!mclBnG2_setStr(&y, buf, strlen(buf), 10));
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&x, &y));

	mclBnG2_neg(&x, &x);
	mclBnG2_add(&x, &x, &y);
	CYBOZU_TEST_ASSERT(mclBnG2_isZero(&x));

	mclBnG2_dbl(&x, &y); // x = 2y
	mclBnG2_add(&z, &y, &y);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&x, &z));
	mclBnG2_add(&z, &z, &y); // z = 3y
	mclBnFr n;
	mclBnFr_setInt(&n, 3);
	mclBnG2_mul(&x, &y, &n); //  x = 3y
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&x, &z));
	mclBnG2_sub(&x, &x, &y); // x = 2y

	mclBnFr_setInt(&n, 2);
	mclBnG2_mul(&z, &y, &n); //  z = 2y
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&x, &z));
	mclBnG2_normalize(&y, &z);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&y, &z));
}

CYBOZU_TEST_AUTO(GT)
{
	mclBnGT x, y, z;
	memset(&x, 1, sizeof(x));
	CYBOZU_TEST_ASSERT(!mclBnGT_isZero(&x));

	mclBnGT_clear(&x);
	CYBOZU_TEST_ASSERT(mclBnGT_isZero(&x));

	mclBnGT_setInt(&x, 1);
	CYBOZU_TEST_ASSERT(mclBnGT_isOne(&x));
	char buf[2048];
	size_t size;
	size = mclBnGT_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	const char *s = "1 0 0 0 0 0 0 0 0 0 0 0";
	CYBOZU_TEST_EQUAL(buf, s);

	s = "1 2 3 4 5 6 7 8 9 10 11 12";
	CYBOZU_TEST_ASSERT(!mclBnGT_setStr(&x,s , strlen(s), 10));
	size = mclBnGT_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_EQUAL(buf, s);

	y = x;
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&x, &y));

	s = "-1 -2 -3 -4 -5 -6 -7 -8 -9 -10 -11 -12";
	CYBOZU_TEST_ASSERT(!mclBnGT_setStr(&z, s, strlen(s), 10));
	size = mclBnGT_getStr(buf, sizeof(buf), &z, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(!mclBnGT_setStr(&y, buf, size, 10));

	mclBnGT_neg(&z, &y);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&x, &z));

	mclBnGT_add(&y, &x, &y);
	CYBOZU_TEST_ASSERT(mclBnGT_isZero(&y));

	s = "2 0 0 0 0 0 0 0 0 0 0 0";
	CYBOZU_TEST_ASSERT(!mclBnGT_setStr(&y, s, strlen(s), 10));
	mclBnGT_mul(&z, &x, &y);
	size = mclBnGT_getStr(buf, sizeof(buf), &z, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_EQUAL(buf, "2 4 6 8 10 12 14 16 18 20 22 24");

	mclBnGT_div(&z, &z, &y);
	size = mclBnGT_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&x, &z));

	/*
		can't use mclBnGT_pow because x is not in GT
	*/
	mclBnFr n;
	mclBnFr_setInt(&n, 3);
	mclBnGT_powGeneric(&z, &x, &n);
	mclBnGT_mul(&y, &x, &x);
	mclBnGT_mul(&y, &y, &x);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&y, &z));

	mclBnGT_mul(&x, &y, &y);
	mclBnGT_sqr(&y, &y);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&x, &y));
}

CYBOZU_TEST_AUTO(GT_inv)
{
	mclBnG1 P;
	mclBnG2 Q;
	mclBnGT e, e1, e2, e3, e4;
	mclBnG1_hashAndMapTo(&P, "1", 1);
	mclBnG2_hashAndMapTo(&Q, "1", 1);
	// e is not in GT
	mclBn_millerLoop(&e, &P, &Q);
	mclBnGT_inv(&e1, &e); // e1 = a - b w if e = a + b w where Fp12 = Fp6[w]
	mclBnGT_invGeneric(&e2, &e);
	mclBnGT_mul(&e3, &e, &e1);
	mclBnGT_mul(&e4, &e, &e2);
	CYBOZU_TEST_ASSERT(!mclBnGT_isOne(&e3)); // GT_inv does not give a correct inverse for an element not in GT
	CYBOZU_TEST_ASSERT(mclBnGT_isOne(&e4));

	mclBn_finalExp(&e3, &e3); // e3 is in GT then e3 = 1
	CYBOZU_TEST_ASSERT(mclBnGT_isOne(&e3));

	// e is in GT
	mclBn_finalExp(&e, &e);
	mclBnGT_inv(&e1, &e);
	mclBnGT_invGeneric(&e2, &e);
	mclBnGT_mul(&e3, &e, &e1);
	mclBnGT_mul(&e4, &e, &e2);
	CYBOZU_TEST_ASSERT(mclBnGT_isOne(&e3)); // GT_inv gives a correct inverse for an element in GT
	CYBOZU_TEST_ASSERT(mclBnGT_isOne(&e4));
}

CYBOZU_TEST_AUTO(Fr_isNegative)
{
	mclBnFr a, half, one;
	mclBnFr_setInt(&half, 2);
	mclBnFr_inv(&half, &half); // half = (r + 1) / 2
	mclBnFr_setInt(&one, 1);
	mclBnFr_sub(&a, &half, &one);
	CYBOZU_TEST_ASSERT(!mclBnFr_isNegative(&a));
	mclBnFr_add(&a, &a, &one);
	CYBOZU_TEST_ASSERT(mclBnFr_isNegative(&a));
}

CYBOZU_TEST_AUTO(Fp_isNegative)
{
	mclBnFp a, half, one;
	mclBnFp_setInt(&half, 2);
	mclBnFp_inv(&half, &half); // half = (p + 1) / 2
	mclBnFp_setInt(&one, 1);
	mclBnFp_sub(&a, &half, &one);
	CYBOZU_TEST_ASSERT(!mclBnFp_isNegative(&a));
	mclBnFp_add(&a, &a, &one);
	CYBOZU_TEST_ASSERT(mclBnFp_isNegative(&a));
}

CYBOZU_TEST_AUTO(Fr_isOdd)
{
	mclBnFr x, one;
	mclBnFr_clear(&x);
	mclBnFr_setInt(&one, 1);
	for (size_t i = 0; i < 100; i++) {
		CYBOZU_TEST_EQUAL(mclBnFr_isOdd(&x), i & 1);
		mclBnFr_add(&x, &x, &one);
	}
}

CYBOZU_TEST_AUTO(Fp_isOdd)
{
	mclBnFp x, one;
	mclBnFp_clear(&x);
	mclBnFp_setInt(&one, 1);
	for (size_t i = 0; i < 100; i++) {
		CYBOZU_TEST_EQUAL(mclBnFp_isOdd(&x), i & 1);
		mclBnFp_add(&x, &x, &one);
	}
}

CYBOZU_TEST_AUTO(pairing)
{
	mclBnFr a, b, ab;
	mclBnFr_setInt(&a, 123);
	mclBnFr_setInt(&b, 456);
	mclBnFr_mul(&ab, &a, &b);
	mclBnG1 P, aP;
	mclBnG2 Q, bQ;
	mclBnGT e, e1, e2;

	CYBOZU_TEST_ASSERT(!mclBnG1_hashAndMapTo(&P, "1", 1));
	CYBOZU_TEST_ASSERT(!mclBnG2_hashAndMapTo(&Q, "1", 1));

	mclBnG1_mul(&aP, &P, &a);
	mclBnG2_mul(&bQ, &Q, &b);

	mclBn_pairing(&e, &P, &Q);
	mclBnGT_pow(&e1, &e, &a);
	mclBn_pairing(&e2, &aP, &Q);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &e2));

	mclBnGT_pow(&e1, &e, &b);
	mclBn_pairing(&e2, &P, &bQ);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &e2));

	mclBnFr n;
	mclBnFr_setInt(&n, 3);
	mclBnGT_pow(&e1, &e, &n);
	mclBnGT_mul(&e2, &e, &e);
	mclBnGT_mul(&e2, &e2, &e);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &e2));
}

CYBOZU_TEST_AUTO(precomputed)
{
	mclBnG1 P1, P2;
	mclBnG2 Q1, Q2;
	CYBOZU_TEST_ASSERT(!mclBnG1_hashAndMapTo(&P1, "1", 1));
	CYBOZU_TEST_ASSERT(!mclBnG1_hashAndMapTo(&P2, "123", 3));
	CYBOZU_TEST_ASSERT(!mclBnG2_hashAndMapTo(&Q1, "1", 1));
	CYBOZU_TEST_ASSERT(!mclBnG2_hashAndMapTo(&Q2, "2", 1));

	const int size = mclBn_getUint64NumToPrecompute();
	std::vector<uint64_t> Q1buf, Q2buf;
	Q1buf.resize(size);
	Q2buf.resize(size);
	mclBn_precomputeG2(Q1buf.data(), &Q1);
	mclBn_precomputeG2(Q2buf.data(), &Q2);

	mclBnGT e1, e2, f1, f2, f3, f4;
	mclBn_pairing(&e1, &P1, &Q1);
	mclBn_precomputedMillerLoop(&f1, &P1, Q1buf.data());
	mclBn_finalExp(&f1, &f1);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &f1));

	mclBn_pairing(&e2, &P2, &Q2);
	mclBn_precomputedMillerLoop(&f2, &P2, Q2buf.data());
	mclBn_finalExp(&f2, &f2);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e2, &f2));

	mclBn_precomputedMillerLoop2(&f3, &P1, Q1buf.data(), &P2, Q2buf.data());
	mclBn_precomputedMillerLoop2mixed(&f4, &P1, &Q1, &P2, Q2buf.data());
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&f3, &f4));
	mclBn_finalExp(&f3, &f3);

	mclBnGT_mul(&e1, &e1, &e2);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &f3));
}

CYBOZU_TEST_AUTO(millerLoopVec)
{
	const size_t n = 7;
	mclBnG1 Pvec[n];
	mclBnG2 Qvec[n];
	for (size_t i = 0; i < n; i++) {
		char d = (char)(i + 1);
		mclBnG1_hashAndMapTo(&Pvec[i], &d, 1);
		mclBnG2_hashAndMapTo(&Qvec[i], &d, 1);
	}
	mclBnGT e1, e2;
	mclBnGT_setInt(&e2, 1);
	for (size_t i = 0; i < n; i++) {
		mclBn_millerLoop(&e1, &Pvec[i], &Qvec[i]);
		mclBnGT_mul(&e2, &e2, &e1);
	}
	mclBn_millerLoopVec(&e1, Pvec, Qvec, n);
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &e2));
}

CYBOZU_TEST_AUTO(millerLoopVecMT)
{
	const size_t n = 10;
	mclBnG1 Pvec[n];
	mclBnG2 Qvec[n];
	for (size_t i = 0; i < n; i++) {
		char d = (char)(i + 1);
		mclBnG1_hashAndMapTo(&Pvec[i], &d, 1);
		mclBnG2_hashAndMapTo(&Qvec[i], &d, 1);
	}
	for (size_t cpuN = 0; cpuN < 4; cpuN++) {
		mclBnGT e1, e2;
		mclBnGT_setInt(&e2, 1);
		for (size_t i = 0; i < n; i++) {
			mclBn_millerLoop(&e1, &Pvec[i], &Qvec[i]);
			mclBnGT_mul(&e2, &e2, &e1);
		}
		mclBn_millerLoopVecMT(&e1, Pvec, Qvec, n, cpuN);
		CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&e1, &e2));
	}
}

CYBOZU_TEST_AUTO(serialize)
{
	const size_t FrSize = mclBn_getFrByteSize();
	const size_t G1Size = mclBn_getG1ByteSize();
	mclBnFr x1, x2;
	mclBnG1 P1, P2;
	mclBnG2 Q1, Q2;
	char buf[1024];
	size_t n;
	size_t expectSize;
	size_t ret;
	// Fr
	expectSize = FrSize;
	mclBnFr_setInt(&x1, -1);
	n = mclBnFr_serialize(buf, sizeof(buf), &x1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnFr_deserialize(&x2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x1, &x2));

	ret = mclBnFr_deserialize(&x2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&x2, 0, sizeof(x2));
	ret = mclBnFr_deserialize(&x2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x1, &x2));

	n = mclBnFr_serialize(buf, expectSize, &x1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// G1
	expectSize = G1Size;
	mclBnG1_hashAndMapTo(&P1, "1", 1);
	n = mclBnG1_serialize(buf, sizeof(buf), &P1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnG1_deserialize(&P2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&P1, &P2));

	ret = mclBnG1_deserialize(&P2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&P2, 0, sizeof(P2));
	ret = mclBnG1_deserialize(&P2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&P1, &P2));

	n = mclBnG1_serialize(buf, expectSize, &P1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// G2
	expectSize = G1Size * 2;
	mclBnG2_hashAndMapTo(&Q1, "1", 1);
	n = mclBnG2_serialize(buf, sizeof(buf), &Q1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnG2_deserialize(&Q2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&Q1, &Q2));

	ret = mclBnG2_deserialize(&Q2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&Q2, 0, sizeof(Q2));
	ret = mclBnG2_deserialize(&Q2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&Q1, &Q2));

	n = mclBnG2_serialize(buf, expectSize, &Q1);
	CYBOZU_TEST_EQUAL(n, expectSize);
}

CYBOZU_TEST_AUTO(serializeToHexStr)
{
	const size_t FrSize = mclBn_getFrByteSize();
	const size_t G1Size = mclBn_getG1ByteSize();
	mclBnFr x1, x2;
	mclBnG1 P1, P2;
	mclBnG2 Q1, Q2;
	char buf[1024];
	size_t n;
	size_t expectSize;
	size_t ret;
	// Fr
	expectSize = FrSize * 2; // hex string
	mclBnFr_setInt(&x1, -1);
	n = mclBnFr_getStr(buf, sizeof(buf), &x1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnFr_setStr(&x2, buf, n, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x1, &x2));

	ret = mclBnFr_setStr(&x2, buf, n - 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_ASSERT(ret != 0);

	memset(&x2, 0, sizeof(x2));
	ret = mclBnFr_setStr(&x2, buf, n + 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&x1, &x2));

	n = mclBnFr_getStr(buf, expectSize, &x1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// G1
	expectSize = G1Size * 2; // hex string
	mclBnG1_hashAndMapTo(&P1, "1", 1);
	n = mclBnG1_getStr(buf, sizeof(buf), &P1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnG1_setStr(&P2, buf, n, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&P1, &P2));

	ret = mclBnG1_setStr(&P2, buf, n - 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_ASSERT(ret != 0);

	memset(&P2, 0, sizeof(P2));
	ret = mclBnG1_setStr(&P2, buf, n + 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&P1, &P2));

	n = mclBnG1_getStr(buf, expectSize, &P1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// G2
	expectSize = G1Size * 2 * 2; // hex string
	mclBnG2_hashAndMapTo(&Q1, "1", 1);
	n = mclBnG2_getStr(buf, sizeof(buf), &Q1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = mclBnG2_setStr(&Q2, buf, n, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&Q1, &Q2));

	ret = mclBnG2_setStr(&Q2, buf, n - 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_ASSERT(ret != 0);

	memset(&Q2, 0, sizeof(Q2));
	ret = mclBnG2_setStr(&Q2, buf, n + 1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&Q1, &Q2));

	n = mclBnG2_getStr(buf, expectSize, &Q1, MCLBN_IO_SERIALIZE_HEX_STR);
	CYBOZU_TEST_EQUAL(n, expectSize);
}

CYBOZU_TEST_AUTO(ETHserialization)
{
	int curveType = mclBn_getCurveType();
	if (curveType != MCL_BLS12_381) return;
	int keepETH = mclBn_getETHserialization();
	char buf[128] = {};
	char str[128];
	buf[0] = 0x12;
	buf[1] = 0x34;
	size_t n;
	mclBnFr x;
	mclBn_setETHserialization(false);
	n = mclBnFr_deserialize(&x, buf, 32);
	CYBOZU_TEST_EQUAL(n, 32);
	n = mclBnFr_getStr(str, sizeof(str), &x, 16);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_EQUAL(strcmp(str, "3412"), 0);

	mclBn_setETHserialization(true);
	n = mclBnFr_deserialize(&x, buf, 32);
	CYBOZU_TEST_EQUAL(n, 32);
	n = mclBnFr_getStr(str, sizeof(str), &x, 16);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_EQUAL(strcmp(str, "1234000000000000000000000000000000000000000000000000000000000000"), 0);

	mclBnFp y;
	mclBn_setETHserialization(false);
	n = mclBnFp_deserialize(&y, buf, 48);
	CYBOZU_TEST_EQUAL(n, 48);
	n = mclBnFp_getStr(str, sizeof(str), &y, 16);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_EQUAL(strcmp(str, "3412"), 0);

	mclBn_setETHserialization(true);
	n = mclBnFp_deserialize(&y, buf, 48);
	CYBOZU_TEST_EQUAL(n, 48);
	n = mclBnFp_getStr(str, sizeof(str), &y, 16);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_EQUAL(strcmp(str, "123400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"), 0);

	mclBn_setETHserialization(keepETH);
}

struct Fp2Str {
	const char *a;
	const char *b;
};

void setFp2(mclBnFp2 *x, const Fp2Str& s)
{
	CYBOZU_TEST_EQUAL(mclBnFp_setStr(&x->d[0], s.a, strlen(s.a), 16), 0);
	CYBOZU_TEST_EQUAL(mclBnFp_setStr(&x->d[1], s.b, strlen(s.b), 16), 0);
}

#if MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE >= 6
CYBOZU_TEST_AUTO(badG2)
{
	int ret;
	ret = mclBn_init(MCL_BN381_1, MCLBN_COMPILED_TIME_VAR);
	CYBOZU_TEST_EQUAL(ret, 0);
	const char *s = "1 18d3d8c085a5a5e7553c3a4eb628e88b8465bf4de2612e35a0a4eb018fb0c82e9698896031e62fd7633ffd824a859474 1dc6edfcf33e29575d4791faed8e7203832217423bf7f7fbf1f6b36625b12e7132c15fbc15562ce93362a322fb83dd0d 65836963b1f7b6959030ddfa15ab38ce056097e91dedffd996c1808624fa7e2644a77be606290aa555cda8481cfb3cb 1b77b708d3d4f65aeedf54b58393463a42f0dc5856baadb5ce608036baeca398c5d9e6b169473a8838098fd72fd28b50";
	mclBnG2 Q;
	ret = mclBnG2_setStr(&Q, s, strlen(s), 16);
	CYBOZU_TEST_ASSERT(ret != 0);
}
#endif

struct Sequential {
	uint32_t pos;
	Sequential() : pos(0) {}
	static uint32_t read(void *self, void *buf, uint32_t bufSize)
	{
		Sequential *seq = reinterpret_cast<Sequential*>(self);
		uint8_t *p = reinterpret_cast<uint8_t*>(buf);
		for (uint32_t i = 0; i < bufSize; i++) {
			p[i] = uint8_t(seq->pos + i) & 0x1f; // mask is to make valid Fp
		}
		seq->pos += bufSize;
		return bufSize;
	}
};

CYBOZU_TEST_AUTO(setRandFunc)
{
	Sequential seq;
	for (int j = 0; j < 3; j++) {
		puts(j == 1 ? "sequential rand" : "true rand");
		for (int i = 0; i < 5; i++) {
			mclBnFr x;
			int ret;
			char buf[1024];
			ret = mclBnFr_setByCSPRNG(&x);
			CYBOZU_TEST_EQUAL(ret, 0);
			size_t n = mclBnFr_getStr(buf, sizeof(buf), &x, 16);
			CYBOZU_TEST_ASSERT(n > 0);
			printf("%d %s\n", i, buf);
		}
		if (j == 0) {
			mclBn_setRandFunc(&seq, Sequential::read);
		} else {
			mclBn_setRandFunc(0, 0);
		}
	}
}

CYBOZU_TEST_AUTO(Fp_1)
{
	mclBnFp x, y;
	memset(&x, 0xff, sizeof(x));
	CYBOZU_TEST_ASSERT(!mclBnFp_isValid(&x));
	CYBOZU_TEST_ASSERT(!mclBnFp_isZero(&x));

	mclBnFp_clear(&x);
	CYBOZU_TEST_ASSERT(mclBnFp_isZero(&x));

	mclBnFp_setInt(&x, 1);
	CYBOZU_TEST_ASSERT(mclBnFp_isOne(&x));

	mclBnFp_setInt(&y, -1);
	CYBOZU_TEST_ASSERT(!mclBnFp_isEqual(&x, &y));

	y = x;
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x, &y));

	mclBnFp_setHashOf(&x, "", 0);
	mclBnFp_setHashOf(&y, "abc", 3);
	CYBOZU_TEST_ASSERT(!mclBnFp_isEqual(&x, &y));
	mclBnFp_setHashOf(&x, "abc", 3);
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x, &y));

	char buf[1024];
	mclBnFp_setInt(&x, 12345678);
	size_t size;
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 8);
	CYBOZU_TEST_EQUAL(buf, "12345678");

	mclBnFp_setInt(&x, -7654321);
	mclBnFp_neg(&x, &x);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 7);
	CYBOZU_TEST_EQUAL(buf, "7654321");

	mclBnFp_setInt(&y, 123 - 7654321);
	mclBnFp_add(&x, &x, &y);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 3);
	CYBOZU_TEST_EQUAL(buf, "123");

	mclBnFp_setInt(&y, 100);
	mclBnFp_sub(&x, &x, &y);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 2);
	CYBOZU_TEST_EQUAL(buf, "23");

	mclBnFp_mul(&x, &x, &y);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 4);
	CYBOZU_TEST_EQUAL(buf, "2300");

	mclBnFp_div(&x, &x, &y);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 2);
	CYBOZU_TEST_EQUAL(buf, "23");

	mclBnFp_mul(&x, &y, &y);
	mclBnFp_sqr(&y, &y);
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x, &y));

	const char *s = "12345678901234567";
	CYBOZU_TEST_ASSERT(!mclBnFp_setStr(&x, s, strlen(s), 10));
	s = "20000000000000000";
	CYBOZU_TEST_ASSERT(!mclBnFp_setStr(&y, s, strlen(s), 10));
	mclBnFp_add(&x, &x, &y);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_EQUAL(size, 17);
	CYBOZU_TEST_EQUAL(buf, "32345678901234567");

	mclBnFp_setInt(&x, 1);
	mclBnFp_neg(&x, &x);
	size = mclBnFp_getStr(buf, sizeof(buf), &x, 10);
	CYBOZU_TEST_ASSERT(size > 0);
	CYBOZU_TEST_EQUAL(size, strlen(buf));
	CYBOZU_TEST_ASSERT(!mclBnFp_setStr(&y, buf, size, 10));
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x, &y));

	for (int i = 0; i < 10; i++) {
		mclBnFp_setByCSPRNG(&x);
		mclBnFp_getStr(buf, sizeof(buf), &x, 16);
		printf("%s\n", buf);
	}
}

CYBOZU_TEST_AUTO(Fp)
{
	mclBnFp x1, x2;
	char buf[1024];
	int ret = mclBnFp_setHashOf(&x1, "abc", 3);
	CYBOZU_TEST_ASSERT(ret == 0);
	mclSize n = mclBnFp_serialize(buf, sizeof(buf), &x1);
	CYBOZU_TEST_ASSERT(n > 0);
	n = mclBnFp_deserialize(&x2, buf, n);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x1, &x2));
	for (size_t i = 0; i < n; i++) {
		buf[i] = char(i);
	}
	ret = mclBnFp_setLittleEndian(&x1, buf, n);
	CYBOZU_TEST_ASSERT(ret == 0);
	memset(buf, 0, sizeof(buf));
	n = mclBnFp_serialize(buf, sizeof(buf), &x1);
	CYBOZU_TEST_ASSERT(n > 0);
	for (size_t i = 0; i < n - 1; i++) {
		CYBOZU_TEST_EQUAL(buf[i], char(i));
	}
	mclBnFp_clear(&x1);
	memset(&x2, 0, sizeof(x2));
	CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&x1, &x2));

	mclBnFp_clear(&x1);
	CYBOZU_TEST_ASSERT(mclBnFp_isZero(&x1));

	mclBnFp_setInt(&x1, 1);
	CYBOZU_TEST_ASSERT(mclBnFp_isOne(&x1));

	mclBnFp_setInt(&x1, -1);
	CYBOZU_TEST_ASSERT(!mclBnFp_isOne(&x1));
    mclBnFp_neg(&x1, &x1);
	CYBOZU_TEST_ASSERT(mclBnFp_isOne(&x1));
}

CYBOZU_TEST_AUTO(mod)
{
	{
		// Fp
		char buf[1024];
		mclBn_getFieldOrder(buf, sizeof(buf));
		mpz_class p(buf);
		mpz_class x = mpz_class(1) << (mclBn_getFpByteSize() * 2);
		mclBnFp y;
		int ret = mclBnFp_setLittleEndianMod(&y, mcl::gmp::getUnit(x), mcl::gmp::getUnitSize(x) * sizeof(void*));
		CYBOZU_TEST_EQUAL(ret, 0);
		mclBnFp_getStr(buf, sizeof(buf), &y, 10);
		CYBOZU_TEST_EQUAL(mpz_class(buf), x % p);
	}
	{
		// Fr
		char buf[1024];
		mclBn_getCurveOrder(buf, sizeof(buf));
		mpz_class p(buf);
		mpz_class x = mpz_class(1) << (mclBn_getFrByteSize() * 2);
		mclBnFr y;
		int ret = mclBnFr_setLittleEndianMod(&y, mcl::gmp::getUnit(x), mcl::gmp::getUnitSize(x) * sizeof(void*));
		CYBOZU_TEST_EQUAL(ret, 0);
		mclBnFr_getStr(buf, sizeof(buf), &y, 10);
		CYBOZU_TEST_EQUAL(mpz_class(buf), x % p);
	}
}

CYBOZU_TEST_AUTO(Fp2)
{
	mclBnFp2 x1, x2;
	char buf[1024];
	int ret = mclBnFp_setHashOf(&x1.d[0], "abc", 3);
	CYBOZU_TEST_ASSERT(ret == 0);
	ret = mclBnFp_setHashOf(&x1.d[1], "xyz", 3);
	CYBOZU_TEST_ASSERT(ret == 0);
	mclSize n = mclBnFp2_serialize(buf, sizeof(buf), &x1);
	CYBOZU_TEST_ASSERT(n > 0);
	n = mclBnFp2_deserialize(&x2, buf, n);
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&x1, &x2));

	mclBnFp2 y, z;
	mclBnFp2_add(&y, &x1, &x2);
	for (int i = 0; i < 2; i++) {
		mclBnFp t;
		mclBnFp_add(&t, &x1.d[i], &x2.d[i]);
		CYBOZU_TEST_ASSERT(mclBnFp_isEqual(&y.d[i], &t));
	}
	mclBnFp2_sub(&y, &y, &x2);
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&y, &x1));
	mclBnFp2_mul(&y, &x1, &x2);
	mclBnFp2_div(&y, &y, &x1);
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&y, &x2));
	mclBnFp2_inv(&y, &x1);
	mclBnFp2_mul(&y, &y, &x1);
	CYBOZU_TEST_ASSERT(mclBnFp2_isOne(&y));
	mclBnFp2_sqr(&y, &x1);
	mclBnFp2_mul(&z, &x1, &x1);
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&y, &z));
	mclBnFp2_sub(&y, &x1, &x2);
	mclBnFp2_sub(&z, &x2, &x1);
	mclBnFp2_neg(&z, &z);
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&y, &z));

	mclBnFp2_clear(&x1);
	memset(&x2, 0, sizeof(x2));
	CYBOZU_TEST_ASSERT(mclBnFp2_isEqual(&x1, &x2));
	CYBOZU_TEST_ASSERT(mclBnFp2_isZero(&x1));
}

CYBOZU_TEST_AUTO(squareRootFr)
{
	mclBnFr x, y, y2;
	for (int i = 0; i < 10; i++) {
		mclBnFr_setInt(&x, i * i);
		CYBOZU_TEST_EQUAL(mclBnFr_squareRoot(&y, &x), 0);
		mclBnFr_sqr(&y2, &y);
		CYBOZU_TEST_EQUAL(mclBnFr_isEqual(&x, &y2), 1);
	}
	char buf[128];
	mclBnFr_setInt(&x, -1);
	CYBOZU_TEST_ASSERT(mclBnFr_serialize(buf, sizeof(buf), &x) > 0);
	int mod8 = (buf[0] + 1) & 7;
	/*
		(2)
		(p) = (-1)^((p^2-1)/8) = 1 if and only if there is x s.t. x^2 = 2 mod p
	*/
	bool hasSquareRoot = (((mod8 * mod8 - 1) / 8) & 1) == 0;
	printf("Fr:hasSquareRoot=%d\n", hasSquareRoot);
	mclBnFr_setInt(&x, 2);
	CYBOZU_TEST_EQUAL(mclBnFr_squareRoot(&y, &x), hasSquareRoot ? 0 : -1);
	if (hasSquareRoot) {
		mclBnFr_sqr(&y2, &y);
		CYBOZU_TEST_EQUAL(mclBnFr_isEqual(&x, &y2), 1);
	}
}

CYBOZU_TEST_AUTO(squareRootFp)
{
	mclBnFp x, y, y2;
	for (int i = 0; i < 10; i++) {
		mclBnFp_setInt(&x, i * i);
		CYBOZU_TEST_EQUAL(mclBnFp_squareRoot(&y, &x), 0);
		mclBnFp_sqr(&y2, &y);
		CYBOZU_TEST_EQUAL(mclBnFp_isEqual(&x, &y2), 1);
	}
	char buf[128];
	mclBnFp_setInt(&x, -1);
	CYBOZU_TEST_ASSERT(mclBnFp_serialize(buf, sizeof(buf), &x) > 0);
	int mod8 = (buf[0] + 1) & 7;
	/*
		(2)
		(p) = (-1)^((p^2-1)/8) = 1 if and only if there is x s.t. x^2 = 2 mod p
	*/
	bool hasSquareRoot = (((mod8 * mod8 - 1) / 8) & 1) == 0;
	printf("Fp:hasSquareRoot=%d\n", hasSquareRoot);
	mclBnFp_setInt(&x, 2);
	CYBOZU_TEST_EQUAL(mclBnFp_squareRoot(&y, &x), hasSquareRoot ? 0 : -1);
	if (hasSquareRoot) {
		mclBnFp_sqr(&y2, &y);
		CYBOZU_TEST_EQUAL(mclBnFp_isEqual(&x, &y2), 1);
	}
}

CYBOZU_TEST_AUTO(squareRootFp2)
{
	mclBnFp2 x, y, y2;
	for (int i = 0; i < 10; i++) {
		mclBnFp_setByCSPRNG(&x.d[0]);
		mclBnFp_setByCSPRNG(&x.d[1]);
		mclBnFp2_sqr(&x, &x);
		CYBOZU_TEST_EQUAL(mclBnFp2_squareRoot(&y, &x), 0);
		mclBnFp2_sqr(&y2, &y);
		CYBOZU_TEST_EQUAL(mclBnFp2_isEqual(&x, &y2), 1);
	}
}

CYBOZU_TEST_AUTO(mapToG1)
{
	mclBnFp x;
	mclBnG1 P1, P2;
	mclBnFp_setHashOf(&x, "abc", 3);
	int ret = mclBnFp_mapToG1(&P1, &x);
	CYBOZU_TEST_ASSERT(ret == 0);
	mclBnG1_hashAndMapTo(&P2, "abc", 3);
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&P1, &P2));
}

CYBOZU_TEST_AUTO(mapToG2)
{
	mclBnFp2 x;
	mclBnG2 P1, P2;
	mclBnFp_setHashOf(&x.d[0], "abc", 3);
	mclBnFp_clear(&x.d[1]);
	int ret = mclBnFp2_mapToG2(&P1, &x);
	CYBOZU_TEST_ASSERT(ret == 0);
	mclBnG2_hashAndMapTo(&P2, "abc", 3);
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&P1, &P2));
}

CYBOZU_TEST_AUTO(getLittleEndian)
{
	const struct {
		const char *in;
		uint8_t out[16];
		size_t size;
	} tbl[] = {
		{ "0", { 0 }, 1 },
		{ "1", { 1 }, 1 },
		{ "0x1200", { 0x00, 0x12 }, 2 },
		{ "0x123400", { 0x00, 0x34, 0x12 }, 3 },
		{ "0x1234567890123456ab", { 0xab, 0x56, 0x34, 0x12, 0x90, 0x78, 0x56, 0x34, 0x12 }, 9 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		size_t n;
		mclBnFr x;
		CYBOZU_TEST_ASSERT(!mclBnFr_setStr(&x, tbl[i].in, strlen(tbl[i].in), 0));
		uint8_t buf[128];
		n = mclBnFr_getLittleEndian(buf, tbl[i].size, &x);
		CYBOZU_TEST_EQUAL(n, tbl[i].size);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].out, n);

		n = mclBnFr_getLittleEndian(buf, tbl[i].size + 1, &x);
		CYBOZU_TEST_EQUAL(n, tbl[i].size);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].out, n);

		n = mclBnFr_getLittleEndian(buf, tbl[i].size - 1, &x);
		CYBOZU_TEST_EQUAL(n, 0);
	}
}

CYBOZU_TEST_AUTO(mulVec)
{
	const size_t N = 70;
	mclBnG1 x1Vec[N], z1, w1;
	mclBnG2 x2Vec[N], z2, w2;
	mclBnGT xtVec[N], zt, wt;
	mclBnFr yVec[N];

	for (size_t i = 0; i < N; i++) {
		char c = char('a' + i);
		mclBnG1_hashAndMapTo(&x1Vec[i], &c, 1);
		mclBnG2_hashAndMapTo(&x2Vec[i], &c, 1);
		mclBn_pairing(&xtVec[i], &x1Vec[i], &x2Vec[i]);
		mclBnFr_setByCSPRNG(&yVec[i]);
	}
	mclBnG1_mulVec(&z1, x1Vec, yVec, N);
	mclBnG2_mulVec(&z2, x2Vec, yVec, N);
	mclBnGT_powVec(&zt, xtVec, yVec, N);

	mclBnG1_clear(&w1);
	mclBnG2_clear(&w2);
	mclBnGT_setInt(&wt, 1);
	for (size_t i = 0; i < N; i++) {
		mclBnG1 t1;
		mclBnG2 t2;
		mclBnGT tt;
		mclBnG1_mul(&t1, &x1Vec[i], &yVec[i]);
		mclBnG2_mul(&t2, &x2Vec[i], &yVec[i]);
		mclBnGT_pow(&tt, &xtVec[i], &yVec[i]);
		mclBnG1_add(&w1, &w1, &t1);
		mclBnG2_add(&w2, &w2, &t2);
		mclBnGT_mul(&wt, &wt, &tt);
	}
	CYBOZU_TEST_ASSERT(mclBnG1_isEqual(&z1, &w1));
	CYBOZU_TEST_ASSERT(mclBnG2_isEqual(&z2, &w2));
	CYBOZU_TEST_ASSERT(mclBnGT_isEqual(&zt, &wt));
}

void G1onlyTest(int curve)
{
	printf("curve=%d\n", curve);
	int ret;
	ret = mclBn_init(curve, MCLBN_COMPILED_TIME_VAR);
	CYBOZU_TEST_EQUAL(ret, 0);
	mclBnG1 P0;
	ret = mclBnG1_getBasePoint(&P0);
	CYBOZU_TEST_EQUAL(ret, 0);
	char buf[256];
	size_t n = mclBnG1_getStr(buf, sizeof(buf), &P0, 16);
	CYBOZU_TEST_ASSERT(n > 0);
	printf("basePoint=%s\n", buf);
	G1test();
}

CYBOZU_TEST_AUTO(G1only)
{
	const int tbl[] = {
		MCL_SECP192K1,
		MCL_NIST_P192,
		MCL_SECP224K1,
		MCL_NIST_P224, // hashAndMapTo is error
		MCL_SECP256K1,
		MCL_NIST_P256,
#if MCLBN_FP_UNIT_SIZE >= 6 && MCLBN_FR_UNIT_SIZE >= 6
		MCL_SECP384R1,
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		G1onlyTest(tbl[i]);
	}
}
