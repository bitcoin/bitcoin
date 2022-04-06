#include <mcl/gmp_util.hpp>
#include <vector>
#include <cybozu/test.hpp>

CYBOZU_TEST_AUTO(testBit)
{
	const size_t maxBit = 100;
	const size_t tbl[] = {
		3, 9, 5, 10, 50, maxBit
	};
	mpz_class a;
	std::vector<bool> b(maxBit + 1);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		a |= mpz_class(1) << tbl[i];
		b[tbl[i]] = 1;
	}
	for (size_t i = 0; i <= maxBit; i++) {
		bool c1 = mcl::gmp::testBit(a, i);
		bool c2 = b[i] != 0;
		CYBOZU_TEST_EQUAL(c1, c2);
	}
}

CYBOZU_TEST_AUTO(getStr)
{
	const struct {
		int x;
		const char *dec;
		const char *hex;
	} tbl[] = {
		{ 0, "0", "0" },
		{ 1, "1", "1" },
		{ 10, "10", "a" },
		{ 16, "16", "10" },
		{ 123456789, "123456789", "75bcd15" },
		{ -1, "-1", "-1" },
		{ -10, "-10", "-a" },
		{ -16, "-16", "-10" },
		{ -100000000, "-100000000", "-5f5e100" },
		{ -987654321, "-987654321", "-3ade68b1" },
		{ -2147483647, "-2147483647", "-7fffffff" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		mpz_class x = tbl[i].x;
		char buf[32];
		size_t n, len;
		len = strlen(tbl[i].dec);
		n = mcl::gmp::getStr(buf, len, x, 10);
		CYBOZU_TEST_EQUAL(n, 0);
		n = mcl::gmp::getStr(buf, len + 1, x, 10);
		CYBOZU_TEST_EQUAL(n, len);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].dec, n);

		len = strlen(tbl[i].hex);
		n = mcl::gmp::getStr(buf, len, x, 16);
		CYBOZU_TEST_EQUAL(n, 0);
		n = mcl::gmp::getStr(buf, len + 1, x, 16);
		CYBOZU_TEST_EQUAL(n, len);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].hex, n);
	}
}

CYBOZU_TEST_AUTO(getRandPrime)
{
	for (int i = 0; i < 10; i++) {
		mpz_class z;
		mcl::gmp::getRandPrime(z, i * 10 + 3);
		CYBOZU_TEST_ASSERT(mcl::gmp::isPrime(z));
	}
}
