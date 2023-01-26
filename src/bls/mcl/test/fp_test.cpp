#define PUT(x) std::cout << #x "=" << (x) << std::endl
#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include <mcl/fp.hpp>
#include "../src/low_func.hpp"
#include "../src/proto.hpp"
#include <time.h>
#include <cybozu/benchmark.hpp>
#include <cybozu/option.hpp>
#include <cybozu/sha2.hpp>
#include <cybozu/xorshift.hpp>

#ifdef _MSC_VER
	#pragma warning(disable: 4127) // const condition
#endif

typedef mcl::FpT<> Fp;

CYBOZU_TEST_AUTO(sizeof)
{
	CYBOZU_TEST_EQUAL(sizeof(Fp), sizeof(mcl::fp::Unit) * Fp::maxSize);
}

void cstrTest()
{
	const struct {
		const char *str;
		int val;
	} tbl[] = {
		{ "0", 0 },
		{ "1", 1 },
		{ "123", 123 },
		{ "0x123", 0x123 },
		{ "0b10101", 21 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		// string cstr
		Fp x(tbl[i].str);
		CYBOZU_TEST_EQUAL(x, tbl[i].val);

		// int cstr
		Fp y(tbl[i].val);
		CYBOZU_TEST_EQUAL(y, x);

		// copy cstr
		Fp z(x);
		CYBOZU_TEST_EQUAL(z, x);

		// assign int
		Fp w;
		w = tbl[i].val;
		CYBOZU_TEST_EQUAL(w, x);

		// assign self
		Fp u;
		u = w;
		CYBOZU_TEST_EQUAL(u, x);

		// conv
		std::ostringstream os;
		os << tbl[i].val;

		std::string str;
		x.getStr(str);
		CYBOZU_TEST_EQUAL(str, os.str());
	}
	const struct {
		const char *str;
		int val;
	} tbl2[] = {
		{ "-123", 123 },
		{ "-0x123", 0x123 },
		{ "-0b10101", 21 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl2); i++) {
		Fp x(tbl2[i].str);
		x = -x;
		CYBOZU_TEST_EQUAL(x, tbl2[i].val);
	}
}

void setStrTest()
{
	const struct {
		const char *in;
		int out;
		int base;
	} tbl[] = {
		{ "100", 100, 0 }, // set base = 10 if base = 0
		{ "100", 4, 2 },
		{ "100", 256, 16 },
		{ "0b100", 4, 0 },
		{ "0b100", 4, 2 },
		{ "0x100", 256, 0 },
		{ "0x100", 256, 16 },
		{ "0b100", 0xb100, 16 }, // hex string
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x;
		x.setStr(tbl[i].in, tbl[i].base);
		CYBOZU_TEST_EQUAL(x, tbl[i].out);
	}
	// use prefix if base conflicts with prefix
	{
		Fp x;
		CYBOZU_TEST_EXCEPTION(x.setStr("0b100", 10), cybozu::Exception);
		CYBOZU_TEST_EXCEPTION(x.setStr("0x100", 2), cybozu::Exception);
		CYBOZU_TEST_EXCEPTION(x.setStr("0x100", 10), cybozu::Exception);

		x = 1;
		std::string s;
		s.resize(Fp::getOp().N * mcl::fp::UnitBitSize, '0');
		s = "0b" + s;
		x.setStr(s, 2);
		CYBOZU_TEST_ASSERT(x.isZero());
		s += '0';
		CYBOZU_TEST_EXCEPTION(x.setStr(s, 2), cybozu::Exception);
	}
}

void streamTest()
{
	const struct {
		const char *in;
		int out10;
		int out16;
	} tbl[] = {
		{ "100", 100, 256 }, // set base = 10 if base = 0
		{ "0x100", 256, 256 },
		{ "0b100", 4, 0xb100 }, // 0b100 = 0xb100 if std::hex
	};
	Fp::setIoMode(0);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		{
			std::istringstream is(tbl[i].in);
			Fp x;
			is >> x;
			CYBOZU_TEST_EQUAL(x, tbl[i].out10);
		}
		{
			std::istringstream is(tbl[i].in);
			Fp x;
			is >> std::hex >> x;
			CYBOZU_TEST_EQUAL(x, tbl[i].out16);
		}
	}
	{
		std::ostringstream os;
		os << Fp(123);
		CYBOZU_TEST_EQUAL(os.str(), "123");
	}
	{
		std::ostringstream os;
		os << std::hex << Fp(0x123);
		CYBOZU_TEST_EQUAL(os.str(), "123");
	}
	{
		std::ostringstream os;
		os << std::hex << std::showbase << Fp(0x123);
		CYBOZU_TEST_EQUAL(os.str(), "0x123");
	}
}

void ioModeTest()
{
	Fp x(123);
	const struct {
		mcl::IoMode ioMode;
		std::string expected;
	} tbl[] = {
		{ mcl::IoBin, "1111011" },
		{ mcl::IoDec, "123" },
		{ mcl::IoHex, "7b" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp::setIoMode(tbl[i].ioMode);
		for (int j = 0; j < 2; j++) {
			std::stringstream ss;
			if (j == 1) {
				ss << std::hex;
			}
			ss << x;
			CYBOZU_TEST_EQUAL(ss.str(), tbl[i].expected);
			Fp y;
			y.clear();
			ss >> y;
			CYBOZU_TEST_EQUAL(x, y);
		}
	}
	for (int i = 0; i < 2; i++) {
		if (i == 0) {
			Fp::setIoMode(mcl::IoArray);
		} else {
			Fp::setIoMode(mcl::IoArrayRaw);
		}
		std::stringstream ss;
		ss << x;
		CYBOZU_TEST_EQUAL(ss.str().size(), Fp::getByteSize());
		Fp y;
		ss >> y;
		CYBOZU_TEST_EQUAL(x, y);
	}
	Fp::setIoMode(mcl::IoAuto);
}

void edgeTest()
{
	const mpz_class& m = Fp::getOp().mp;
	/*
		real mont
		   0    0
		   1    R^-1
		   R    1
		  -1    -R^-1
		  -R    -1
	*/
	mpz_class t = 1;
	const size_t N = Fp::getUnitSize();
	const mpz_class R = (t << (N * mcl::fp::UnitBitSize)) % m;
	const mpz_class tbl[] = {
		0, 1, R, m - 1, m - R
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const mpz_class& x = tbl[i];
		for (size_t j = i; j < CYBOZU_NUM_OF_ARRAY(tbl); j++) {
			const mpz_class& y = tbl[j];
			mpz_class z = (x * y) % m;
			Fp xx, yy;
			xx.setMpz(x);
			yy.setMpz(y);
			Fp zz = xx * yy;
			zz.getMpz(t);
			CYBOZU_TEST_EQUAL(z, t);
		}
	}
	t = m;
	t /= 2;
	Fp x;
	x.setMpz(t);
	CYBOZU_TEST_EQUAL(x * 2, -1);
	t += 1;
	x.setMpz(t);
	CYBOZU_TEST_EQUAL(x * 2, 1);
}

void convTest()
{
#if 1
	const char *bin, *hex, *dec;
	if (Fp::getBitSize() <= 117) {
		bin = "0b1000000000000000000000000000000000000000000000000000000000001110";
		hex = "0x800000000000000e";
		dec = "9223372036854775822";
	} else {
		bin = "0b100100011010001010110011110001001000000010010001101000101011001111000100100000001001000110100010101100111100010010000";
		hex = "0x123456789012345678901234567890";
		dec = "94522879687365475552814062743484560";
	}
#else
	const char *bin = "0b1001000110100";
	const char *hex = "0x1234";
	const char *dec = "4660";
#endif
	Fp b(bin);
	Fp h(hex);
	Fp d(dec);
	CYBOZU_TEST_EQUAL(b, h);
	CYBOZU_TEST_EQUAL(b, d);

	std::string str;
	b.getStr(str, mcl::IoBinPrefix);
	CYBOZU_TEST_EQUAL(str, bin);
	b.getStr(str);
	CYBOZU_TEST_EQUAL(str, dec);
	b.getStr(str, mcl::IoHexPrefix);
	CYBOZU_TEST_EQUAL(str, hex);
}

void compareTest()
{
	{
		const struct {
			int lhs;
			int rhs;
			int cmp;
		} tbl[] = {
			{ 0, 0, 0 },
			{ 1, 0, 1 },
			{ 0, 1, -1 },
			{ -1, 0, 1 }, // m-1, 0
			{ 0, -1, -1 }, // 0, m-1
			{ 123, 456, -1 },
			{ 456, 123, 1 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const Fp x(tbl[i].lhs);
			const Fp y(tbl[i].rhs);
			const int cmp = tbl[i].cmp;
			if (cmp == 0) {
				CYBOZU_TEST_EQUAL(x, y);
				CYBOZU_TEST_ASSERT(x >= y);
				CYBOZU_TEST_ASSERT(x <= y);
			} else if (cmp > 0) {
				CYBOZU_TEST_ASSERT(x > y);
				CYBOZU_TEST_ASSERT(x >= y);
			} else {
				CYBOZU_TEST_ASSERT(x < y);
				CYBOZU_TEST_ASSERT(x <= y);
			}
		}
	}
	{
		Fp x(5);
		CYBOZU_TEST_ASSERT(x < 10);
		CYBOZU_TEST_ASSERT(x == 5);
		CYBOZU_TEST_ASSERT(x > 2);
	}
	{
		Fp x(1);
		CYBOZU_TEST_ASSERT(x.isOne());
		x = 2;
		CYBOZU_TEST_ASSERT(!x.isOne());
	}
	{
		const struct {
			int v;
			bool expected;
		} tbl[] = {
			{ 0, false },
			{ 1, false },
			{ -1, true },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			Fp x = tbl[i].v;
			CYBOZU_TEST_EQUAL(x.isNegative(), tbl[i].expected);
		}
		std::string str;
		Fp::getModulo(str);
		char buf[1024];
		size_t n = Fp::getModulo(buf, sizeof(buf));
		CYBOZU_TEST_EQUAL(n, str.size());
		CYBOZU_TEST_EQUAL(buf, str.c_str());
		mpz_class half(str);
		half = (half - 1) / 2;
		Fp x;
		x.setMpz(half - 1);
		CYBOZU_TEST_ASSERT(!x.isNegative());
		x.setMpz(half);
		CYBOZU_TEST_ASSERT(!x.isNegative());
		x.setMpz(half + 1);
		CYBOZU_TEST_ASSERT(x.isNegative());
	}
}

void moduloTest(const char *pStr)
{
	std::string str;
	Fp::getModulo(str);
	CYBOZU_TEST_EQUAL(str, mcl::gmp::getStr(mpz_class(pStr)));
}

void opeTest()
{
	const struct {
		int x;
		int y;
		int add; // x + y
		int sub; // x - y
		int mul; // x * y
		int sqr; // x^2
	} tbl[] = {
		{ 0, 1, 1, -1, 0, 0 },
		{ 9, 5, 14, 4, 45, 81 },
		{ 10, 13, 23,  -3, 130, 100 },
		{ 2000, 1000, 3000, 1000, 2000 * 1000, 2000 * 2000 },
		{ 12345, 9999, 12345 + 9999, 12345 - 9999, 12345 * 9999, 12345 * 12345 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const Fp x(tbl[i].x);
		const Fp y(tbl[i].y);
		Fp z;
		Fp::add(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].add);
		Fp::sub(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].sub);
		Fp::mul(z, x, y);
		CYBOZU_TEST_EQUAL(z, tbl[i].mul);

		Fp r;
		Fp::inv(r, y);
		Fp::mul(z, z, r);
		CYBOZU_TEST_EQUAL(z, tbl[i].x);
		z = x + y;
		CYBOZU_TEST_EQUAL(z, tbl[i].add);
		z = x - y;
		CYBOZU_TEST_EQUAL(z, tbl[i].sub);
		z = x * y;
		CYBOZU_TEST_EQUAL(z, tbl[i].mul);

		Fp::sqr(z, x);
		CYBOZU_TEST_EQUAL(z, tbl[i].sqr);

		z = x / y;
		z *= y;
		CYBOZU_TEST_EQUAL(z, tbl[i].x);
	}
	if (!Fp::isFullBit()) {
		Fp x(5), y(3), z;
		Fp::addPre(z, x, y);
		if (Fp::compareRaw(z, Fp::getP()) >= 0) {
			Fp::subPre(z, z, Fp::getP());
		}
		CYBOZU_TEST_EQUAL(z, Fp(8));
		if (Fp::compareRaw(x, y) < 0) {
			Fp::addPre(x, x, Fp::getP());
		}
		Fp::subPre(x, x, y);
		CYBOZU_TEST_EQUAL(x, Fp(2));
	}
}

struct tag2;

void powTest()
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	for (int i = 0; i < 100; i++) {
		Fp::pow(y, x, i);
		CYBOZU_TEST_EQUAL(y, z);
		z *= x;
	}
	x = z;
	Fp::pow(z, x, Fp::getOp().mp - 1);
	CYBOZU_TEST_EQUAL(z, 1);
	Fp::pow(z, x, Fp::getOp().mp);
	CYBOZU_TEST_EQUAL(z, x);
#if 0
	typedef mcl::FpT<tag2, 128> Fp_other;
	Fp_other::init("1009");
	x = 5;
	Fp_other n = 3;
	z = 3;
	Fp::pow(x, x, z);
	CYBOZU_TEST_EQUAL(x, 125);
	x = 5;
	Fp::pow(x, x, n);
	CYBOZU_TEST_EQUAL(x, 125);
#endif
}

void mulUnitTest()
{
	Fp x(-1), y, z;
	for (unsigned int u = 0; u < 20; u++) {
		Fp::mul(y, x, u);
		Fp::mulUnit(z, x, u);
		CYBOZU_TEST_EQUAL(y, z);
	}
}

void powNegTest()
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	Fp rx = 1 / x;
	for (int i = 0; i < 100; i++) {
		Fp::pow(y, x, -i);
		CYBOZU_TEST_EQUAL(y, z);
		z *= rx;
	}
}

void powFpTest()
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	for (int i = 0; i < 100; i++) {
		Fp::pow(y, x, Fp(i));
		CYBOZU_TEST_EQUAL(y, z);
		z *= x;
	}
}

void powGmp()
{
	Fp x, y, z;
	x = 12345;
	z = 1;
	for (int i = 0; i < 100; i++) {
		Fp::pow(y, x, mpz_class(i));
		CYBOZU_TEST_EQUAL(y, z);
		z *= x;
	}
}

struct TagAnother;

#if 0
void anotherFpTest(mcl::fp::Mode mode)
{
	typedef mcl::FpT<TagAnother, 128> G;
	G::init("13", mode);
	G a = 3;
	G b = 9;
	a *= b;
	CYBOZU_TEST_EQUAL(a, 1);
}
#endif

void setArrayTest1()
{
	uint8_t b1[] = { 0x56, 0x34, 0x12 };
	Fp x;
	x.setArray(b1, 3);
	CYBOZU_TEST_EQUAL(x, 0x123456);
	uint32_t b2[] = { 0x12, 0x34 };
	x.setArray(b2, 2);
	CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
}

#if 0
void setArrayTest2(mcl::fp::Mode mode)
{
	Fp::init("0x10000000000001234567a5", mode);
	const struct {
		uint32_t buf[3];
		size_t bufN;
		const char *expected;
	} tbl[] = {
		{ { 0x234567a4, 0x00000001, 0x00100000}, 1, "0x234567a4" },
		{ { 0x234567a4, 0x00000001, 0x00100000}, 2, "0x1234567a4" },
		{ { 0x234567a4, 0x00000001, 0x00080000}, 3, "0x08000000000001234567a4" },
		{ { 0x234567a4, 0x00000001, 0x00100000}, 3, "0x10000000000001234567a4" },
	};
	Fp x;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		x.setArray(tbl[i].buf, tbl[i].bufN);
		CYBOZU_TEST_EQUAL(x, Fp(tbl[i].expected));
	}
	uint32_t large[3] = { 0x234567a5, 0x00000001, 0x00100000};
	CYBOZU_TEST_EXCEPTION(x.setArray(large, 3), cybozu::Exception);
}
#endif

void setArrayMaskTest1()
{
	uint8_t b1[] = { 0x56, 0x34, 0x12 };
	Fp x;
	x.setArrayMask(b1, 3);
	CYBOZU_TEST_EQUAL(x, 0x123456);
	uint32_t b2[] = { 0x12, 0x34 };
	x.setArrayMask(b2, 2);
	CYBOZU_TEST_EQUAL(x, Fp("0x3400000012"));
}

#if 0
void setArrayMaskTest2(mcl::fp::Mode mode)
{
	Fp::init("0x10000000000001234567a5", mode);
	const struct {
		uint32_t buf[3];
		size_t bufN;
		const char *expected;
	} tbl[] = {
		{ { 0x234567a4, 0x00000001, 0x00100000}, 1, "0x234567a4" },
		{ { 0x234567a4, 0x00000001, 0x00100000}, 2, "0x1234567a4" },
		{ { 0x234567a4, 0x00000001, 0x00100000}, 3, "0x10000000000001234567a4" },
		{ { 0x234567a5, 0xfffffff1, 0xffffffff}, 3, "0x0ffffffffffff1234567a5" },
	};
	Fp x;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		x.setArrayMask(tbl[i].buf, tbl[i].bufN);
		CYBOZU_TEST_EQUAL(x, Fp(tbl[i].expected));
	}
}
#endif

void setArrayModTest()
{
	const mpz_class& p = Fp::getOp().mp;
	mpz_class tbl[] = {
		0, // max
		0,
		1,
		p - 1,
		p,
		p + 1,
		p * 2 - 1,
		p * 2,
		p * 2 + 1,
		p * (p - 1) - 1,
		p * (p - 1),
		p * (p - 1) + 1,
		p * p - 1,
		p * p,
		p * p + 1,
	};
	std::string maxStr(mcl::gmp::getBitSize(p) * 2, '1');
	mcl::gmp::setStr(tbl[0], maxStr, 2);
	const size_t unitByteSize = sizeof(mcl::fp::Unit);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const mpz_class& x = tbl[i];
		const mcl::fp::Unit *px = mcl::gmp::getUnit(x);
		const size_t xn = mcl::gmp::getUnitSize(x);
		const size_t xByteSize = xn * unitByteSize;
		const size_t fpByteSize = unitByteSize * Fp::getOp().N;
		Fp y;
		bool b;
		y.setArrayMod(&b, px, xn);
		bool expected = xByteSize <= fpByteSize * 2;
		CYBOZU_TEST_EQUAL(b, expected);
		if (!b) continue;
		CYBOZU_TEST_EQUAL(y.getMpz(), x % p);
	}
}

CYBOZU_TEST_AUTO(set64bit)
{
	Fp::init("3138550867693340381917894711603833208051177722232017256453");
	const struct {
		const char *p;
		int64_t i;
	} tbl[] = {
		{ "0x1234567812345678", int64_t(0x1234567812345678ull) },
		{ "-5", -5 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i].p);
		Fp y(tbl[i].i);
		CYBOZU_TEST_EQUAL(x, y);
	}
}

void getUint64Test()
{
	const uint64_t tbl[] = {
		0, 1, 123, 0xffffffff, int64_t(0x7fffffffffffffffull)
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		uint64_t a = tbl[i];
		Fp x(a);
		uint64_t b = x.getUint64();
		CYBOZU_TEST_ASSERT(!x.isNegative());
		CYBOZU_TEST_EQUAL(a, b);
	}
	{
		Fp x("0xffffffffffffffff");
		CYBOZU_TEST_EQUAL(x.getUint64(), uint64_t(0xffffffffffffffffull));
	}
	{
		Fp x("0x10000000000000000");
		CYBOZU_TEST_EXCEPTION(x.getUint64(), cybozu::Exception);
		x = -1;
		CYBOZU_TEST_EXCEPTION(x.getUint64(), cybozu::Exception);
	}
	{
		Fp x("0x10000000000000000");
		bool b = true;
		CYBOZU_TEST_EQUAL(x.getUint64(&b), 0u);
		CYBOZU_TEST_ASSERT(!b);
	}
}

void getInt64Test()
{
	const int64_t tbl[] = {
		0, 1, 123, 0xffffffff, int64_t(0x7fffffffffffffffull),
		-1, -2, -12345678, -int64_t(0x7fffffffffffffffull) - 1/*-int64_t(1) << 63*/,
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		int64_t a = tbl[i];
		Fp x(a);
		CYBOZU_TEST_EQUAL(x.isNegative(), a < 0);
		int64_t b = x.getInt64();
		CYBOZU_TEST_EQUAL(a, b);
	}
	{
		Fp x("0x8000000000000000");
		CYBOZU_TEST_EXCEPTION(x.getInt64(), cybozu::Exception);
	}
	{
		Fp x("0x8000000000000000");
		bool b = true;
		CYBOZU_TEST_EQUAL(x.getInt64(&b), 0u);
		CYBOZU_TEST_ASSERT(!b);
	}
}

void getLittleEndianTest()
{
	if (Fp::getOp().bitSize < 80) return;
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
		Fp x(tbl[i].in);
		uint8_t buf[128];
		size_t n = x.getLittleEndian(buf, tbl[i].size);
		CYBOZU_TEST_EQUAL(n, tbl[i].size);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].out, n);

		n = x.getLittleEndian(buf, tbl[i].size + 1);
		CYBOZU_TEST_EQUAL(n, tbl[i].size);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].out, n);

		n = x.getLittleEndian(buf, tbl[i].size - 1);
		CYBOZU_TEST_EQUAL(n, 0);
	}
}

void divBy2Test()
{
	const int tbl[] = { -4, -3, -2, -1, 0, 1, 2, 3 };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x(tbl[i]), y;
		Fp::divBy2(y, x);
		y *= 2;
		CYBOZU_TEST_EQUAL(y, x);
		y = x;
		Fp::divBy2(y, y);
		y *= 2;
		CYBOZU_TEST_EQUAL(y, x);
	}
}

void getStrTest()
{
	Fp x(0);
	std::string str;
	str = x.getStr();
	CYBOZU_TEST_EQUAL(str, "0");
	str = x.getStr(mcl::IoBinPrefix);
	CYBOZU_TEST_EQUAL(str, "0b0");
	str = x.getStr(mcl::IoBin);
	CYBOZU_TEST_EQUAL(str, "0");
	str = x.getStr(mcl::IoHexPrefix);
	CYBOZU_TEST_EQUAL(str, "0x0");
	str = x.getStr(mcl::IoHex);
	CYBOZU_TEST_EQUAL(str, "0");

	x = 123;
	str = x.getStr();
	CYBOZU_TEST_EQUAL(str, "123");
	str = x.getStr(mcl::IoBinPrefix);
	CYBOZU_TEST_EQUAL(str, "0b1111011");
	str = x.getStr(mcl::IoBin);
	CYBOZU_TEST_EQUAL(str, "1111011");
	str = x.getStr(mcl::IoHexPrefix);
	CYBOZU_TEST_EQUAL(str, "0x7b");
	str = x.getStr(mcl::IoHex);
	CYBOZU_TEST_EQUAL(str, "7b");

	{
		std::ostringstream os;
		os << x;
		CYBOZU_TEST_EQUAL(os.str(), "123");
	}
	{
		std::ostringstream os;
		os << std::hex << std::showbase << x;
		CYBOZU_TEST_EQUAL(os.str(), "0x7b");
	}
	{
		std::ostringstream os;
		os << std::hex << x;
		CYBOZU_TEST_EQUAL(os.str(), "7b");
	}
	const char *tbl[] = {
		"0x0",
		"0x5",
		"0x123",
		"0x123456789012345679adbc",
		"0xffffffff26f2fc170f69466a74defd8d",
		"0x100000000000000000000000000000033",
		"0x11ee12312312940000000000000000000000000002342343"
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const char *s = tbl[i];
		mpz_class mx(s);
		if (mx >= Fp::getOp().mp) continue;
		Fp y(s);
		std::string xs, ys;
		mcl::gmp::getStr(xs, mx, 16);
		y.getStr(ys, 16);
		CYBOZU_TEST_EQUAL(xs, ys);
	}
}

void setHashOfTest()
{
	const std::string msgTbl[] = {
		"", "abc", "111111111111111111111111111111111111",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(msgTbl); i++) {
		size_t bitSize = Fp::getBitSize();
		std::string digest;
		if (bitSize <= 256) {
			digest = cybozu::Sha256().digest(msgTbl[i]);
		} else {
			digest = cybozu::Sha512().digest(msgTbl[i]);
		}
		Fp x, y;
		x.setArrayMask((const uint8_t*)digest.c_str(), digest.size());
		y.setHashOf(msgTbl[i]);
		CYBOZU_TEST_EQUAL(x, y);
	}
}

CYBOZU_TEST_AUTO(getArray)
{
	const struct {
		const char *s;
		uint32_t v[4];
		size_t vn;
	} tbl[] = {
		{ "0", { 0, 0, 0, 0 }, 1 },
		{ "1234", { 1234, 0, 0, 0 }, 1 },
		{ "0xaabbccdd12345678", { 0x12345678, 0xaabbccdd, 0, 0 }, 2 },
		{ "0x11112222333344445555666677778888", { 0x77778888, 0x55556666, 0x33334444, 0x11112222 }, 4 },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		mpz_class x(tbl[i].s);
		const size_t bufN = 8;
		uint32_t buf[bufN];
		mcl::gmp::getArray(buf, bufN, x);
		size_t n = mcl::fp::getNonZeroArraySize(buf, bufN);
		CYBOZU_TEST_EQUAL(n, tbl[i].vn);
		CYBOZU_TEST_EQUAL_ARRAY(buf, tbl[i].v, n);
	}
}

void serializeTest()
{
	const char *tbl[] = { "0", "-1", "123" };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		char buf[1024];
		Fp x, y;
		x.setStr(tbl[i]);
		size_t n = x.serialize(buf, sizeof(buf));
		CYBOZU_TEST_EQUAL(n, Fp::getByteSize());
		y.deserialize(buf, n);
		CYBOZU_TEST_EQUAL(x, y);

		n = x.serialize(buf, sizeof(buf), mcl::IoSerializeHexStr);
		CYBOZU_TEST_EQUAL(n, Fp::getByteSize() * 2);
		y.deserialize(buf, n, mcl::IoSerializeHexStr);
		CYBOZU_TEST_EQUAL(x, y);
	}
}

void modpTest()
{
	const mpz_class& p = Fp::getOp().mp;
	mpz_class tbl[] = {
		0, // max
		0,
		1,
		p - 1,
		p,
		p + 1,
		p * 2 - 1,
		p * 2,
		p * 2 + 1,
		p * (p - 1) - 1,
		p * (p - 1),
		p * (p - 1) + 1,
		p * p - 1,
		p * p,
		p * p + 1,
	};
	std::string maxStr(mcl::gmp::getBitSize(p) * 2, '1');
	mcl::gmp::setStr(tbl[0], maxStr, 2);
	mcl::Modp modp;
	modp.init(p);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const mpz_class& x = tbl[i];
		mpz_class r1, r2;
		r1 = x % p;
		modp.modp(r2, x);
		CYBOZU_TEST_EQUAL(r1, r2);
	}
}

#include <iostream>
#if (defined(MCL_USE_LLVM) || defined(MCL_X64_ASM)) && (MCL_MAX_BIT_SIZE >= 521)
CYBOZU_TEST_AUTO(mod_NIST_P521)
{
	const size_t len = 521;
	const size_t N = len / mcl::fp::UnitBitSize;
	const char *tbl[] = {
		"0",
		"0xffffffff",
		"0x1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0",
		"0x1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe",
		"0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
		"0x20000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
		"0x20000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff00111423424",
		"0x11111111111111112222222222222222333333333333333344444444444444445555555555555555666666666666666677777777777777778888888888888888aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbccccccccccccccccddddddddddddddddeeeeeeeeeeeeeeeeffffffffffffffff1234712341234123412341234123412341234",
		"0x3ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
	};
	const char *p = "0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
	Fp::init(p, mcl::fp::FP_XBYAK);
	const mpz_class mp(p);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		mpz_class mx(tbl[i]);
		mcl::fp::Unit in[N * 2 + 1] = {};
		mcl::fp::Unit ok[N + 1];
		mcl::fp::Unit ex[N + 1];
		mcl::gmp::getArray(in, N * 2 + 1, mx);
		mpz_class my = mx % mp;
		mcl::gmp::getArray(ok, N + 1, my);
#ifdef MCL_USE_LLVM
		mcl_fpDbl_mod_NIST_P521L(ex, in, Fp::getOp().p);
		CYBOZU_TEST_EQUAL_ARRAY(ex, ok, N + 1);
#endif
#ifdef MCL_X64_ASM
		const mcl::fp::Op& op = Fp::getOp();
		if (!op.isMont) {
			op.fpDbl_mod(ex, in, op.p);
			CYBOZU_TEST_EQUAL_ARRAY(ex, ok, N + 1);
		}
#endif
	}
}
#endif

void mul2Test()
{
	const int x0 = 1234567;
	Fp x = x0;
	mpz_class mx = x0;
	for (size_t i = 0; i < 100; i++) {
		Fp::mul2(x, x);
		mx = (mx * 2) % Fp::getOp().mp;
		CYBOZU_TEST_EQUAL(mx, x.getMpz());
	}
}

void invVecTest()
{
	const size_t maxN = 10;
	Fp x[maxN], y[maxN];
	cybozu::XorShift rg;
	for (size_t n = 0; n < maxN; n++) {
		for (int j = 0; j < 10; j++) {
			for (size_t i = 0; i < n; i++) {
				if ((j != 0 && (rg.get32() % 3) == 0) || j == 1) {
					x[i].clear();
				} else {
					x[i].setByCSPRNG(rg);
				}
				y[i] = 1;
			}
			Fp::invVec(y, x, n);
			for (size_t i = 0; i < n; i++) {
				if (x[i].isZero()) {
					CYBOZU_TEST_ASSERT(y[i].isZero());
				} else {
					CYBOZU_TEST_EQUAL(y[i], 1 / x[i]);
				}
			}
			Fp::invVec(x, x, n); // same addr
			CYBOZU_TEST_EQUAL_ARRAY(y, x, n);
		}
	}
}

void sub(mcl::fp::Mode mode)
{
	printf("mode=%s\n", mcl::fp::ModeToStr(mode));
	const char *tbl[] = {
		// N = 3
		"0x30000000000000000000000000000000000000000000002b",
		"0x70000000000000000000000000000000000000000000001f",
		"0x800000000000000000000000000000000000000000000005",
		"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
		"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d",
		"0xffffffffffffffffffffffffffffffffffffffffffffff13", // max prime

		// N = 4
		"0x0000000000000001000000000000000000000000000000000000000000000085", // min prime
		"0x2523648240000001ba344d80000000086121000000000013a700000000000013",
		"0x7523648240000001ba344d80000000086121000000000013a700000000000017",
		"0x800000000000000000000000000000000000000000000000000000000000005f",
		"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f", // secp256k1
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff43", // max prime
#if MCL_MAX_BIT_SIZE >= 384

		// N = 6
		"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
#endif

#if MCL_MAX_BIT_SIZE >= 521
		// N = 9
		"0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const char *pStr = tbl[i];
		printf("prime=%s\n", pStr);
		Fp::init(pStr, mode);
		invVecTest();
		mul2Test();
		cstrTest();
		setStrTest();
		streamTest();
		ioModeTest();
		edgeTest();
		convTest();
		compareTest();
		moduloTest(pStr);
		opeTest();
		mulUnitTest();
		powTest();
		powNegTest();
		powFpTest();
		powGmp();
		setArrayTest1();
		setArrayMaskTest1();
		setArrayModTest();
		getUint64Test();
		getInt64Test();
		getLittleEndianTest();
		divBy2Test();
		getStrTest();
		setHashOfTest();
		serializeTest();
		modpTest();
	}
//	anotherFpTest(mode);
//	setArrayTest2(mode);
//	setArrayMaskTest2(mode);
}

std::string g_mode;

CYBOZU_TEST_AUTO(main)
{
	if (g_mode.empty() || g_mode == "auto") {
		sub(mcl::fp::FP_AUTO);
	}
	if (g_mode.empty() || g_mode == "gmp") {
		sub(mcl::fp::FP_GMP);
	}
	if (g_mode.empty() || g_mode == "gmp_mont") {
		sub(mcl::fp::FP_GMP_MONT);
	}
#ifdef MCL_USE_LLVM
	if (g_mode.empty() || g_mode == "llvm") {
		sub(mcl::fp::FP_LLVM);
	}
	if (g_mode.empty() || g_mode == "llvm_mont") {
		sub(mcl::fp::FP_LLVM_MONT);
	}
#endif
#ifdef MCL_X64_ASM
	if (g_mode.empty() || g_mode == "xbyak") {
		sub(mcl::fp::FP_XBYAK);
	}
#endif
}

CYBOZU_TEST_AUTO(convertArrayAsLE)
{
	using namespace mcl::fp;
#if MCL_SIZEOF_UNIT == 4
	const Unit src[] = { 0x12345678, 0xaabbccdd, 0xffeeddcc, 0x87654321 };
#else
	const Unit src[] = { uint64_t(0xaabbccdd12345678ull), uint64_t(0x87654321ffeeddcc) };
#endif
	const uint8_t ok[] = { 0x78, 0x56, 0x34, 0x12, 0xdd, 0xcc, 0xbb, 0xaa, 0xcc, 0xdd, 0xee, 0xff, 0x21, 0x43, 0x65, 0x87 };
	const size_t dstN = 2;
	mcl::fp::Unit dst[dstN];
	for (size_t i = 1; i <= sizeof(dst); i++) {
		memset(dst, 0xff, sizeof(dst));
		mcl::fp::convertArrayAsLE(dst, dstN, ok, i);
		if (i < sizeof(Unit)) {
			CYBOZU_TEST_EQUAL(src[0] & ((uint64_t(1) << (i * 8)) - 1), dst[0]);
			CYBOZU_TEST_EQUAL(dst[1], 0u);
			continue;
		}
		CYBOZU_TEST_EQUAL(dst[0], src[0]);
		if (i == sizeof(Unit)) {
			CYBOZU_TEST_EQUAL(dst[1], 0u);
			continue;
		}
		if (i < sizeof(dst)) {
			CYBOZU_TEST_EQUAL(src[1] & ((uint64_t(1) << ((i - sizeof(Unit)) * 8)) - 1), dst[1]);
			continue;
		}
		CYBOZU_TEST_EQUAL(src[1], dst[1]);
	}
	dst[0] = 1;
	mcl::fp::convertArrayAsLE(dst, 0, ok, 1);
	CYBOZU_TEST_EQUAL(dst[0], 1u);
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	opt.appendOpt(&g_mode, "", "m", ": mode(auto/gmp/gmp_mont/llvm/llvm_mont/xbyak)");
	opt.appendHelp("h", ": show this message");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	return cybozu::test::autoRun.run(argc, argv);
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}
