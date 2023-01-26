#ifndef MCL_USE_LLVM
	#define MCL_USE_LLVM
#endif
#include <cybozu/test.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/itoa.hpp>
#include "../src/low_func.hpp"
#include <cybozu/benchmark.hpp>

cybozu::XorShift rg;

extern "C" void add_test(mcl::fp::Unit *z, const mcl::fp::Unit *x, const mcl::fp::Unit *y);

template<size_t bit>
void bench()
{
	using namespace mcl::fp;
	const size_t N = bit / UnitBitSize;
	Unit x[N], y[N];
	for (int i = 0; i < 10; i++) {
		Unit z[N];
		Unit w[N];
		rg.read(x, N);
		rg.read(y, N);
		AddPre<N, Gtag>::f(z, x, y);
		AddPre<N, Ltag>::f(w, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, w, N);

		SubPre<N, Gtag>::f(z, x, y);
		SubPre<N, Ltag>::f(w, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, w, N);
	}
	const std::string bitS = cybozu::itoa(bit);
	std::string name;
	name = "add" + bitS; CYBOZU_BENCH(name.c_str(), (AddPre<N, Ltag>::f), x, x, y);
	name = "sub" + bitS; CYBOZU_BENCH(name.c_str(), (SubPre<N, Ltag>::f), x, x, y);
}

CYBOZU_TEST_AUTO(addPre64) { bench<64>(); }
CYBOZU_TEST_AUTO(addPre128) { bench<128>(); }
CYBOZU_TEST_AUTO(addPre192) { bench<192>(); }
CYBOZU_TEST_AUTO(addPre256) { bench<256>(); }
CYBOZU_TEST_AUTO(addPre320) { bench<320>(); }
CYBOZU_TEST_AUTO(addPre384) { bench<384>(); }
CYBOZU_TEST_AUTO(addPre448) { bench<448>(); }
CYBOZU_TEST_AUTO(addPre512) { bench<512>(); }
//CYBOZU_TEST_AUTO(addPre96) { bench<96>(); }
//CYBOZU_TEST_AUTO(addPre160) { bench<160>(); }
//CYBOZU_TEST_AUTO(addPre224) { bench<224>(); }
#if 0
CYBOZU_TEST_AUTO(addPre)
{
	using namespace mcl::fp;
	const size_t bit = 128;
	const size_t N = bit / UnitBitSize;
	Unit x[N], y[N];
	for (int i = 0; i < 10; i++) {
		Unit z[N];
		Unit w[N];
		rg.read(x, N);
		rg.read(y, N);
		low_addPre_G<N>(z, x, y);
		addPre<bit>(w, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, w, N);
		add_test(w, x, y);
		CYBOZU_TEST_EQUAL_ARRAY(z, w, N);
	}
	std::string name = "add" + cybozu::itoa(bit);
	CYBOZU_BENCH(name.c_str(), addPre<bit>, x, x, y);
	CYBOZU_BENCH("add", add_test, x, x, y);
}
#endif

