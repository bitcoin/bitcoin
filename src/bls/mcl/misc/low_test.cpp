#include <stdio.h>
#include <stdint.h>

void dump(const char *msg, const uint32_t *x, size_t n)
{
	printf("%s", msg);
	for (size_t i = 0; i < n; i++) {
		printf("%08x", x[n - 1 - i]);
	}
	printf("\n");
}
#include "../src/low_func_wasm.hpp"

#define MCL_USE_VINT
#define MCL_VINT_FIXED_BUFFER
#define MCL_SIZEOF_UNIT 4
#define MCL_MAX_BIT_SIZE 768
#include <mcl/vint.hpp>
#include <cybozu/test.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/benchmark.hpp>
#include <mcl/util.hpp>

const int C = 10000;

template<class RG>
void setRand(uint32_t *x, size_t n, RG& rg)
{
	for (size_t i = 0; i < n; i++) {
		x[i] = rg.get32();
	}
}

/*
g++ -Ofast -DNDEBUG -Wall -Wextra -m32 -I ./include/ misc/low_test.cpp
Core i7-8700
         mulT  karatsuba
N =  6, 182clk   225clk
N =  8, 300clk   350clk
N = 12, 594clk   730clk
*/
template<size_t N>
void mulTest()
{
	printf("N=%zd (%zdbit)\n", N, N * 32);
	cybozu::XorShift rg;
	uint32_t x[N];
	uint32_t y[N];
	uint32_t z[N * 2];
	for (size_t i = 0; i < 1000; i++) {
		setRand(x, N, rg);
		setRand(y, N, rg);
		// remove MSB
		x[N - 1] &= 0x7fffffff;
		y[N - 1] &= 0x7fffffff;
		mcl::Vint vx, vy;
		vx.setArray(x, N);
		vy.setArray(y, N);
		vx *= vy;
		mcl::mulT<N>(z, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, vx.getUnit(), N * 2);
		memset(z, 0, sizeof(z));
		mcl::karatsubaT<N>(z, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, vx.getUnit(), N * 2);
	}
	CYBOZU_BENCH_C("mulT", C, mcl::mulT<N>, z, x, y);
	CYBOZU_BENCH_C("kara", C, mcl::karatsubaT<N>, z, x, y);
}

CYBOZU_TEST_AUTO(mulT)
{
	mulTest<8>();
	mulTest<12>();
}

template<size_t N>
void sqrTest()
{
	printf("N=%zd (%zdbit)\n", N, N * 32);
	cybozu::XorShift rg;
	uint32_t x[N];
	uint32_t y[N * 2];
	for (size_t i = 0; i < 1000; i++) {
		setRand(x, N, rg);
		// remove MSB
		x[N - 1] &= 0x7fffffff;
		mcl::Vint vx;
		vx.setArray(x, N);
		vx *= vx;
		mcl::sqrT<N>(y, x);
		CYBOZU_TEST_EQUAL_ARRAY(y, vx.getUnit(), N * 2);
	}
	CYBOZU_BENCH_C("sqrT", C, mcl::sqrT<N>, y, x);
}

CYBOZU_TEST_AUTO(sqrT)
{
	sqrTest<8>();
	sqrTest<12>();
}

struct Montgomery {
	mcl::Vint p_;
	mcl::Vint R_; // (1 << (pn_ * 64)) % p
	mcl::Vint RR_; // (R * R) % p
	uint32_t rp_; // rp * p = -1 mod M = 1 << 64
	size_t pn_;
	Montgomery() {}
	explicit Montgomery(const mcl::Vint& p)
	{
		p_ = p;
		rp_ = mcl::fp::getMontgomeryCoeff(p.getUnit()[0]);
		pn_ = p.getUnitSize();
		R_ = 1;
		R_ = (R_ << (pn_ * 64)) % p_;
		RR_ = (R_ * R_) % p_;
	}

	void toMont(mcl::Vint& x) const { mul(x, x, RR_); }
	void fromMont(mcl::Vint& x) const { mul(x, x, 1); }

	void mul(mcl::Vint& z, const mcl::Vint& x, const mcl::Vint& y) const
	{
		const size_t ySize = y.getUnitSize();
		mcl::Vint c = x * y.getUnit()[0];
		uint32_t q = c.getUnit()[0] * rp_;
		c += p_ * q;
		c >>= sizeof(uint32_t) * 8;
		for (size_t i = 1; i < pn_; i++) {
			if (i < ySize) {
				c += x * y.getUnit()[i];
			}
			uint32_t q = c.getUnit()[0] * rp_;
			c += p_ * q;
			c >>= sizeof(uint32_t) * 8;
		}
		if (c >= p_) {
			c -= p_;
		}
		z = c;
	}
	void mod(mcl::Vint& z, const mcl::Vint& xy) const
	{
		z = xy;
		for (size_t i = 0; i < pn_; i++) {
			uint32_t q = z.getUnit()[0] * rp_;
			mcl::Vint t = q;
			z += p_ * t;
			z >>= 32;
		}
		if (z >= p_) {
			z -= p_;
		}
	}
};

template<size_t N>
void mulMontTest(const char *pStr)
{
	mcl::Vint vp;
	vp.setStr(pStr);
	Montgomery mont(vp);

	cybozu::XorShift rg;
	uint32_t x[N];
	uint32_t y[N];
	uint32_t z[N];
	uint32_t _p[N + 1];
	uint32_t *const p = _p + 1;
	vp.getArray(p, N);
	p[-1] = mont.rp_;

	for (size_t i = 0; i < 1000; i++) {
		setRand(x, N, rg);
		setRand(y, N, rg);
		// remove MSB
		x[N - 1] &= 0x7fffffff;
		y[N - 1] &= 0x7fffffff;
		mcl::Vint vx, vy, vz;
		vx.setArray(x, N);
		vy.setArray(y, N);
		mont.mul(vz, vx, vy);
		mcl::mulMontT<N>(z, x, y, p);
		CYBOZU_TEST_EQUAL_ARRAY(z, vz.getUnit(), N);

		mont.mul(vz, vx, vx);
		mcl::sqrMontT<N>(z, x, p);
		CYBOZU_TEST_EQUAL_ARRAY(z, vz.getUnit(), N);
	}
	CYBOZU_BENCH_C("mulMontT", C, mcl::mulMontT<N>, x, x, y, p);
	CYBOZU_BENCH_C("sqrMontT", C, mcl::sqrMontT<N>, x, x, p);
}

template<size_t N>
void modTest(const char *pStr)
{
	mcl::Vint vp;
	vp.setStr(pStr);
	Montgomery mont(vp);

	cybozu::XorShift rg;
	uint32_t xy[N * 2];
	uint32_t z[N];
	uint32_t _p[N + 1];
	uint32_t *const p = _p + 1;
	vp.getArray(p, N);
	p[-1] = mont.rp_;

	for (size_t i = 0; i < 1000; i++) {
		setRand(xy, N * 2, rg);
		// remove MSB
		xy[N * 2 - 1] &= 0x7fffffff;
		mcl::Vint vxy, vz;
		vxy.setArray(xy, N * 2);
		mont.mod(vz, vxy);
		mcl::modT<N>(z, xy, p);
		CYBOZU_TEST_EQUAL_ARRAY(z, vz.getUnit(), N);
	}
	CYBOZU_BENCH_C("modT", C, mcl::modT<N>, z, xy, p);
}

CYBOZU_TEST_AUTO(mont)
{
	const char *pBN254 = "0x2523648240000001ba344d80000000086121000000000013a700000000000013";
	puts("BN254");
	mulMontTest<8>(pBN254);
	modTest<8>(pBN254);

	const char *pBLS12_381 = "0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab";
	puts("BLS12");
	mulMontTest<12>(pBLS12_381);
	modTest<12>(pBLS12_381);
}

