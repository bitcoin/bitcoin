#include <cybozu/test.hpp>
#include <mcl/gmp_util.hpp>
#include <stdint.h>
#include <string>
#include <cybozu/itoa.hpp>
#include <mcl/fp.hpp>
#include "../src/xbyak/xbyak_util.h"
#include "../src/fp_generator.hpp"
#include <iostream>
#include <cybozu/xorshift.hpp>
#include <cybozu/benchmark.hpp>

typedef mcl::FpT<> Fp;

const int MAX_N = 4;

const char *primeTable[] = {
#if 0
	"0x7fffffffffffffffffffffffffffffff", // 127bit(not full)
	"0xffffffffffffffffffffffffffffff61", // 128bit(full)
#endif
	"0x7fffffffffffffffffffffffffffffffffffffffffffffed", // 191bit(not full)
	"0xfffffffffffffffffffffffffffffffffffffffeffffee37", // 192bit(full)
	"0x2523648240000001ba344d80000000086121000000000013a700000000000013", // 254bit(not full)
	"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff43", // 256bit(full)
};

void strToArray(uint64_t *p, size_t n, const char *pStr)
{
	mpz_class x;
	mcl::gmp::setStr(x, pStr, 16);
	mcl::gmp::getArray(p, n, x);
}

struct Int {
	int vn;
	uint64_t v[MAX_N];
	Int()
		: vn(0)
	{
	}
	explicit Int(int vn)
	{
		if (vn > MAX_N) {
			printf("vn(%d) is too large\n", vn);
			exit(1);
		}
		this->vn = vn;
	}
	void set(const char *str) { setStr(str); }
	void set(const Fp& rhs)
	{
		mcl::gmp::getArray(v, MAX_N, rhs.getMpz());
	}
	void set(const uint64_t* x)
	{
		for (int i = 0; i < vn; i++) v[i] = x[i];
	}
	void setStr(const char *str)
	{
		strToArray(v, MAX_N, str);
	}
	std::string getStr() const
	{
		std::string ret;
		for (int i = 0; i < vn; i++) {
			ret += cybozu::itohex(v[vn - 1 - i], false);
		}
		return ret;
	}
	void put(const char *msg = "") const
	{
		if (msg) printf("%s=", msg);
		printf("%s\n", getStr().c_str());
	}
	bool operator==(const Int& rhs) const
	{
		if (vn != rhs.vn) return false;
		for (int i = 0; i < vn; i++) {
			if (v[i] != rhs.v[i]) return false;
		}
		return true;
	}
	bool operator!=(const Int& rhs) const { return !operator==(rhs); }
	bool operator==(const Fp& rhs) const
	{
		Int t(vn);
		t.set(rhs);
		return operator==(t);
	}
	bool operator!=(const Fp& rhs) const { return !operator==(rhs); }
};
static inline std::ostream& operator<<(std::ostream& os, const Int& x)
{
	return os << x.getStr();
}

void testAddSub(const mcl::fp::Op& op)
{
	Fp x, y;
	const uint64_t *p = op.p;
	Int mx(op.N), my(op.N);
	x.setStr("0x8811aabb23427cc");
	y.setStr("0x8811aabb23427cc11");
	mx.set(x);
	my.set(y);
	for (int i = 0; i < 30; i++) {
		CYBOZU_TEST_EQUAL(mx, x);
		x += x;
		op.fp_add(mx.v, mx.v, mx.v, p);
	}
	for (int i = 0; i < 30; i++) {
		CYBOZU_TEST_EQUAL(mx, x);
		x += y;
		op.fp_add(mx.v, mx.v, my.v, p);
	}
	for (int i = 0; i < 30; i++) {
		CYBOZU_TEST_EQUAL(my, y);
		y -= x;
		op.fp_sub(my.v, my.v, mx.v, p);
	}
}

void testNeg(const mcl::fp::Op& op)
{
	Fp x;
	Int mx(op.N), my(op.N);
	const char *tbl[] = {
		"0",
		"0x12346",
		"0x11223344556677881122334455667788",
		"0x0abbccddeeffaabb0000000000000000",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		x.setStr(tbl[i]);
		mx.set(x);
		x = -x;
		op.fp_neg(mx.v, mx.v, op.p);
		CYBOZU_TEST_EQUAL(mx, x);
	}
}

#if 0
void testMulI(const mcl::fp::FpGenerator& fg, int pn)
{
	cybozu::XorShift rg;
//printf("pn=%d, %p\n", pn, fg.mulUnit_);
	for (int i = 0; i < 100; i++) {
		uint64_t x[MAX_N];
		uint64_t z[MAX_N + 1];
		rg.read(x, pn);
		uint64_t y = rg.get64();
		mpz_class mx;
		mcl::gmp::setArray(mx, x, pn);
		mpz_class my;
		mcl::gmp::set(my, y);
		mx *= my;
		uint64_t d = fg.mulUnit_(z, x, y);
		z[pn] = d;
		mcl::gmp::setArray(my, z, pn + 1);
		CYBOZU_TEST_EQUAL(mx, my);
	}
	{
		uint64_t x[MAX_N];
		uint64_t z[MAX_N + 1];
		rg.read(x, pn);
		uint64_t y = rg.get64();
		CYBOZU_BENCH_C("mulUnit", 10000000, fg.mulUnit_, z, x, y);
	}
}
#endif

void testShr1(const mcl::fp::Op& op, int pn)
{
	cybozu::XorShift rg;
	for (int i = 0; i < 100; i++) {
		uint64_t x[MAX_N];
		uint64_t z[MAX_N];
		rg.read(x, pn);
		mpz_class mx;
		mcl::gmp::setArray(mx, x, pn);
		mx >>= 1;
		op.fp_shr1(z, x);
		mpz_class my;
		mcl::gmp::setArray(my, z, pn);
		CYBOZU_TEST_EQUAL(mx, my);
	}
}

void test(const char *pStr)
{
	printf("test %s\n", pStr);
	Fp::init(pStr, mcl::fp::FP_XBYAK);
	const mcl::fp::Op& op = Fp::getOp();
	const int pn = (int)op.N;
	testAddSub(op);
	testNeg(op);
//	testMulI(*op.fg, pn);
	testShr1(op, pn);
}

CYBOZU_TEST_AUTO(all)
{
	if (!mcl::fp::isEnableJIT()) return;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(primeTable); i++) {
		test(primeTable[i]);
	}
}
