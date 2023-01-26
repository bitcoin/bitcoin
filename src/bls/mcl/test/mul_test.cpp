#define PUT(x) std::cout << #x "=" << (x) << std::endl;
#include <mcl/fp.hpp>
#include <mcl/ecparam.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>

struct FpTag;
typedef mcl::FpT<FpTag, 384> Fp;

void put(const char *msg, const void *buf, size_t bufSize)
{
	printf("%s:", msg);
	const unsigned char* p = (const unsigned char*)buf;
	for (size_t i = 0; i < bufSize; i++) {
		printf("%02x", p[i]);
	}
	printf("\n");
}

template<typename T>
inline size_t getBinary2(uint8_t *bin, size_t maxBinN, const T *x, size_t xN, size_t w)
{
	if (w == 0 || w >= 8) return 0;
	size_t binN = 0;
	size_t zeroNum = 0;

	mcl::fp::BitIterator<T> iter(x, xN);
	while (iter.hasNext()) {
		do {
			if (iter.peekBit()) break;
			zeroNum++;
			iter.skipBit();
		} while (iter.hasNext());
		for (size_t i = 0; i < zeroNum; i++) {
			if (binN == maxBinN) return 0;
			bin[binN++] = 0;
		}
		uint32_t v = iter.getNext(w);
		if (binN == maxBinN) return 0;
		bin[binN++] = v;
		zeroNum = w - 1;
	}
	while (binN > 0 && bin[binN - 1] == 0) {
		binN--;
	}
	return binN;
}

inline size_t getBinary(uint8_t *bin, size_t maxBinN, mpz_class x, size_t w)
{
	return getBinary2(bin, maxBinN, mcl::gmp::getUnit(x), mcl::gmp::getUnitSize(x), w);
}

template<class F, typename T, size_t w = 5>
void pow3(F& z, const F& x, const T *y, size_t yN)
{
	assert(yN >= 0);
	assert(w > 0);
	if (y == 0) {
		z = 1;
		return;
	}
	const size_t tblSize = 1 << (w - 1);
	uint8_t bin[sizeof(F) * 8 + 1];
	F tbl[tblSize];
	size_t binN = getBinary2(bin, sizeof(bin), y, yN, w);
	assert(binN > 0);

	F x2;
	F::sqr(x2, x);
	tbl[0] = x;
	for (size_t i = 1; i < tblSize; i++) {
		F::mul(tbl[i], tbl[i - 1], x2);
	}
	z = 1;
	for (size_t i = 0; i < binN; i++) {
		const size_t bit = binN - 1 - i;
		F::sqr(z, z);
		uint8_t n = bin[bit];
		if (n > 0) {
			z *= tbl[(n - 1) >> 1];
		}
	}
}

template<class T>
void pow3(T& z, const T& x, const mpz_class& y)
{
	pow3(z, x, mcl::gmp::getUnit(y), mcl::gmp::getUnitSize(y));
}

template<class T, class F>
void pow3(T& z, const T& x, const F& y)
{
	pow3(z, x, y.getMpz());
}

void bench(const char *name, const char *pStr)
{
	puts("------------------");
	printf("bench name=%s\n", name);
	Fp::init(pStr);
	const int C = 10000;
	cybozu::XorShift rg;
	Fp x, y, x0;
	for (int i = 0; i < 100; i++) {
		x.setByCSPRNG(rg);
		y.setByCSPRNG(rg);
		Fp t1, t2;
		Fp::pow(t1, x, y.getMpz());
		pow3(t2, x, y.getMpz());
		CYBOZU_TEST_EQUAL(t1, t2);
	}
	CYBOZU_BENCH_C("Fp::add", C, Fp::add, x, x, y);
	CYBOZU_BENCH_C("Fp::sub", C, Fp::sub, x, x, y);
	CYBOZU_BENCH_C("Fp::mul", C, Fp::mul, x, x, y);
	CYBOZU_BENCH_C("Fp::sqr", C, Fp::sqr, x, x);
	CYBOZU_BENCH_C("Fp::inv", C, Fp::inv, x, x);
	CYBOZU_BENCH_C("Fp::pow", C, Fp::pow, x, x, x);
	CYBOZU_BENCH_C("pow3   ", C, pow3, x, x, x);
	CYBOZU_BENCH_C("getMpz", C, x.getMpz);
	uint8_t bin[sizeof(Fp) * 8 + 1];
	mpz_class mx = x.getMpz();
	CYBOZU_BENCH_C("getBinary:4", C, getBinary, bin, sizeof(bin), mx, 4);
	CYBOZU_BENCH_C("getBinary:5", C, getBinary, bin, sizeof(bin), mx, 5);
}

CYBOZU_TEST_AUTO(main)
{
	const struct {
		const char *name;
		const char *pStr;
	} tbl[] = {
		{ "secp256k1p", mcl::ecparam::secp256k1.p },
		{ "secp256k1r", mcl::ecparam::secp256k1.n },
		{ "bn254r", "0x2523648240000001ba344d8000000007ff9f800000000010a10000000000000d" },
		{ "bn254p", "0x2523648240000001ba344d80000000086121000000000013a700000000000013" },
		{ "bls12_381p", "0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001" },
		{ "bls12_381r", "0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab" },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		bench(tbl[i].name, tbl[i].pStr);
	}
}

