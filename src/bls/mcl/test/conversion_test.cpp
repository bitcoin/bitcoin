#include <cybozu/test.hpp>
#include <mcl/conversion.hpp>

CYBOZU_TEST_AUTO(arrayToDec)
{
	const struct {
		uint32_t x[6];
		size_t xn;
		const char *s;
	} tbl[] = {
		{ { 0, 0, 0, 0, 0 }, 1, "0" },
		{ { 9, 0, 0, 0, 0 }, 1, "9" },
		{ { 123456789, 0, 0, 0, 0 }, 1, "123456789" },
		{ { 2147483647, 0, 0, 0, 0 }, 1, "2147483647" },
		{ { 0xffffffff, 0, 0, 0, 0 }, 1, "4294967295" },
		{ { 0x540be400, 0x2, 0, 0, 0 }, 2, "10000000000" },
		{ { 0xffffffff, 0xffffffff, 0, 0, 0 }, 2, "18446744073709551615" },
		{ { 0x89e80001, 0x8ac72304, 0, 0, 0 }, 2, "10000000000000000001" },
		{ { 0xc582ca00, 0x8ac72304, 0, 0, 0 }, 2, "10000000001000000000" },
		{ { 0, 0, 1, 0, 0 }, 3, "18446744073709551616" },
		{ { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0 }, 4, "340282366920938463463374607431768211455" },
		{ { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }, 5, "1461501637330902918203684832716283019655932542975" },
		{ { 0x3b9aca00, 0x5e3f3700, 0x1cbfa532, 0x04f6433a, 0xd83ff078 }, 5, "1234567901234560000000000000000000000001000000000" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t bufSize = 128;
		char buf[bufSize] = {};
		const char *str = tbl[i].s;
		const uint32_t *x = tbl[i].x;
		const size_t strLen = strlen(str);
		size_t n = mcl::fp::arrayToDec(buf, bufSize, x, tbl[i].xn);
		CYBOZU_TEST_EQUAL(n, strLen);
		CYBOZU_TEST_EQUAL_ARRAY(buf + bufSize - n, str, n);
		const size_t maxN = 32;
		uint32_t xx[maxN] = {};
		size_t xn = mcl::fp::decToArray(xx, maxN, str, strLen);
		CYBOZU_TEST_EQUAL(xn, tbl[i].xn);
		CYBOZU_TEST_EQUAL_ARRAY(xx, x, xn);
	}
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t bufSize = 128;
		char buf[bufSize] = {};
		const char *str = tbl[i].s;
		const size_t strLen = strlen(str);
		uint64_t x[8] = {};
		size_t xn = (tbl[i].xn + 1) / 2;
		const uint32_t *src = tbl[i].x;
		for (size_t i = 0; i < xn; i++) {
			x[i] = (uint64_t(src[i * 2 + 1]) << 32) | src[i * 2 + 0];
		}
		size_t n = mcl::fp::arrayToDec(buf, bufSize, x, xn);
		CYBOZU_TEST_EQUAL(n, strLen);
		CYBOZU_TEST_EQUAL_ARRAY(buf + bufSize - n, str, n);
		const size_t maxN = 32;
		uint64_t xx[maxN] = {};
		size_t xxn = mcl::fp::decToArray(xx, maxN, str, strLen);
		CYBOZU_TEST_EQUAL(xxn, xn);
		CYBOZU_TEST_EQUAL_ARRAY(xx, x, xn);
	}
}

CYBOZU_TEST_AUTO(writeHexStr)
{
	const char *hex1tbl = "0123456789abcdef";
	const char *hex2tbl = "0123456789ABCDEF";
	for (size_t i = 0; i < 16; i++) {
		uint8_t v = 0xff;
		CYBOZU_TEST_ASSERT(mcl::fp::local::hexCharToUint8(&v, hex1tbl[i]));
		CYBOZU_TEST_EQUAL(v, i);
		CYBOZU_TEST_ASSERT(mcl::fp::local::hexCharToUint8(&v, hex2tbl[i]));
		CYBOZU_TEST_EQUAL(v, i);
	}
	const struct Tbl {
		const char *bin;
		size_t n;
		const char *hex;
	} tbl[] = {
		{ "", 0, "" },
		{ "\x12\x34\xab", 3, "1234ab" },
		{ "\xff\xfc\x00\x12", 4, "fffc0012" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		char buf[32];
		cybozu::MemoryOutputStream os(buf, sizeof(buf));
		const char *bin = tbl[i].bin;
		const char *hex = tbl[i].hex;
		size_t n = tbl[i].n;
		bool b;
		mcl::fp::writeHexStr(&b, os, bin, n);
		CYBOZU_TEST_ASSERT(b);
		CYBOZU_TEST_EQUAL(os.getPos(), n * 2);
		CYBOZU_TEST_EQUAL_ARRAY(buf, hex, n * 2);

		cybozu::MemoryInputStream is(hex, n * 2);
		size_t w = mcl::fp::readHexStr(buf, n, is);
		CYBOZU_TEST_EQUAL(w, n);
		CYBOZU_TEST_EQUAL_ARRAY(buf, bin, n);
	}
}
