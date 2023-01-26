#include <mcl/gmp_util.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/test.hpp>

#define PUT(x) std::cout << #x << "=" << x << std::endl;

CYBOZU_TEST_AUTO(modp)
{
	const int C = 1000000;
	const char *pTbl[] = {
		"0x2523648240000001ba344d8000000007ff9f800000000010a10000000000000d",
		"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
		"0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001",
	};
	const char *xTbl[] = {
		"0x12345678892082039482094823",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
		"0x10000000000000000000000000000000000000000000000000000000000000000",
		"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
	};
	mcl::Modp modp;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(pTbl); i++) {
		const mpz_class p(pTbl[i]);
		std::cout << std::hex << "p=" << p << std::endl;
		modp.init(p);
		for (size_t j = 0; j < CYBOZU_NUM_OF_ARRAY(xTbl); j++) {
			const mpz_class x(xTbl[j]);
			std::cout << std::hex << "x=" << x << std::endl;
			mpz_class r1, r2;
			r1 = x % p;
			modp.modp(r2, x);
			CYBOZU_TEST_EQUAL(r1, r2);
			CYBOZU_BENCH_C("x % p", C, mcl::gmp::mod, r1, x, p);
			CYBOZU_BENCH_C("modp ", C, modp.modp, r2, x);
		}
	}
}
