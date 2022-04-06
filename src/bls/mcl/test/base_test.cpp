// not compiled
#include <map>
#include <mcl/op.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/bit_operation.hpp>
#include <mcl/conversion.hpp>
#include <mcl/fp.hpp>

#include "../src/fp_generator.hpp"
#if (CYBOZU_HOST == CYBOZU_HOST_INTEL) && (MCL_SIZEOF_UNIT == 8)
	#define USE_XBYAK
	static mcl::FpGenerator fg;
#endif
#define PUT(x) std::cout << #x "=" << (x) << std::endl

const size_t MAX_N = 32;
typedef mcl::fp::Unit Unit;

size_t getUnitSize(size_t bitSize)
{
	return (bitSize + sizeof(Unit) * 8 - 1) / (sizeof(Unit) * 8);
}

void setMpz(mpz_class& mx, const Unit *x, size_t n)
{
	mcl::gmp::setArray(mx, x, n);
}
void getMpz(Unit *x, size_t n, const mpz_class& mx)
{
	mcl::fp::toArray(x,  n, mx.get_mpz_t());
}

struct Montgomery {
	mpz_class p_;
	mpz_class R_; // (1 << (n_ * 64)) % p
	mpz_class RR_; // (R * R) % p
	Unit r_; // p * r = -1 mod M = 1 << 64
	size_t n_;
	Montgomery() {}
	explicit Montgomery(const mpz_class& p)
	{
		p_ = p;
		r_ = mcl::montgomery::getCoff(mcl::gmp::getUnit(p, 0));
		n_ = mcl::gmp::getUnitSize(p);
		R_ = 1;
		R_ = (R_ << (n_ * 64)) % p_;
		RR_ = (R_ * R_) % p_;
	}

	void toMont(mpz_class& x) const { mul(x, x, RR_); }
	void fromMont(mpz_class& x) const { mul(x, x, 1); }

	void mont(Unit *z, const Unit *x, const Unit *y) const
	{
		mpz_class mx, my;
		setMpz(mx, x, n_);
		setMpz(my, y, n_);
		mul(mx, mx, my);
		getMpz(z, n_, mx);
	}
	void mul(mpz_class& z, const mpz_class& x, const mpz_class& y) const
	{
#if 1
		const size_t ySize = mcl::gmp::getUnitSize(y);
		mpz_class c = y == 0 ? mpz_class(0) : x * mcl::gmp::getUnit(y, 0);
		Unit q = c == 0 ? 0 : mcl::gmp::getUnit(c, 0) * r_;
		c += p_ * q;
		c >>= sizeof(Unit) * 8;
		for (size_t i = 1; i < n_; i++) {
			if (i < ySize) {
				c += x * mcl::gmp::getUnit(y, i);
			}
			Unit q = c == 0 ? 0 : mcl::gmp::getUnit(c, 0) * r_;
			c += p_ * q;
			c >>= sizeof(Unit) * 8;
		}
		if (c >= p_) {
			c -= p_;
		}
		z = c;
#else
		z = x * y;
		const size_t zSize = mcl::gmp::getUnitSize(z);
		for (size_t i = 0; i < n_; i++) {
			if (i < zSize) {
				Unit q = mcl::gmp::getUnit(z, 0) * r_;
				z += p_ * (mp_limb_t)q;
			}
			z >>= sizeof(Unit) * 8;
		}
		if (z >= p_) {
			z -= p_;
		}
#endif
	}
};

void put(const char *msg, const Unit *x, size_t n)
{
	printf("%s ", msg);
	for (size_t i = 0; i < n; i++) printf("%016llx ", (long long)x[n - 1 - i]);
	printf("\n");
}
void verifyEqual(const Unit *x, const Unit *y, size_t n, const char *file, int line)
{
	bool ok = mcl::fp::isEqualArray(x, y, n);
	CYBOZU_TEST_ASSERT(ok);
	if (ok) return;
	printf("%s:%d\n", file, line);
	put("L", x, n);
	put("R", y, n);
	exit(1);
}
#define VERIFY_EQUAL(x, y, n) verifyEqual(x, y, n, __FILE__, __LINE__)

void addC(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	mpz_class mx, my, mp;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	setMpz(mp, p, n);
	mx += my;
	if (mx >= mp) mx -= mp;
	getMpz(z, n, mx);
}
void subC(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	mpz_class mx, my, mp;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	setMpz(mp, p, n);
	mx -= my;
	if (mx < 0) mx += mp;
	getMpz(z, n, mx);
}
static inline void set_zero(mpz_t& z, Unit *p, size_t n)
{
	z->_mp_alloc = (int)n;
	z->_mp_size = 0;
	z->_mp_d = (mp_limb_t*)p;
}
static inline void set_mpz_t(mpz_t& z, const Unit* p, int n)
{
	z->_mp_alloc = n;
	int i = n;
	while (i > 0 && p[i - 1] == 0) {
		i--;
	}
	z->_mp_size = i;
	z->_mp_d = (mp_limb_t*)p;
}

// z[2n] <- x[n] * y[n]
void mulPreC(Unit *z, const Unit *x, const Unit *y, size_t n)
{
#if 1
	mpz_t mx, my, mz;
	set_zero(mz, z, n * 2);
	set_mpz_t(mx, x, n);
	set_mpz_t(my, y, n);
	mpz_mul(mz, mx, my);
	mcl::fp::toArray(z, n * 2, mz);
#else
	mpz_class mx, my;
	setMpz(mx, x, n);
	setMpz(my, y, n);
	mx *= my;
	getMpz(z, n * 2, mx);
#endif
}

void modC(Unit *y, const Unit *x, const Unit *p, size_t n)
{
	mpz_t mx, my, mp;
	set_mpz_t(mx, x, n * 2);
	set_mpz_t(my, y, n);
	set_mpz_t(mp, p, n);
	mpz_mod(my, mx, mp);
	mcl::fp::clearArray(y, my->_mp_size, n);
}

void mul(Unit *z, const Unit *x, const Unit *y, const Unit *p, size_t n)
{
	Unit ret[MAX_N * 2];
	mpz_t mx, my, mz, mp;
	set_zero(mz, ret, MAX_N * 2);
	set_mpz_t(mx, x, n);
	set_mpz_t(my, y, n);
	set_mpz_t(mp, p, n);
	mpz_mul(mz, mx, my);
	mpz_mod(mz, mz, mp);
	mcl::fp::toArray(z, n, mz);
}

typedef mcl::fp::void3op void3op;
typedef mcl::fp::void4op void4op;
typedef mcl::fp::void4Iop void4Iop;

const struct FuncOp {
	size_t bitSize;
	void4op addS;
	void4op addL;
	void4op subS;
	void4op subL;
	void3op mulPre;
	void4Iop mont;
} gFuncOpTbl[] = {
	{ 128, mcl_fp_add128S, mcl_fp_add128L, mcl_fp_sub128S, mcl_fp_sub128L, mcl_fp_mul128pre, mcl_fp_mont128 },
	{ 192, mcl_fp_add192S, mcl_fp_add192L, mcl_fp_sub192S, mcl_fp_sub192L, mcl_fp_mul192pre, mcl_fp_mont192 },
	{ 256, mcl_fp_add256S, mcl_fp_add256L, mcl_fp_sub256S, mcl_fp_sub256L, mcl_fp_mul256pre, mcl_fp_mont256 },
	{ 320, mcl_fp_add320S, mcl_fp_add320L, mcl_fp_sub320S, mcl_fp_sub320L, mcl_fp_mul320pre, mcl_fp_mont320 },
	{ 384, mcl_fp_add384S, mcl_fp_add384L, mcl_fp_sub384S, mcl_fp_sub384L, mcl_fp_mul384pre, mcl_fp_mont384 },
	{ 448, mcl_fp_add448S, mcl_fp_add448L, mcl_fp_sub448S, mcl_fp_sub448L, mcl_fp_mul448pre, mcl_fp_mont448 },
	{ 512, mcl_fp_add512S, mcl_fp_add512L, mcl_fp_sub512S, mcl_fp_sub512L, mcl_fp_mul512pre, mcl_fp_mont512 },
#if MCL_SIZEOF_UNIT == 4
	{ 160, mcl_fp_add160S, mcl_fp_add160L, mcl_fp_sub160S, mcl_fp_sub160L, mcl_fp_mul160pre, mcl_fp_mont160 },
	{ 224, mcl_fp_add224S, mcl_fp_add224L, mcl_fp_sub224S, mcl_fp_sub224L, mcl_fp_mul224pre, mcl_fp_mont224 },
	{ 288, mcl_fp_add288S, mcl_fp_add288L, mcl_fp_sub288S, mcl_fp_sub288L, mcl_fp_mul288pre, mcl_fp_mont288 },
	{ 352, mcl_fp_add352S, mcl_fp_add352L, mcl_fp_sub352S, mcl_fp_sub352L, mcl_fp_mul352pre, mcl_fp_mont352 },
	{ 416, mcl_fp_add416S, mcl_fp_add416L, mcl_fp_sub416S, mcl_fp_sub416L, mcl_fp_mul416pre, mcl_fp_mont416 },
	{ 480, mcl_fp_add480S, mcl_fp_add480L, mcl_fp_sub480S, mcl_fp_sub480L, mcl_fp_mul480pre, mcl_fp_mont480 },
	{ 544, mcl_fp_add544S, mcl_fp_add544L, mcl_fp_sub544S, mcl_fp_sub544L, mcl_fp_mul544pre, mcl_fp_mont544 },
#else
	{ 576, mcl_fp_add576S, mcl_fp_add576L, mcl_fp_sub576S, mcl_fp_sub576L, mcl_fp_mul576pre, mcl_fp_mont576 },
#endif
};

FuncOp getFuncOp(size_t bitSize)
{
	typedef std::map<size_t, FuncOp> Map;
	static Map map;
	static bool init = false;
	if (!init) {
		init = true;
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(gFuncOpTbl); i++) {
			map[gFuncOpTbl[i].bitSize] = gFuncOpTbl[i];
		}
	}
	for (Map::const_iterator i = map.begin(), ie = map.end(); i != ie; ++i) {
		if (bitSize <= i->second.bitSize) {
			return i->second;
		}
	}
	printf("ERR bitSize=%d\n", (int)bitSize);
	exit(1);
}

void test(const Unit *p, size_t bitSize)
{
	printf("bitSize %d\n", (int)bitSize);
	const size_t n = getUnitSize(bitSize);
#ifdef NDEBUG
	bool doBench = true;
#else
	bool doBench = false;
#endif
	const FuncOp funcOp = getFuncOp(bitSize);
	const void4op addS = funcOp.addS;
	const void4op addL = funcOp.addL;
	const void4op subS = funcOp.subS;
	const void4op subL = funcOp.subL;
	const void3op mulPre = funcOp.mulPre;
	const void4Iop mont = funcOp.mont;

	mcl::fp::Unit x[MAX_N], y[MAX_N];
	mcl::fp::Unit z[MAX_N], w[MAX_N];
	mcl::fp::Unit z2[MAX_N * 2];
	mcl::fp::Unit w2[MAX_N * 2];
	cybozu::XorShift rg;
	mcl::fp::getRandVal(x, rg, p, bitSize);
	mcl::fp::getRandVal(y, rg, p, bitSize);
	const size_t C = 10;

	addC(z, x, y, p, n);
	addS(w, x, y, p);
	VERIFY_EQUAL(z, w, n);
	for (size_t i = 0; i < C; i++) {
		addC(z, y, z, p, n);
		addS(w, y, w, p);
		VERIFY_EQUAL(z, w, n);
		addC(z, y, z, p, n);
		addL(w, y, w, p);
		VERIFY_EQUAL(z, w, n);
		subC(z, x, z, p, n);
		subS(w, x, w, p);
		VERIFY_EQUAL(z, w, n);
		subC(z, x, z, p, n);
		subL(w, x, w, p);
		VERIFY_EQUAL(z, w, n);
		mulPreC(z2, x, z, n);
		mulPre(w2, x, z);
		VERIFY_EQUAL(z2, w2, n * 2);
	}
	{
		mpz_class mp;
		setMpz(mp, p, n);
		Montgomery m(mp);
#ifdef USE_XBYAK
		if (bitSize > 128) fg.init(p, n);
#endif
		/*
			real mont
			   0    0
			   1    R^-1
			   R    1
			  -1    -R^-1
			  -R    -1
		*/
		mpz_class t = 1;
		const mpz_class R = (t << (n * 64)) % mp;
		const mpz_class tbl[] = {
			0, 1, R, mp - 1, mp - R
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const mpz_class& mx = tbl[i];
			for (size_t j = i; j < CYBOZU_NUM_OF_ARRAY(tbl); j++) {
				const mpz_class& my = tbl[j];
				getMpz(x, n, mx);
				getMpz(y, n, my);
				m.mont(z, x, y);
				mont(w, x, y, p, m.r_);
				VERIFY_EQUAL(z, w, n);
#ifdef USE_XBYAK
				if (bitSize > 128) {
					fg.mul_(w, x, y);
					VERIFY_EQUAL(z, w, n);
				}
#endif
			}
		}
		if (doBench) {
//			CYBOZU_BENCH("montC", m.mont, x, y, x);
			CYBOZU_BENCH("montA  ", mont, x, y, x, p, m.r_);
		}
	}
	if (doBench) {
//		CYBOZU_BENCH("addS", addS, x, y, x, p); // slow
//		CYBOZU_BENCH("subS", subS, x, y, x, p);
//		CYBOZU_BENCH("addL", addL, x, y, x, p);
//		CYBOZU_BENCH("subL", subL, x, y, x, p);
		CYBOZU_BENCH("mulPreA", mulPre, w2, y, x);
		CYBOZU_BENCH("mulPreC", mulPreC, w2, y, x, n);
		CYBOZU_BENCH("modC   ", modC, x, w2, p, n);
	}
#ifdef USE_XBYAK
	if (bitSize <= 128) return;
	if (doBench) {
		fg.init(p, n);
		CYBOZU_BENCH("addA   ", fg.add_, x, y, x);
		CYBOZU_BENCH("subA   ", fg.sub_, x, y, x);
//		CYBOZU_BENCH("mulA", fg.mul_, x, y, x);
	}
#endif
	printf("mont test %d\n", (int)bitSize);
}

CYBOZU_TEST_AUTO(all)
{
	const struct {
		size_t n;
		const uint64_t p[9];
	} tbl[] = {
//		{ 2, { 0xf000000000000001, 1, } },
		{ 2, { 0x000000000000001d, 0x8000000000000000, } },
		{ 3, { 0x000000000000012b, 0x0000000000000000, 0x0000000080000000, } },
//		{ 3, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x07ffffffffffffff, } },
//		{ 3, { 0x7900342423332197, 0x1234567890123456, 0x1480948109481904, } },
		{ 3, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0xffffffffffffffff, } },
//		{ 4, { 0x7900342423332197, 0x4242342420123456, 0x1234567892342342, 0x1480948109481904, } },
//		{ 4, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x17ffffffffffffff, 0x1513423423423415, } },
		{ 4, { 0xa700000000000013, 0x6121000000000013, 0xba344d8000000008, 0x2523648240000001, } },
//		{ 5, { 0x0000000000000009, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 5, { 0xfffffffffffffc97, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 6, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x4820984290482212, 0x9482094820948209, 0x0194810841094810, } },
//		{ 6, { 0x7204224233321972, 0x0342308472047204, 0x4567890123456790, 0x0948204204243123, 0x2098420984209482, 0x2093482094810948, } },
		{ 6, { 0x00000000ffffffff, 0xffffffff00000000, 0xfffffffffffffffe, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
//		{ 7, { 0x0000000000000063, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x8000000000000000, } },
		{ 7, { 0x000000000fffcff1, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 8, { 0xffffffffffffd0c9, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, } },
		{ 9, { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0x00000000000001ff, } },
//		{ 9, { 0x4720422423332197, 0x0034230847204720, 0x3456789012345679, 0x2498540975555312, 0x9482904924029424, 0x0948209842098402, 0x1098410948109482, 0x0820958209582094, 0x0000000000000029, } },
//		{ 9, { 0x0f69466a74defd8d, 0xfffffffe26f2fc17, 0x7fffffffffffffff, 0x8572938572398583, 0x5732057823857293, 0x9820948205872380, 0x3409238420492034, 0x9483842098340298, 0x0000000000000003, } },
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const size_t n = tbl[i].n;
		const size_t bitSize = (n - 1) * 64 + cybozu::bsr<uint64_t>(tbl[i].p[n - 1]) + 1;
		test((const Unit*)tbl[i].p, bitSize);
	}
}

