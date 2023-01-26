#ifndef MCL_INCLUDE_MCL_BN_HPP
#define MCL_INCLUDE_MCL_BN_HPP
// use MCL_INCLUDE_MCL_BN_HPP instead of #pragma once to be able to include twice
/**
	@file
	@brief optimal ate pairing over BN-curve / BLS12-curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/fp_tower.hpp>
#include <mcl/ec.hpp>
#include <mcl/curve_type.h>
namespace mcl { namespace local {

// to export fast cofactor multiplication to mapto_wb19
template<class T>
void mulByCofactorBLS12fast(T& Q, const T& P);

} } // mcl::local
#include <mcl/mapto_wb19.hpp>
#include <assert.h>
#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <vector>
#endif

#ifdef MCL_USE_OMP
#include <omp.h>
#endif

/*
	set bit size of Fp and Fr
*/
#ifndef MCL_MAX_FP_BIT_SIZE
	#define MCL_MAX_FP_BIT_SIZE 256
#endif

#ifndef MCL_MAX_FR_BIT_SIZE
	#define MCL_MAX_FR_BIT_SIZE MCL_MAX_FP_BIT_SIZE
#endif
#ifndef MCL_NAMESPACE_BN
	#define MCL_NAMESPACE_BN bn
#endif
namespace mcl {

namespace MCL_NAMESPACE_BN {

namespace local {
struct FpTag;
struct FrTag;
}

typedef mcl::FpT<local::FpTag, MCL_MAX_FP_BIT_SIZE> Fp;
typedef mcl::FpT<local::FrTag, MCL_MAX_FR_BIT_SIZE> Fr;
typedef mcl::Fp2T<Fp> Fp2;
typedef mcl::Fp6T<Fp> Fp6;
typedef mcl::Fp12T<Fp> Fp12;
typedef mcl::EcT<Fp> G1;
typedef mcl::EcT<Fp2> G2;
typedef Fp12 GT;

typedef mcl::FpDblT<Fp> FpDbl;
typedef mcl::Fp2DblT<Fp> Fp2Dbl;
typedef mcl::Fp6DblT<Fp> Fp6Dbl;

inline void Frobenius(Fp2& y, const Fp2& x)
{
	Fp2::Frobenius(y, x);
}
inline void Frobenius(Fp12& y, const Fp12& x)
{
	Fp12::Frobenius(y, x);
}
/*
	twisted Frobenius for G2
*/
void Frobenius(G2& D, const G2& S);
void Frobenius2(G2& D, const G2& S);
void Frobenius3(G2& D, const G2& S);

namespace local {

typedef mcl::FixedArray<int8_t, 128> SignVec;

inline size_t getPrecomputeQcoeffSize(const SignVec& sv)
{
	size_t idx = 2 + 2;
	for (size_t i = 2; i < sv.size(); i++) {
		idx++;
		if (sv[i]) idx++;
	}
	return idx;
}

template<class X, class C, size_t N>
X evalPoly(const X& x, const C (&c)[N])
{
	X ret = c[N - 1];
	for (size_t i = 1; i < N; i++) {
		ret *= x;
		ret += c[N - 1 - i];
	}
	return ret;
}

enum TwistBtype {
	tb_generic,
	tb_1m1i, // 1 - 1i
	tb_1m2i // 1 - 2i
};

/*
	l = (a, b, c) => (a, b * P.y, c * P.x)
*/
inline void updateLine(Fp6& l, const G1& P)
{
	l.b.a *= P.y;
	l.b.b *= P.y;
	l.c.a *= P.x;
	l.c.b *= P.x;
}

struct Compress {
	Fp12& z_;
	Fp2& g1_;
	Fp2& g2_;
	Fp2& g3_;
	Fp2& g4_;
	Fp2& g5_;
	// z is output area
	Compress(Fp12& z, const Fp12& x)
		: z_(z)
		, g1_(z.getFp2()[4])
		, g2_(z.getFp2()[3])
		, g3_(z.getFp2()[2])
		, g4_(z.getFp2()[1])
		, g5_(z.getFp2()[5])
	{
		g2_ = x.getFp2()[3];
		g3_ = x.getFp2()[2];
		g4_ = x.getFp2()[1];
		g5_ = x.getFp2()[5];
	}
	Compress(Fp12& z, const Compress& c)
		: z_(z)
		, g1_(z.getFp2()[4])
		, g2_(z.getFp2()[3])
		, g3_(z.getFp2()[2])
		, g4_(z.getFp2()[1])
		, g5_(z.getFp2()[5])
	{
		g2_ = c.g2_;
		g3_ = c.g3_;
		g4_ = c.g4_;
		g5_ = c.g5_;
	}
	void decompressBeforeInv(Fp2& nume, Fp2& denomi) const
	{
		assert(&nume != &denomi);

		if (g2_.isZero()) {
			Fp2::mul2(nume, g4_);
			nume *= g5_;
			denomi = g3_;
		} else {
			Fp2 t;
			Fp2::sqr(nume, g5_);
			Fp2::mul_xi(denomi, nume);
			Fp2::sqr(nume, g4_);
			Fp2::sub(t, nume, g3_);
			Fp2::mul2(t, t);
			t += nume;
			Fp2::add(nume, denomi, t);
			Fp2::divBy4(nume, nume);
			denomi = g2_;
		}
	}

	// output to z
	void decompressAfterInv()
	{
		Fp2& g0 = z_.getFp2()[0];
		Fp2 t0, t1;
		// Compute g0.
		Fp2::sqr(t0, g1_);
		Fp2::mul(t1, g3_, g4_);
		t0 -= t1;
		Fp2::mul2(t0, t0);
		t0 -= t1;
		Fp2::mul(t1, g2_, g5_);
		t0 += t1;
		Fp2::mul_xi(g0, t0);
		g0.a += Fp::one();
	}

public:
	void decompress() // for test
	{
		Fp2 nume, denomi;
		decompressBeforeInv(nume, denomi);
		Fp2::inv(denomi, denomi);
		g1_ = nume * denomi; // g1 is recoverd.
		decompressAfterInv();
	}
	/*
		2275clk * 186 = 423Kclk QQQ
	*/
	static void squareC(Compress& z)
	{
		Fp2 t0, t1, t2;
		Fp2Dbl T0, T1, T2, T3;
		Fp2Dbl::sqrPre(T0, z.g4_);
		Fp2Dbl::sqrPre(T1, z.g5_);
		Fp2Dbl::mul_xi(T2, T1);
		T2 += T0;
		Fp2Dbl::mod(t2, T2);
		Fp2::add(t0, z.g4_, z.g5_);
		Fp2Dbl::sqrPre(T2, t0);
		T0 += T1;
		T2 -= T0;
		Fp2Dbl::mod(t0, T2);
		Fp2::add(t1, z.g2_, z.g3_);
		Fp2Dbl::sqrPre(T3, t1);
		Fp2Dbl::sqrPre(T2, z.g2_);
		Fp2::mul_xi(t1, t0);
		z.g2_ += t1;
		Fp2::mul2(z.g2_, z.g2_);
		z.g2_ += t1;
		Fp2::sub(t1, t2, z.g3_);
		Fp2::mul2(t1, t1);
		Fp2Dbl::sqrPre(T1, z.g3_);
		Fp2::add(z.g3_, t1, t2);
		Fp2Dbl::mul_xi(T0, T1);
		T0 += T2;
		Fp2Dbl::mod(t0, T0);
		Fp2::sub(z.g4_, t0, z.g4_);
		Fp2::mul2(z.g4_, z.g4_);
		z.g4_ += t0;
		Fp2Dbl::addPre(T2, T2, T1);
		T3 -= T2;
		Fp2Dbl::mod(t0, T3);
		z.g5_ += t0;
		Fp2::mul2(z.g5_, z.g5_);
		z.g5_ += t0;
	}
	static void square_n(Compress& z, int n)
	{
		for (int i = 0; i < n; i++) {
			squareC(z);
		}
	}
	/*
		Exponentiation over compression for:
		z = x^Param::z.abs()
	*/
	static void fixed_power(Fp12& z, const Fp12& x)
	{
		if (x.isOne()) {
			z = 1;
			return;
		}
		Fp12 x_org = x;
		Fp12 d62;
		Fp2 c55nume, c55denomi, c62nume, c62denomi;
		Compress c55(z, x);
		square_n(c55, 55);
		c55.decompressBeforeInv(c55nume, c55denomi);
		Compress c62(d62, c55);
		square_n(c62, 62 - 55);
		c62.decompressBeforeInv(c62nume, c62denomi);
		Fp2 acc;
		Fp2::mul(acc, c55denomi, c62denomi);
		Fp2::inv(acc, acc);
		Fp2 t;
		Fp2::mul(t, acc, c62denomi);
		Fp2::mul(c55.g1_, c55nume, t);
		c55.decompressAfterInv();
		Fp2::mul(t, acc, c55denomi);
		Fp2::mul(c62.g1_, c62nume, t);
		c62.decompressAfterInv();
		z *= x_org;
		z *= d62;
	}
};

struct MapTo {
	enum {
		BNtype,
		BLS12type,
		STD_ECtype
	};
	Fp c1_; // sqrt(-3)
	Fp c2_; // (-1 + sqrt(-3)) / 2
	mpz_class z_;
	mpz_class z2_;
	mpz_class cofactor_;
	mpz_class g2cofactor_;
	Fr g2cofactorAdj_;
	Fr g2cofactorAdjInv_;
	int type_;
	int mapToMode_;
	MapTo_WB19<Fp, G1, Fp2, G2> mapTo_WB19_;
	MapTo()
		: type_(0)
		, mapToMode_(MCL_MAP_TO_MODE_ORIGINAL)
	{
	}

	int legendre(bool *pb, const Fp& x) const
	{
		mpz_class xx;
		x.getMpz(pb, xx);
		if (!*pb) return 0;
		return gmp::legendre(xx, Fp::getOp().mp);
	}
	int legendre(bool *pb, const Fp2& x) const
	{
		Fp y;
		Fp2::norm(y, x);
		return legendre(pb, y);
	}
	void mulFp(Fp& x, const Fp& y) const
	{
		x *= y;
	}
	void mulFp(Fp2& x, const Fp& y) const
	{
		x.a *= y;
		x.b *= y;
	}
	/*
		P.-A. Fouque and M. Tibouchi,
		"Indifferentiable hashing to Barreto Naehrig curves,"
		in Proc. Int. Conf. Cryptol. Inform. Security Latin Amer., 2012, vol. 7533, pp.1-17.

		w = sqrt(-3) t / (1 + b + t^2)
		Remark: throw exception if t = 0, c1, -c1 and b = 2
	*/
	template<class G, class F>
	bool calcBN(G& P, const F& t) const
	{
		F x, y, w;
		bool b;
		bool negative = legendre(&b, t) < 0;
		if (!b) return false;
		if (t.isZero()) return false;
		F::sqr(w, t);
		w += G::b_;
		*w.getFp0() += Fp::one();
		if (w.isZero()) return false;
		F::inv(w, w);
		mulFp(w, c1_);
		w *= t;
		for (int i = 0; i < 3; i++) {
			switch (i) {
			case 0: F::mul(x, t, w); F::neg(x, x); *x.getFp0() += c2_; break;
			case 1: F::neg(x, x); *x.getFp0() -= Fp::one(); break;
			case 2: F::sqr(x, w); F::inv(x, x); *x.getFp0() += Fp::one(); break;
			}
			G::getWeierstrass(y, x);
			if (F::squareRoot(y, y)) {
				if (negative) F::neg(y, y);
				P.set(&b, x, y, false);
				assert(b);
				return true;
			}
		}
		return false;
	}
	/*
		Faster Hashing to G2
		Laura Fuentes-Castaneda, Edward Knapp, Francisco Rodriguez-Henriquez
		section 6.1
		for BN
		Q = zP + Frob(3zP) + Frob^2(zP) + Frob^3(P)
		  = -(18x^3 + 12x^2 + 3x + 1)cofactor_ P
	*/
	void mulByCofactorBN(G2& Q, const G2& P) const
	{
#if 0
		G2::mulGeneric(Q, P, cofactor_);
#else
#if 0
		mpz_class t = -(1 + z_ * (3 + z_ * (12 + z_ * 18)));
		G2::mulGeneric(Q, P, t * cofactor_);
#else
		G2 T0, T1, T2;
		/*
			G2::mul (GLV method) can't be used because P is not on G2
		*/
		G2::mulGeneric(T0, P, z_);
		G2::dbl(T1, T0);
		T1 += T0; // 3zP
		Frobenius(T1, T1);
		Frobenius2(T2, T0);
		T0 += T1;
		T0 += T2;
		Frobenius3(T2, P);
		G2::add(Q, T0, T2);
#endif
#endif
	}
	/*
		#(Fp) / r = (z + 1 - t) / r = (z - 1)^2 / 3
	*/
	void mulByCofactorBLS12(G1& Q, const G1& P) const
	{
		G1::mulGeneric(Q, P, cofactor_);
	}
	/*
		Efficient hash maps to G2 on BLS curves
		Alessandro Budroni, Federico Pintore
		Q = (z(z-1)-1)P + Frob((z-1)P) + Frob^2(2P)
		original G2 cofactor = this cofactor * g2cofactorAdj_
	*/
	void mulByCofactorBLS12fast(G2& Q, const G2& P) const
	{
		G2 T0, T1;
		G2::mulGeneric(T0, P, z_ - 1);
		G2::mulGeneric(T1, T0, z_);
		T1 -= P;
		Frobenius(T0, T0);
		T0 += T1;
		G2::dbl(T1, P);
		Frobenius2(T1, T1);
		G2::add(Q, T0, T1);
	}
	void mulByCofactorBLS12(G2& Q, const G2& P) const
	{
		mulByCofactorBLS12fast(Q, P);
	}
	/*
		cofactor_ is for G2(not used now)
	*/
	void initBN(const mpz_class& cofactor, const mpz_class &z, int curveType)
	{
		z_ = z;
		cofactor_ = cofactor;
		if (curveType == MCL_BN254) {
			const char *c1 = "252364824000000126cd890000000003cf0f0000000000060c00000000000004";
			const char *c2 = "25236482400000017080eb4000000006181800000000000cd98000000000000b";
			bool b;
			c1_.setStr(&b, c1, 16);
			c2_.setStr(&b, c2, 16);
			(void)b;
			return;
		}
		bool b = Fp::squareRoot(c1_, -3);
		assert(b);
		(void)b;
		c2_ = (c1_ - 1) / 2;
	}
	void initBLS12(const mpz_class& z, int curveType)
	{
		z_ = z;
		if (curveType == MCL_BLS12_381) {
			const char *z2 = "396c8c005555e1560000000055555555";
			const char *cofactor = "396c8c005555e1568c00aaab0000aaab";
			const char *g2cofactor = "5d543a95414e7f1091d50792876a202cd91de4547085abaa68a205b2e5a7ddfa628f1cb4d9e82ef21537e293a6691ae1616ec6e786f0c70cf1c38e31c7238e5";
			const char *c1 = "be32ce5fbeed9ca374d38c0ed41eefd5bb675277cdf12d11bc2fb026c41400045c03fffffffdfffd";
			const char *c2 = "5f19672fdf76ce51ba69c6076a0f77eaddb3a93be6f89688de17d813620a00022e01fffffffefffe";
			const char *g2cofactorAdjInv = "204d0ec030004ec0600000002fffffffd";
			const char *g2cofactorAdj = "26a48d1bb889d46d66689d580335f2ac37d2aaab55543d5455555554aaaaaaab";
			bool b;
			gmp::setStr(&b, z2_, z2, 16); assert(b); (void)b;
			gmp::setStr(&b, cofactor_, cofactor, 16); assert(b); (void)b;
			gmp::setStr(&b, g2cofactor_, g2cofactor, 16); assert(b); (void)b;
			c1_.setStr(&b, c1, 16); assert(b); (void)b;
			c2_.setStr(&b, c2, 16); assert(b); (void)b;
			g2cofactorAdjInv_.setStr(&b, g2cofactorAdjInv, 16); assert(b); (void)b;
			g2cofactorAdj_.setStr(&b, g2cofactorAdj, 16); assert(b); (void)b;
			mapTo_WB19_.init();
			return;
		}
		z2_ = (z_ * z_ - 1) / 3;
		// cofactor for G1
		cofactor_ = (z - 1) * (z - 1) / 3;
		const int g2Coff[] = { 13, -4, -4, 6, -4, 0, 5, -4, 1 };
		g2cofactor_ = local::evalPoly(z, g2Coff) / 9;
		bool b = Fp::squareRoot(c1_, -3);
		assert(b);
		(void)b;
		c2_ = (c1_ - 1) / 2;
		mpz_class t = (z * z - 1) * 3;;
		g2cofactorAdjInv_.setMpz(&b, t);
		assert(b);
		(void)b;
		Fr::inv(g2cofactorAdj_, g2cofactorAdjInv_);
	}
	/*
		change mapTo function to mode
	*/
	bool setMapToMode(int mode)
	{
		if (type_ == STD_ECtype) {
			mapToMode_ = MCL_MAP_TO_MODE_TRY_AND_INC;
			return true;
		}
		switch (mode) {
		case MCL_MAP_TO_MODE_ORIGINAL:
		case MCL_MAP_TO_MODE_TRY_AND_INC:
		case MCL_MAP_TO_MODE_HASH_TO_CURVE_07:
		case MCL_MAP_TO_MODE_ETH2_LEGACY:
			mapToMode_ = mode;
			return true;
		default:
			return false;
		}
	}
	/*
		if type == STD_ECtype, then cofactor, z are not used.
	*/
	void init(const mpz_class& cofactor, const mpz_class &z, int curveType)
	{
		if (0 <= curveType && curveType < MCL_EC_BEGIN) {
			type_ = (curveType == MCL_BLS12_381 || curveType == MCL_BLS12_377 || curveType == MCL_BLS12_461) ? BLS12type : BNtype;
		} else {
			type_ = STD_ECtype;
		}
		setMapToMode(MCL_MAP_TO_MODE_ORIGINAL);
		if (type_ == BNtype) {
			initBN(cofactor, z, curveType);
		} else if (type_ == BLS12type) {
			initBLS12(z, curveType);
		}
	}
	template<class G, class F>
	bool mapToEc(G& P, const F& t) const
	{
		if (mapToMode_ == MCL_MAP_TO_MODE_TRY_AND_INC || mapToMode_ == MCL_MAP_TO_MODE_ETH2_LEGACY) {
			ec::tryAndIncMapTo<G>(P, t);
		} else {
			if (!calcBN<G, F>(P, t)) return false;
		}
		return true;
	}
	void mulByCofactor(G1& P) const
	{
		switch (type_) {
		case BNtype:
			// no subgroup
			break;
		case BLS12type:
			mulByCofactorBLS12(P, P);
			break;
		}
		assert(P.isValid());
	}
	void mulByCofactor(G2& P) const
	{
		switch(type_) {
		case BNtype:
			mulByCofactorBN(P, P);
			break;
		case BLS12type:
			mulByCofactorBLS12(P, P);
			break;
		}
		assert(P.isValid());
	}
	bool calc(G1& P, const Fp& t) const
	{
		if (mapToMode_ == MCL_MAP_TO_MODE_HASH_TO_CURVE_07) {
			mapTo_WB19_.FpToG1(P, t);
			return true;
		}
		if (!mapToEc(P, t)) return false;
		mulByCofactor(P);
		return true;
	}
	bool calc(G2& P, const Fp2& t) const
	{
		if (mapToMode_ == MCL_MAP_TO_MODE_HASH_TO_CURVE_07) {
			mapTo_WB19_.Fp2ToG2(P, t);
			return true;
		}
		if (!mapToEc(P, t)) return false;
		if (mapToMode_ == MCL_MAP_TO_MODE_ETH2_LEGACY) {
			Fp2 negY;
			Fp2::neg(negY, P.y);
			int cmp = Fp::compare(P.y.b, negY.b);
			if (!(cmp > 0 || (cmp == 0 && P.y.a > negY.a))) {
				P.y = negY;
			}
		}
		mulByCofactor(P);
		if (mapToMode_ == MCL_MAP_TO_MODE_ETH2_LEGACY) {
			P *= g2cofactorAdj_;
		}
		return true;
	}
};


/*
	Software implementation of Attribute-Based Encryption: Appendixes
	GLV for G1 on BN/BLS12
*/

struct GLV1 : mcl::GLV1T<G1, Fr> {
	static bool usePrecomputedTable(int curveType)
	{
		if (curveType < 0) return false;
		const struct Tbl {
			int curveType;
			const char *rw;
			size_t rBitSize;
			const char *v0, *v1;
			const char *B[2][2];
		} tbl[] = {
			{
				MCL_BN254,
				"49b36240000000024909000000000006cd80000000000007",
				256,
				"2a01fab7e04a017b9c0eb31ff36bf3357",
				"37937ca688a6b4904",
				{
					{
						"61818000000000028500000000000004",
						"8100000000000001",
					},
					{
						"8100000000000001",
						"-61818000000000020400000000000003",
					},
				},
			},
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			if (tbl[i].curveType != curveType) continue;
			bool b;
			rw.setStr(&b, tbl[i].rw, 16); if (!b) continue;
			rBitSize = tbl[i].rBitSize;
			gmp::setStr(&b, v0, tbl[i].v0, 16); if (!b) continue;
			gmp::setStr(&b, v1, tbl[i].v1, 16); if (!b) continue;
			gmp::setStr(&b, B[0][0], tbl[i].B[0][0], 16); if (!b) continue;
			gmp::setStr(&b, B[0][1], tbl[i].B[0][1], 16); if (!b) continue;
			gmp::setStr(&b, B[1][0], tbl[i].B[1][0], 16); if (!b) continue;
			gmp::setStr(&b, B[1][1], tbl[i].B[1][1], 16); if (!b) continue;
			return true;
		}
		return false;
	}
	static void initForBN(const mpz_class& z, bool isBLS12 = false, int curveType = -1)
	{
		if (usePrecomputedTable(curveType)) return;
		bool b = Fp::squareRoot(rw, -3);
		assert(b);
		(void)b;
		rw = -(rw + 1) / 2;
		rBitSize = Fr::getOp().bitSize;
		rBitSize = (rBitSize + fp::UnitBitSize - 1) & ~(fp::UnitBitSize - 1);// a little better size
		if (isBLS12) {
			/*
				BLS12
				L = z^4
				(-z^2+1) + L = 0
				1 + z^2 L = 0
			*/
			B[0][0] = -z * z + 1;
			B[0][1] = 1;
			B[1][0] = 1;
			B[1][1] = z * z;
		} else {
			/*
				BN
				L = 36z^4 - 1
				(6z^2+2z) - (2z+1)   L = 0
				(-2z-1) - (6z^2+4z+1)L = 0
			*/
			B[0][0] = 6 * z * z + 2 * z;
			B[0][1] = -2 * z - 1;
			B[1][0] = -2 * z - 1;
			B[1][1] = -6 * z * z - 4 * z - 1;
		}
		// [v0 v1] = [r 0] * B^(-1)
		const mpz_class& r = Fr::getOp().mp;
		v0 = ((-B[1][1]) << rBitSize) / r;
		v1 = ((B[1][0]) << rBitSize) / r;
	}
};

/*
	GLV method for G2 and GT on BN/BLS12
*/
template<class _Fr>
struct GLV2T {
	typedef GLV2T<_Fr> GLV2;
	typedef _Fr Fr;
	static size_t rBitSize;
	static mpz_class B[4][4];
	static mpz_class v[4];
	static mpz_class z;
	static mpz_class abs_z;
	static bool isBLS12;
	static void init(const mpz_class& z, bool isBLS12 = false)
	{
		const mpz_class& r = Fr::getOp().mp;
		GLV2::z = z;
		GLV2::abs_z = z < 0 ? -z : z;
		GLV2::isBLS12 = isBLS12;
		rBitSize = Fr::getOp().bitSize;
		rBitSize = (rBitSize + mcl::fp::UnitBitSize - 1) & ~(mcl::fp::UnitBitSize - 1);// a little better size
		mpz_class z2p1 = z * 2 + 1;
		B[0][0] = z + 1;
		B[0][1] = z;
		B[0][2] = z;
		B[0][3] = -2 * z;
		B[1][0] = z2p1;
		B[1][1] = -z;
		B[1][2] = -(z + 1);
		B[1][3] = -z;
		B[2][0] = 2 * z;
		B[2][1] = z2p1;
		B[2][2] = z2p1;
		B[2][3] = z2p1;
		B[3][0] = z - 1;
		B[3][1] = 2 * z2p1;
		B[3][2] =  -2 * z + 1;
		B[3][3] = z - 1;
		/*
			v[] = [r 0 0 0] * B^(-1) = [2z^2+3z+1, 12z^3+8z^2+z, 6z^3+4z^2+z, -(2z+1)]
		*/
		const char *zBN254 = "-4080000000000001";
		mpz_class t;
		bool b;
		mcl::gmp::setStr(&b, t, zBN254, 16);
		assert(b);
		(void)b;
		if (z == t) {
			static const char *vTblBN254[] = {
				"e00a8e7f56e007e5b09fe7fdf43ba998",
				"-152aff56a8054abf9da75db2da3d6885101e5fd3997d41cb1",
				"-a957fab5402a55fced3aed96d1eb44295f40f136ee84e09b",
				"-e00a8e7f56e007e929d7b2667ea6f29c",
			};
			for (int i = 0; i < 4; i++) {
				mcl::gmp::setStr(&b, v[i], vTblBN254[i], 16);
				assert(b);
				(void)b;
			}
		} else {
			v[0] = ((1 + z * (3 + z * 2)) << rBitSize) / r;
			v[1] = ((z * (1 + z * (8 + z * 12))) << rBitSize) / r;
			v[2] = ((z * (1 + z * (4 + z * 6))) << rBitSize) / r;
			v[3] = -((z * (1 + z * 2)) << rBitSize) / r;
		}
	}
	/*
		u[] = [x, 0, 0, 0] - v[] * x * B
	*/
	static void split(mpz_class u[4], const mpz_class& x)
	{
		if (isBLS12) {
			/*
				Frob(P) = zP
				x = u[0] + u[1] z + u[2] z^2 + u[3] z^3
			*/
			bool isNeg = false;
			mpz_class t = x;
			if (t < 0) {
				t = -t;
				isNeg = true;
			}
			for (int i = 0; i < 4; i++) {
				// t = t / abs_z, u[i] = t % abs_z
				mcl::gmp::divmod(t, u[i], t, abs_z);
				if (((z < 0) && (i & 1)) ^ isNeg) {
					u[i] = -u[i];
				}
			}
			return;
		}
		// BN
		mpz_class t[4];
		for (int i = 0; i < 4; i++) {
			t[i] = (x * v[i]) >> rBitSize;
		}
		for (int i = 0; i < 4; i++) {
			u[i] = (i == 0) ? x : 0;
			for (int j = 0; j < 4; j++) {
				u[i] -= t[j] * B[j][i];
			}
		}
	}
	template<class T>
	static void mul(T& Q, const T& P, const mpz_class& x, bool constTime = false)
	{
		if (constTime) {
			ec::local::mul1CT<GLV2, T, Fr, 4, 4>(Q, P, x);
		} else {
			ec::local::mulVecNGLVT<GLV2, T, Fr, 4, 5, 1>(Q, &P, &x, 1);
		}
	}
	template<class T>
	static void mulLambda(T& Q, const T& P)
	{
		Frobenius(Q, P);
	}
	template<class T>
	static size_t mulVecNGLV(T& z, const T *xVec, const mpz_class *yVec, size_t n)
	{
		return ec::local::mulVecNGLVT<GLV2, T, Fr, 4, 5, fp::maxMulVecNGLV>(z, xVec, yVec, n);
	}
	static void pow(Fp12& z, const Fp12& x, const mpz_class& y, bool constTime = false)
	{
		typedef GroupMtoA<Fp12> AG; // as additive group
		AG& _z = static_cast<AG&>(z);
		const AG& _x = static_cast<const AG&>(x);
		mul(_z, _x, y, constTime);
	}
};

template<class Fr> size_t GLV2T<Fr>::rBitSize = 0;
template<class Fr> mpz_class GLV2T<Fr>::B[4][4];
template<class Fr> mpz_class GLV2T<Fr>::v[4];
template<class Fr> mpz_class GLV2T<Fr>::z;
template<class Fr> mpz_class GLV2T<Fr>::abs_z;
template<class Fr> bool GLV2T<Fr>::isBLS12 = false;

struct Param {
	CurveParam cp;
	mpz_class z;
	mpz_class abs_z;
	bool isNegative;
	bool isBLS12;
	mpz_class p;
	mpz_class r;
	local::MapTo mapTo;
	// for G2 Frobenius
	Fp2 g2;
	Fp2 g3;
	/*
		Dtype twist
		(x', y') = phi(x, y) = (x/w^2, y/w^3)
		y^2 = x^3 + b
		=> (y'w^3)^2 = (x'w^2)^3 + b
		=> y'^2 = x'^3 + b / w^6 ; w^6 = xi
		=> y'^2 = x'^3 + twist_b;
	*/
	Fp2 twist_b;
	local::TwistBtype twist_b_type;
/*
	mpz_class exp_c0;
	mpz_class exp_c1;
	mpz_class exp_c2;
	mpz_class exp_c3;
*/

	// Loop parameter for the Miller loop part of opt. ate pairing.
	local::SignVec siTbl;
	size_t precomputedQcoeffSize;
	bool useNAF;
	local::SignVec zReplTbl;

	// for initG1only
	G1 basePoint;

	void init(bool *pb, const mcl::CurveParam& cp, fp::Mode mode)
	{
		this->cp = cp;
		isBLS12 = (cp.curveType == MCL_BLS12_381 || cp.curveType == MCL_BLS12_377 || cp.curveType == MCL_BLS12_461);
#ifdef MCL_STATIC_CODE
		if (!isBLS12) {
			*pb = false;
			return;
		}
#endif
		gmp::setStr(pb, z, cp.z);
		if (!*pb) return;
		isNegative = z < 0;
		if (isNegative) {
			abs_z = -z;
		} else {
			abs_z = z;
		}
		if (isBLS12) {
			mpz_class z2 = z * z;
			mpz_class z4 = z2 * z2;
			r = z4 - z2 + 1;
			p = z - 1;
			p = p * p * r / 3 + z;
		} else {
			const int pCoff[] = { 1, 6, 24, 36, 36 };
			const int rCoff[] = { 1, 6, 18, 36, 36 };
			p = local::evalPoly(z, pCoff);
			assert((p % 6) == 1);
			r = local::evalPoly(z, rCoff);
		}
		Fr::init(pb, r, mode);
		if (!*pb) return;
		Fp::init(pb, cp.xi_a, p, mode);
		if (!*pb) return;
#ifdef MCL_DUMP_JIT
		*pb = true;
		return;
#endif
		Fp2::init(pb);
		if (!*pb) return;
		const Fp2 xi(cp.xi_a, 1);
		g2 = Fp2::get_gTbl()[0];
		g3 = Fp2::get_gTbl()[3];
		if (cp.isMtype) {
			Fp2::inv(g2, g2);
			Fp2::inv(g3, g3);
		}
		if (cp.isMtype) {
			twist_b = Fp2(cp.b) * xi;
		} else {
			if (cp.b == 2 && cp.xi_a == 1) {
				twist_b = Fp2(1, -1); // shortcut
			} else {
				twist_b = Fp2(cp.b) / xi;
			}
		}
		if (twist_b == Fp2(1, -1)) {
			twist_b_type = tb_1m1i;
		} else if (twist_b == Fp2(1, -2)) {
			twist_b_type = tb_1m2i;
		} else {
			twist_b_type = tb_generic;
		}
		G1::init(0, cp.b, mcl::ec::Jacobi);
		G2::init(0, twist_b, mcl::ec::Jacobi);

		const mpz_class largest_c = isBLS12 ? abs_z : gmp::abs(z * 6 + 2);
		useNAF = gmp::getNAF(siTbl, largest_c);
		precomputedQcoeffSize = local::getPrecomputeQcoeffSize(siTbl);
		gmp::getNAF(zReplTbl, gmp::abs(z));
/*
		if (isBLS12) {
			mpz_class z2 = z * z;
			mpz_class z3 = z2 * z;
			mpz_class z4 = z3 * z;
			mpz_class z5 = z4 * z;
			exp_c0 = z5 - 2 * z4 + 2 * z2 - z + 3;
			exp_c1 = z4 - 2 * z3 + 2 * z - 1;
			exp_c2 = z3 - 2 * z2 + z;
			exp_c3 = z2 - 2 * z + 1;
		} else {
			exp_c0 = -2 + z * (-18 + z * (-30 - 36 * z));
			exp_c1 = 1 + z * (-12 + z * (-18 - 36 * z));
			exp_c2 = 6 * z * z + 1;
		}
*/
		if (isBLS12) {
			mapTo.init(0, z, cp.curveType);
		} else {
			mapTo.init(2 * p - r, z, cp.curveType);
		}
		GLV1::initForBN(z, isBLS12, cp.curveType);
		GLV2T<Fr>::init(z, isBLS12);
		basePoint.clear();
		G1::setOrder(r);
		G2::setOrder(r);
		*pb = true;
	}
	void initG1only(bool *pb, const mcl::EcParam& para)
	{
		Fp::init(pb, para.p);
		if (!*pb) return;
		Fr::init(pb, para.n);
		if (!*pb) return;
		G1::init(pb, para.a, para.b);
		if (!*pb) return;
		mapTo.init(0, 0, para.curveType);
		Fp x0, y0;
		x0.setStr(pb, para.gx);
		if (!*pb) return;
		y0.setStr(pb, para.gy);
		basePoint.set(pb, x0, y0);
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void init(const mcl::CurveParam& cp, fp::Mode mode)
	{
		bool b;
		init(&b, cp, mode);
		if (!b) throw cybozu::Exception("Param:init");
	}
#endif
};

template<size_t dummyImpl = 0>
struct StaticVar {
	static local::Param param;
};

template<size_t dummyImpl>
local::Param StaticVar<dummyImpl>::param;

} // mcl::bn::local

namespace BN {

static const local::Param& param = local::StaticVar<>::param;
static local::Param& nonConstParam = local::StaticVar<>::param;

} // mcl::bn::BN

namespace local {

typedef GLV2T<Fr> GLV2;

inline void mulArrayGLV2(G2& z, const G2& x, const mcl::fp::Unit *y, size_t yn, bool isNegative, bool constTime)
{
	mpz_class s;
	bool b;
	mcl::gmp::setArray(&b, s, y, yn);
	assert(b);
	if (isNegative) s = -s;
	GLV2::mul(z, x, s, constTime);
}
inline void powArrayGLV2(Fp12& z, const Fp12& x, const mcl::fp::Unit *y, size_t yn, bool isNegative, bool constTime)
{
	mpz_class s;
	bool b;
	mcl::gmp::setArray(&b, s, y, yn);
	assert(b);
	if (isNegative) s = -s;
	GLV2::pow(z, x, s, constTime);
}

inline size_t mulVecNGLV2(G2& z, const G2 *xVec, const mpz_class *yVec, size_t n)
{
	return GLV2::mulVecNGLV(z, xVec, yVec, n);
}

inline size_t powVecNGLV2(Fp12& z, const Fp12 *xVec, const mpz_class *yVec, size_t n)
{
	typedef GroupMtoA<Fp12> AG; // as additive group
	AG& _z = static_cast<AG&>(z);
	const AG *_xVec = static_cast<const AG*>(xVec);
	return GLV2::mulVecNGLV(_z, _xVec, yVec, n);
}

/*
	Faster Squaring in the Cyclotomic Subgroup of Sixth Degree Extensions
	Robert Granger, Michael Scott
*/
inline void sqrFp4(Fp2& z0, Fp2& z1, const Fp2& x0, const Fp2& x1)
{
#if 1
	Fp2Dbl T0, T1, T2;
	Fp2Dbl::sqrPre(T0, x0);
	Fp2Dbl::sqrPre(T1, x1);
	Fp2Dbl::mul_xi(T2, T1);
	Fp2Dbl::add(T2, T2, T0);
	Fp2::add(z1, x0, x1);
	Fp2Dbl::mod(z0, T2);
	Fp2Dbl::sqrPre(T2, z1);
	Fp2Dbl::sub(T2, T2, T0);
	Fp2Dbl::sub(T2, T2, T1);
	Fp2Dbl::mod(z1, T2);
#else
	Fp2 t0, t1, t2;
	Fp2::sqr(t0, x0);
	Fp2::sqr(t1, x1);
	Fp2::mul_xi(z0, t1);
	z0 += t0;
	Fp2::add(z1, x0, x1);
	Fp2::sqr(z1, z1);
	z1 -= t0;
	z1 -= t1;
#endif
}

inline void fasterSqr(Fp12& y, const Fp12& x)
{
#if 0
	Fp12::sqr(y, x);
#else
	const Fp2& x0(x.a.a);
	const Fp2& x4(x.a.b);
	const Fp2& x3(x.a.c);
	const Fp2& x2(x.b.a);
	const Fp2& x1(x.b.b);
	const Fp2& x5(x.b.c);
	Fp2& y0(y.a.a);
	Fp2& y4(y.a.b);
	Fp2& y3(y.a.c);
	Fp2& y2(y.b.a);
	Fp2& y1(y.b.b);
	Fp2& y5(y.b.c);
	Fp2 t0, t1;
	sqrFp4(t0, t1, x0, x1);
	Fp2::sub(y0, t0, x0);
	Fp2::mul2(y0, y0);
	y0 += t0;
	Fp2::add(y1, t1, x1);
	Fp2::mul2(y1, y1);
	y1 += t1;
	Fp2 t2, t3;
	sqrFp4(t0, t1, x2, x3);
	sqrFp4(t2, t3, x4, x5);
	Fp2::sub(y4, t0, x4);
	Fp2::mul2(y4, y4);
	y4 += t0;
	Fp2::add(y5, t1, x5);
	Fp2::mul2(y5, y5);
	y5 += t1;
	Fp2::mul_xi(t0, t3);
	Fp2::add(y2, t0, x2);
	Fp2::mul2(y2, y2);
	y2 += t0;
	Fp2::sub(y3, t2, x3);
	Fp2::mul2(y3, y3);
	y3 += t2;
#endif
}

/*
	y = x^z if z > 0
	  = unitaryInv(x^(-z)) if z < 0
*/
inline void pow_z(Fp12& y, const Fp12& x)
{
#if 1
	if (BN::param.cp.curveType == MCL_BN254) {
		Compress::fixed_power(y, x);
	} else {
		Fp12 orgX = x;
		y = x;
		Fp12 conj;
		conj.a = x.a;
		Fp6::neg(conj.b, x.b);
		for (size_t i = 1; i < BN::param.zReplTbl.size(); i++) {
			fasterSqr(y, y);
			if (BN::param.zReplTbl[i] > 0) {
				y *= orgX;
			} else if (BN::param.zReplTbl[i] < 0) {
				y *= conj;
			}
		}
	}
#else
	Fp12::pow(y, x, param.abs_z);
#endif
	if (BN::param.isNegative) {
		Fp12::unitaryInv(y, y);
	}
}
inline void mul_twist_b(Fp2& y, const Fp2& x)
{
	switch (BN::param.twist_b_type) {
	case local::tb_1m1i:
		/*
			b / xi = 1 - 1i
			(a + bi)(1 - 1i) = (a + b) + (b - a)i
		*/
		{
			Fp t;
			Fp::add(t, x.a, x.b);
			Fp::sub(y.b, x.b, x.a);
			y.a = t;
		}
		return;
	case local::tb_1m2i:
		/*
			b / xi = 1 - 2i
			(a + bi)(1 - 2i) = (a + 2b) + (b - 2a)i
		*/
		{
			Fp t;
			Fp::sub(t, x.b, x.a);
			t -= x.a;
			Fp::add(y.a, x.a, x.b);
			y.a += x.b;
			y.b = t;
		}
		return;
	case local::tb_generic:
		Fp2::mul(y, x, BN::param.twist_b);
		return;
	}
}

inline void dblLineWithoutP(Fp6& l, G2& Q)
{
	Fp2 t0, t1, t2, t3, t4, t5;
	Fp2Dbl T0, T1;
	Fp2::sqr(t0, Q.z);
	Fp2::mul(t4, Q.x, Q.y);
	Fp2::sqr(t1, Q.y);
	Fp2::mul2(t3, t0);
	Fp2::divBy2(t4, t4);
	Fp2::add(t5, t0, t1);
	t0 += t3;
	mul_twist_b(t2, t0);
	Fp2::sqr(t0, Q.x);
	Fp2::mul2(t3, t2);
	t3 += t2;
	Fp2::sub(Q.x, t1, t3);
	t3 += t1;
	Q.x *= t4;
	Fp2::divBy2(t3, t3);
	Fp2Dbl::sqrPre(T0, t3);
	Fp2Dbl::sqrPre(T1, t2);
	Fp2Dbl::sub(T0, T0, T1);
	Fp2Dbl::add(T1, T1, T1);
	Fp2Dbl::sub(T0, T0, T1);
	Fp2::add(t3, Q.y, Q.z);
	Fp2Dbl::mod(Q.y, T0);
	Fp2::sqr(t3, t3);
	t3 -= t5;
	Fp2::mul(Q.z, t1, t3);
	Fp2::sub(l.a, t2, t1);
	l.c = t0;
	l.b = t3;
}
inline void addLineWithoutP(Fp6& l, G2& R, const G2& Q)
{
	Fp2 t1, t2, t3, t4;
	Fp2Dbl T1, T2;
	Fp2::mul(t1, R.z, Q.x);
	Fp2::mul(t2, R.z, Q.y);
	Fp2::sub(t1, R.x, t1);
	Fp2::sub(t2, R.y, t2);
	Fp2::sqr(t3, t1);
	Fp2::mul(R.x, t3, R.x);
	Fp2::sqr(t4, t2);
	t3 *= t1;
	t4 *= R.z;
	t4 += t3;
	t4 -= R.x;
	t4 -= R.x;
	R.x -= t4;
	Fp2Dbl::mulPre(T1, t2, R.x);
	Fp2Dbl::mulPre(T2, t3, R.y);
	Fp2Dbl::sub(T2, T1, T2);
	Fp2Dbl::mod(R.y, T2);
	Fp2::mul(R.x, t1, t4);
	Fp2::mul(R.z, t3, R.z);
	Fp2::neg(l.c, t2);
	Fp2Dbl::mulPre(T1, t2, Q.x);
	Fp2Dbl::mulPre(T2, t1, Q.y);
	Fp2Dbl::sub(T1, T1, T2);
	l.b = t1;
	Fp2Dbl::mod(l.a, T1);
}
inline void dblLine(Fp6& l, G2& Q, const G1& P)
{
	dblLineWithoutP(l, Q);
	local::updateLine(l, P);
}
inline void addLine(Fp6& l, G2& R, const G2& Q, const G1& P)
{
	addLineWithoutP(l, R, Q);
	local::updateLine(l, P);
}
inline void mulFp6cb_by_G1xy(Fp6& y, const Fp6& x, const G1& P)
{
	y.a = x.a;
	Fp2::mulFp(y.c, x.c, P.x);
	Fp2::mulFp(y.b, x.b, P.y);
}

/*
	x = a + bv + cv^2
	y = (y0, y4, y2) -> (y0, 0, y2, 0, y4, 0)
	z = xy = (a + bv + cv^2)(d + ev)
	= (ad + ce xi) + ((a + b)(d + e) - ad - be)v + (be + cd)v^2
*/
inline void Fp6mul_01(Fp6& z, const Fp6& x, const Fp2& d, const Fp2& e)
{
	const Fp2& a = x.a;
	const Fp2& b = x.b;
	const Fp2& c = x.c;
	Fp2 t0, t1;
	Fp2Dbl AD, CE, BE, CD, T;
	Fp2Dbl::mulPre(AD, a, d);
	Fp2Dbl::mulPre(CE, c, e);
	Fp2Dbl::mulPre(BE, b, e);
	Fp2Dbl::mulPre(CD, c, d);
	Fp2::add(t0, a, b);
	Fp2::add(t1, d, e);
	Fp2Dbl::mulPre(T, t0, t1);
	T -= AD;
	T -= BE;
	Fp2Dbl::mod(z.b, T);
	Fp2Dbl::mul_xi(CE, CE);
	AD += CE;
	Fp2Dbl::mod(z.a, AD);
	BE += CD;
	Fp2Dbl::mod(z.c, BE);
}
/*
	input
	z = (z0 + z1v + z2v^2) + (z3 + z4v + z5v^2)w = Z0 + Z1w
	                  0        3  4
	x = (a, b, c) -> (b, 0, 0, c, a, 0) = X0 + X1w
	X0 = b = (b, 0, 0)
	X1 = c + av = (c, a, 0)
	w^2 = v, v^3 = xi
	output
	z <- zx = (Z0X0 + Z1X1v) + ((Z0 + Z1)(X0 + X1) - Z0X0 - Z1X1)w
	Z0X0 = Z0 b
	Z1X1 = Z1 (c, a, 0)
	(Z0 + Z1)(X0 + X1) = (Z0 + Z1) (b + c, a, 0)
*/
inline void mul_403(Fp12& z, const Fp6& x)
{
	const Fp2& a = x.a;
	const Fp2& b = x.b;
	const Fp2& c = x.c;
#if 1
	Fp6& z0 = z.a;
	Fp6& z1 = z.b;
	Fp6 z0x0, z1x1, t0;
	Fp2 t1;
	Fp2::add(t1, x.b, c);
	Fp6::add(t0, z0, z1);
	Fp2::mul(z0x0.a, z0.a, b);
	Fp2::mul(z0x0.b, z0.b, b);
	Fp2::mul(z0x0.c, z0.c, b);
	Fp6mul_01(z1x1, z1, c, a);
	Fp6mul_01(t0, t0, t1, a);
	Fp6::sub(z.b, t0, z0x0);
	z.b -= z1x1;
	// a + bv + cv^2 = cxi + av + bv^2
	Fp2::mul_xi(z1x1.c, z1x1.c);
	Fp2::add(z.a.a, z0x0.a, z1x1.c);
	Fp2::add(z.a.b, z0x0.b, z1x1.a);
	Fp2::add(z.a.c, z0x0.c, z1x1.b);
#else
	Fp2& z0 = z.a.a;
	Fp2& z1 = z.a.b;
	Fp2& z2 = z.a.c;
	Fp2& z3 = z.b.a;
	Fp2& z4 = z.b.b;
	Fp2& z5 = z.b.c;
	Fp2Dbl Z0B, Z1B, Z2B, Z3C, Z4C, Z5C;
	Fp2Dbl T0, T1, T2, T3, T4, T5;
	Fp2 bc, t;
	Fp2::addPre(bc, b, c);
	Fp2::addPre(t, z5, z2);
	Fp2Dbl::mulPre(T5, t, bc);
	Fp2Dbl::mulPre(Z5C, z5, c);
	Fp2Dbl::mulPre(Z2B, z2, b);
	Fp2Dbl::sub(T5, T5, Z5C);
	Fp2Dbl::sub(T5, T5, Z2B);
	Fp2Dbl::mulPre(T0, z1, a);
	T5 += T0;

	Fp2::addPre(t, z4, z1);
	Fp2Dbl::mulPre(T4, t, bc);
	Fp2Dbl::mulPre(Z4C, z4, c);
	Fp2Dbl::mulPre(Z1B, z1, b);
	Fp2Dbl::sub(T4, T4, Z4C);
	Fp2Dbl::sub(T4, T4, Z1B);
	Fp2Dbl::mulPre(T0, z0, a);
	T4 += T0;

	Fp2::addPre(t, z3, z0);
	Fp2Dbl::mulPre(T3, t, bc);
	Fp2Dbl::mulPre(Z3C, z3, c);
	Fp2Dbl::mulPre(Z0B, z0, b);
	Fp2Dbl::sub(T3, T3, Z3C);
	Fp2Dbl::sub(T3, T3, Z0B);
	Fp2::mul_xi(t, z2);
	Fp2Dbl::mulPre(T0, t, a);
	T3 += T0;

	Fp2Dbl::mulPre(T2, z3, a);
	T2 += Z2B;
	T2 += Z4C;

	Fp2::mul_xi(t, z5);
	Fp2Dbl::mulPre(T1, t, a);
	T1 += Z1B;
	T1 += Z3C;

	Fp2Dbl::mulPre(T0, z4, a);
	T0 += Z5C;
	Fp2Dbl::mul_xi(T0, T0);
	T0 += Z0B;

	Fp2Dbl::mod(z0, T0);
	Fp2Dbl::mod(z1, T1);
	Fp2Dbl::mod(z2, T2);
	Fp2Dbl::mod(z3, T3);
	Fp2Dbl::mod(z4, T4);
	Fp2Dbl::mod(z5, T5);
#endif
}
/*
	input
	z = (z0 + z1v + z2v^2) + (z3 + z4v + z5v^2)w = Z0 + Z1w
	                  0  1        4
	x = (a, b, c) -> (a, c, 0, 0, b, 0) = X0 + X1w
	X0 = (a, c, 0)
	X1 = (0, b, 0)
	w^2 = v, v^3 = xi
	output
	z <- zx = (Z0X0 + Z1X1v) + ((Z0 + Z1)(X0 + X1) - Z0X0 - Z1X1)w
	Z0X0 = Z0 (a, c, 0)
	Z1X1 = Z1 (0, b, 0) = Z1 bv
	(Z0 + Z1)(X0 + X1) = (Z0 + Z1) (a, b + c, 0)

	(a + bv + cv^2)v = c xi + av + bv^2
*/
inline void mul_041(Fp12& z, const Fp6& x)
{
	const Fp2& a = x.a;
	const Fp2& b = x.b;
	const Fp2& c = x.c;
	Fp6& z0 = z.a;
	Fp6& z1 = z.b;
	Fp6 z0x0, z1x1, t0;
	Fp2 t1;
	Fp2::mul(z1x1.a, z1.c, b);
	Fp2::mul_xi(z1x1.a, z1x1.a);
	Fp2::mul(z1x1.b, z1.a, b);
	Fp2::mul(z1x1.c, z1.b, b);
	Fp2::add(t1, x.b, c);
	Fp6::add(t0, z0, z1);
	Fp6mul_01(z0x0, z0, a, c);
	Fp6mul_01(t0, t0, a, t1);
	Fp6::sub(z.b, t0, z0x0);
	z.b -= z1x1;
	// a + bv + cv^2 = cxi + av + bv^2
	Fp2::mul_xi(z1x1.c, z1x1.c);
	Fp2::add(z.a.a, z0x0.a, z1x1.c);
	Fp2::add(z.a.b, z0x0.b, z1x1.a);
	Fp2::add(z.a.c, z0x0.c, z1x1.b);
}
inline void mulSparse(Fp12& z, const Fp6& x)
{
	if (BN::param.cp.isMtype) {
		mul_041(z, x);
	} else {
		mul_403(z, x);
	}
}
inline void convertFp6toFp12(Fp12& y, const Fp6& x)
{
	if (BN::param.cp.isMtype) {
		// (a, b, c) -> (a, c, 0, 0, b, 0)
		y.a.a = x.a;
		y.b.b = x.b;
		y.a.b = x.c;
		y.a.c.clear();
		y.b.a.clear();
		y.b.c.clear();
	} else {
		// (a, b, c) -> (b, 0, 0, c, a, 0)
		y.b.b = x.a;
		y.a.a = x.b;
		y.b.a = x.c;
		y.a.b.clear();
		y.a.c.clear();
		y.b.c.clear();
	}
}
inline void mulSparse2(Fp12& z, const Fp6& x, const Fp6& y)
{
	convertFp6toFp12(z, x);
	mulSparse(z, y);
}
inline void mapToCyclotomic(Fp12& y, const Fp12& x)
{
	Fp12 z;
	Fp12::Frobenius2(z, x); // z = x^(p^2)
	z *= x; // x^(p^2 + 1)
	Fp12::inv(y, z);
	Fp6::neg(z.b, z.b); // z^(p^6) = conjugate of z
	y *= z;
}
/*
	Implementing Pairings at the 192-bit Security Level
	D.F.Aranha, L.F.Castaneda, E.Knapp, A.Menezes, F.R.Henriquez
	Section 4
*/
inline void expHardPartBLS12(Fp12& y, const Fp12& x)
{
#if 0
	const mpz_class& p = param.p;
	mpz_class p2 = p * p;
	mpz_class p4 = p2 * p2;
	Fp12::pow(y, x, (p4 - p2 + 1) / param.r * 3);
	return;
#endif
#if 1
	/*
		Efficient Final Exponentiation via Cyclotomic Structure
		for Pairings over Families of Elliptic Curves
		https://eprint.iacr.org/2020/875.pdf p.13
		(z-1)^2 (z+p)(z^2+p^2-1)+3
	*/
	Fp12 a0, a1, a2;
	pow_z(a0, x); // z
	Fp12::unitaryInv(a1, x); // -1
	a0 *= a1; // z-1
	pow_z(a1, a0); // (z-1)^z
	Fp12::unitaryInv(a0, a0); // -(z-1)
	a0 *= a1; // (z-1)^2
	pow_z(a1, a0); // z
	Fp12::Frobenius(a0, a0); // p
	a0 *=a1; // (z-1)^2 (z+p)
	pow_z(a1, a0); // z
	pow_z(a1, a1); // z^2
	Fp12::Frobenius2(a2, a0); // p^2
	Fp12::unitaryInv(a0, a0); // -1
	a0 *= a1;
	a0 *= a2; // z^2+p^2-1
	fasterSqr(a1, x);
	a1 *= x; // x^3
	Fp12::mul(y, a0, a1);
#else
	Fp12 a0, a1, a2, a3, a4, a5, a6, a7;
	Fp12::unitaryInv(a0, x); // a0 = x^-1
	fasterSqr(a1, a0); // x^-2
	pow_z(a2, x); // x^z
	fasterSqr(a3, a2); // x^2z
	a1 *= a2; // a1 = x^(z-2)
	pow_z(a7, a1); // a7 = x^(z^2-2z)
	pow_z(a4, a7); // a4 = x^(z^3-2z^2)
	pow_z(a5, a4); // a5 = x^(z^4-2z^3)
	a3 *= a5; // a3 = x^(z^4-2z^3+2z)
	pow_z(a6, a3); // a6 = x^(z^5-2z^4+2z^2)

	Fp12::unitaryInv(a1, a1); // x^(2-z)
	a1 *= a6; // x^(z^5-2z^4+2z^2-z+2)
	a1 *= x; // x^(z^5-2z^4+2z^2-z+3) = x^c0
	a3 *= a0; // x^(z^4-2z^3-1) = x^c1
	Fp12::Frobenius(a3, a3); // x^(c1 p)
	a1 *= a3; // x^(c0 + c1 p)
	a4 *= a2; // x^(z^3-2z^2+z) = x^c2
	Fp12::Frobenius2(a4, a4);  // x^(c2 p^2)
	a1 *= a4; // x^(c0 + c1 p + c2 p^2)
	a7 *= x; // x^(z^2-2z+1) = x^c3
	Fp12::Frobenius3(y, a7);
	y *= a1;
#endif
}
/*
	Faster Hashing to G2
	Laura Fuentes-Castaneda, Edward Knapp, Francisco Rodriguez-Henriquez
	section 4.1
	y = x^(d 2z(6z^2 + 3z + 1)) where
	p = p(z) = 36z^4 + 36z^3 + 24z^2 + 6z + 1
	r = r(z) = 36z^4 + 36z^3 + 18z^2 + 6z + 1
	d = (p^4 - p^2 + 1) / r
	d1 = d 2z(6z^2 + 3z + 1)
	= c0 + c1 p + c2 p^2 + c3 p^3

	c0 = 1 + 6z + 12z^2 + 12z^3
	c1 = 4z + 6z^2 + 12z^3
	c2 = 6z + 6z^2 + 12z^3
	c3 = -1 + 4z + 6z^2 + 12z^3
	x -> x^z -> x^2z -> x^4z -> x^6z -> x^(6z^2) -> x^(12z^2) -> x^(12z^3)
	a = x^(6z) x^(6z^2) x^(12z^3)
	b = a / (x^2z)
	x^d1 = (a x^(6z^2) x) b^p a^(p^2) (b / x)^(p^3)
*/
inline void expHardPartBN(Fp12& y, const Fp12& x)
{
#if 0
	const mpz_class& p = param.p;
	mpz_class p2 = p * p;
	mpz_class p4 = p2 * p2;
	Fp12::pow(y, x, (p4 - p2 + 1) / param.r);
	return;
#endif
#if 1
	Fp12 a, b;
	Fp12 a2, a3;
	pow_z(b, x); // x^z
	fasterSqr(b, b); // x^2z
	fasterSqr(a, b); // x^4z
	a *= b; // x^6z
	pow_z(a2, a); // x^(6z^2)
	a *= a2;
	fasterSqr(a3, a2); // x^(12z^2)
	pow_z(a3, a3); // x^(12z^3)
	a *= a3;
	Fp12::unitaryInv(b, b);
	b *= a;
	a2 *= a;
	Fp12::Frobenius2(a, a);
	a *= a2;
	a *= x;
	Fp12::unitaryInv(y, x);
	y *= b;
	Fp12::Frobenius(b, b);
	a *= b;
	Fp12::Frobenius3(y, y);
	y *= a;
#else
	Fp12 t1, t2, t3;
	Fp12::Frobenius(t1, x);
	Fp12::Frobenius(t2, t1);
	Fp12::Frobenius(t3, t2);
	Fp12::pow(t1, t1, param.exp_c1);
	Fp12::pow(t2, t2, param.exp_c2);
	Fp12::pow(y, x, param.exp_c0);
	y *= t1;
	y *= t2;
	y *= t3;
#endif
}
/*
	adjP = (P.x * 3, -P.y)
	remark : returned value is NOT on a curve
*/
inline void makeAdjP(G1& adjP, const G1& P)
{
	Fp x2;
	Fp::mul2(x2, P.x);
	Fp::add(adjP.x, x2, P.x);
	Fp::neg(adjP.y, P.y);
	// adjP.z.clear(); // not used
}

} // mcl::bn::local

/*
	y = x^((p^12 - 1) / r)
	(p^12 - 1) / r = (p^2 + 1) (p^6 - 1) (p^4 - p^2 + 1)/r
	(a + bw)^(p^6) = a - bw in Fp12
	(p^4 - p^2 + 1)/r = c0 + c1 p + c2 p^2 + p^3
*/
inline void finalExp(Fp12& y, const Fp12& x)
{
	if (x.isZero()) {
		y.clear();
		return;
	}
#if 1
	mapToCyclotomic(y, x);
#else
	const mpz_class& p = param.p;
	mpz_class p2 = p * p;
	mpz_class p4 = p2 * p2;
	Fp12::pow(y, x, p2 + 1);
	Fp12::pow(y, y, p4 * p2 - 1);
#endif
	if (BN::param.isBLS12) {
		expHardPartBLS12(y, y);
	} else {
		expHardPartBN(y, y);
	}
}
inline void millerLoop(Fp12& f, const G1& P_, const G2& Q_)
{
	G1 P(P_);
	G2 Q(Q_);
	P.normalize();
	Q.normalize();
	if (Q.isZero()) {
		f = 1;
		return;
	}
	assert(BN::param.siTbl[1] == 1);
	G2 T = Q;
	G2 negQ;
	if (BN::param.useNAF) {
		G2::neg(negQ, Q);
	}
	Fp6 d, e;
	G1 adjP;
	makeAdjP(adjP, P);
	dblLine(d, T, adjP);
	addLine(e, T, Q, P);
	mulSparse2(f, d, e);
	for (size_t i = 2; i < BN::param.siTbl.size(); i++) {
		dblLine(e, T, adjP);
		Fp12::sqr(f, f);
		mulSparse(f, e);
		if (BN::param.siTbl[i]) {
			if (BN::param.siTbl[i] > 0) {
				addLine(e, T, Q, P);
			} else {
				addLine(e, T, negQ, P);
			}
			mulSparse(f, e);
		}
	}
	if (BN::param.z < 0) {
		Fp6::neg(f.b, f.b);
	}
	if (BN::param.isBLS12) return;
	if (BN::param.z < 0) {
		G2::neg(T, T);
	}
	Frobenius(Q, Q);
	addLine(d, T, Q, P);
	Frobenius(Q, Q);
	G2::neg(Q, Q);
	addLine(e, T, Q, P);
	Fp12 ft;
	mulSparse2(ft, d, e);
	f *= ft;
}
inline void pairing(Fp12& f, const G1& P, const G2& Q)
{
	millerLoop(f, P, Q);
	finalExp(f, f);
}
/*
	allocate param.precomputedQcoeffSize elements of Fp6 for Qcoeff
*/
inline void precomputeG2(Fp6 *Qcoeff, const G2& Q_)
{
	size_t idx = 0;
	G2 Q(Q_);
	Q.normalize();
	if (Q.isZero()) {
		for (size_t i = 0; i < BN::param.precomputedQcoeffSize; i++) {
			Qcoeff[i] = 1;
		}
		return;
	}
	G2 T = Q;
	G2 negQ;
	if (BN::param.useNAF) {
		G2::neg(negQ, Q);
	}
	assert(BN::param.siTbl[1] == 1);
	dblLineWithoutP(Qcoeff[idx++], T);
	addLineWithoutP(Qcoeff[idx++], T, Q);
	for (size_t i = 2; i < BN::param.siTbl.size(); i++) {
		dblLineWithoutP(Qcoeff[idx++], T);
		if (BN::param.siTbl[i]) {
			if (BN::param.siTbl[i] > 0) {
				addLineWithoutP(Qcoeff[idx++], T, Q);
			} else {
				addLineWithoutP(Qcoeff[idx++], T, negQ);
			}
		}
	}
	if (BN::param.z < 0) {
		G2::neg(T, T);
	}
	if (BN::param.isBLS12) return;
	Frobenius(Q, Q);
	addLineWithoutP(Qcoeff[idx++], T, Q);
	Frobenius(Q, Q);
	G2::neg(Q, Q);
	addLineWithoutP(Qcoeff[idx++], T, Q);
	assert(idx == BN::param.precomputedQcoeffSize);
}
/*
	millerLoop(e, P, Q) is same as the following
	std::vector<Fp6> Qcoeff;
	precomputeG2(Qcoeff, Q);
	precomputedMillerLoop(e, P, Qcoeff);
*/
#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void precomputeG2(std::vector<Fp6>& Qcoeff, const G2& Q)
{
	Qcoeff.resize(BN::param.precomputedQcoeffSize);
	precomputeG2(Qcoeff.data(), Q);
}
#endif
template<class Array>
void precomputeG2(bool *pb, Array& Qcoeff, const G2& Q)
{
	*pb = Qcoeff.resize(BN::param.precomputedQcoeffSize);
	if (!*pb) return;
	precomputeG2(Qcoeff.data(), Q);
}

inline void precomputedMillerLoop(Fp12& f, const G1& P_, const Fp6* Qcoeff)
{
	G1 P(P_);
	P.normalize();
	G1 adjP;
	makeAdjP(adjP, P);
	size_t idx = 0;
	Fp6 d, e;
	mulFp6cb_by_G1xy(d, Qcoeff[idx], adjP);
	idx++;

	mulFp6cb_by_G1xy(e, Qcoeff[idx], P);
	idx++;
	mulSparse2(f, d, e);
	for (size_t i = 2; i < BN::param.siTbl.size(); i++) {
		mulFp6cb_by_G1xy(e, Qcoeff[idx], adjP);
		idx++;
		Fp12::sqr(f, f);
		mulSparse(f, e);
		if (BN::param.siTbl[i]) {
			mulFp6cb_by_G1xy(e, Qcoeff[idx], P);
			idx++;
			mulSparse(f, e);
		}
	}
	if (BN::param.z < 0) {
		Fp6::neg(f.b, f.b);
	}
	if (BN::param.isBLS12) return;
	mulFp6cb_by_G1xy(d, Qcoeff[idx], P);
	idx++;
	mulFp6cb_by_G1xy(e, Qcoeff[idx], P);
	idx++;
	Fp12 ft;
	mulSparse2(ft, d, e);
	f *= ft;
}
#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void precomputedMillerLoop(Fp12& f, const G1& P, const std::vector<Fp6>& Qcoeff)
{
	precomputedMillerLoop(f, P, Qcoeff.data());
}
#endif
/*
	f = MillerLoop(P1, Q1) x MillerLoop(P2, Q2)
	Q2coeff : precomputed Q2
*/
inline void precomputedMillerLoop2mixed(Fp12& f, const G1& P1_, const G2& Q1_, const G1& P2_, const Fp6* Q2coeff)
{
	G1 P1(P1_), P2(P2_);
	G2 Q1(Q1_);
	P1.normalize();
	P2.normalize();
	Q1.normalize();
	if (Q1.isZero()) {
		precomputedMillerLoop(f, P2_, Q2coeff);
		return;
	}
	G2 T = Q1;
	G2 negQ1;
	if (BN::param.useNAF) {
		G2::neg(negQ1, Q1);
	}
	G1 adjP1, adjP2;
	makeAdjP(adjP1, P1);
	makeAdjP(adjP2, P2);
	size_t idx = 0;
	Fp6 d1, d2, e1, e2;
	dblLine(d1, T, adjP1);
	mulFp6cb_by_G1xy(d2, Q2coeff[idx], adjP2);
	idx++;

	Fp12 f1, f2;
	addLine(e1, T, Q1, P1);
	mulSparse2(f1, d1, e1);

	mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
	mulSparse2(f2, d2, e2);
	Fp12::mul(f, f1, f2);
	idx++;
	for (size_t i = 2; i < BN::param.siTbl.size(); i++) {
		dblLine(e1, T, adjP1);
		mulFp6cb_by_G1xy(e2, Q2coeff[idx], adjP2);
		idx++;
		Fp12::sqr(f, f);
		mulSparse2(f1, e1, e2);
		f *= f1;
		if (BN::param.siTbl[i]) {
			if (BN::param.siTbl[i] > 0) {
				addLine(e1, T, Q1, P1);
			} else {
				addLine(e1, T, negQ1, P1);
			}
			mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
			idx++;
			mulSparse2(f1, e1, e2);
			f *= f1;
		}
	}
	if (BN::param.z < 0) {
		G2::neg(T, T);
		Fp6::neg(f.b, f.b);
	}
	if (BN::param.isBLS12) return;
	Frobenius(Q1, Q1);
	addLine(d1, T, Q1, P1);
	mulFp6cb_by_G1xy(d2, Q2coeff[idx], P2);
	idx++;
	Frobenius(Q1, Q1);
	G2::neg(Q1, Q1);
	addLine(e1, T, Q1, P1);
	mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
	idx++;
	mulSparse2(f1, d1, e1);
	mulSparse2(f2, d2, e2);
	f *= f1;
	f *= f2;
}
/*
	f = MillerLoop(P1, Q1) x MillerLoop(P2, Q2)
	Q1coeff, Q2coeff : precomputed Q1, Q2
*/
inline void precomputedMillerLoop2(Fp12& f, const G1& P1_, const Fp6* Q1coeff, const G1& P2_, const Fp6* Q2coeff)
{
	G1 P1(P1_), P2(P2_);
	P1.normalize();
	P2.normalize();
	G1 adjP1, adjP2;
	makeAdjP(adjP1, P1);
	makeAdjP(adjP2, P2);
	size_t idx = 0;
	Fp6 d1, d2, e1, e2;
	mulFp6cb_by_G1xy(d1, Q1coeff[idx], adjP1);
	mulFp6cb_by_G1xy(d2, Q2coeff[idx], adjP2);
	idx++;

	Fp12 f1, f2;
	mulFp6cb_by_G1xy(e1, Q1coeff[idx], P1);
	mulSparse2(f1, d1, e1);

	mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
	mulSparse2(f2, d2, e2);
	Fp12::mul(f, f1, f2);
	idx++;
	for (size_t i = 2; i < BN::param.siTbl.size(); i++) {
		mulFp6cb_by_G1xy(e1, Q1coeff[idx], adjP1);
		mulFp6cb_by_G1xy(e2, Q2coeff[idx], adjP2);
		idx++;
		Fp12::sqr(f, f);
		mulSparse2(f1, e1, e2);
		f *= f1;
		if (BN::param.siTbl[i]) {
			mulFp6cb_by_G1xy(e1, Q1coeff[idx], P1);
			mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
			idx++;
			mulSparse2(f1, e1, e2);
			f *= f1;
		}
	}
	if (BN::param.z < 0) {
		Fp6::neg(f.b, f.b);
	}
	if (BN::param.isBLS12) return;
	mulFp6cb_by_G1xy(d1, Q1coeff[idx], P1);
	mulFp6cb_by_G1xy(d2, Q2coeff[idx], P2);
	idx++;
	mulFp6cb_by_G1xy(e1, Q1coeff[idx], P1);
	mulFp6cb_by_G1xy(e2, Q2coeff[idx], P2);
	idx++;
	mulSparse2(f1, d1, e1);
	mulSparse2(f2, d2, e2);
	f *= f1;
	f *= f2;
}
#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void precomputedMillerLoop2(Fp12& f, const G1& P1, const std::vector<Fp6>& Q1coeff, const G1& P2, const std::vector<Fp6>& Q2coeff)
{
	precomputedMillerLoop2(f, P1, Q1coeff.data(), P2, Q2coeff.data());
}
inline void precomputedMillerLoop2mixed(Fp12& f, const G1& P1, const G2& Q1, const G1& P2, const std::vector<Fp6>& Q2coeff)
{
	precomputedMillerLoop2mixed(f, P1, Q1, P2, Q2coeff.data());
}
#endif

/*
	e = prod_i ML(Pvec[i], Qvec[i])
	if initF:
	  _f = e
	else:
	  _f *= e
*/
template<size_t N>
inline void millerLoopVecN(Fp12& _f, const G1* Pvec, const G2* Qvec, size_t n, bool initF)
{
	assert(n <= N);
	G1 P[N];
	G2 Q[N];
	// remove zero elements
	{
		size_t realN = 0;
		for (size_t i = 0; i < n; i++) {
			if (!Pvec[i].isZero() && !Qvec[i].isZero()) {
				G1::normalize(P[realN], Pvec[i]);
				G2::normalize(Q[realN], Qvec[i]);
				realN++;
			}
		}
		if (realN <= 0) {
			if (initF) _f = 1;
			return;
		}
		n = realN; // update n
	}
	Fp12 ff;
	Fp12& f(initF ? _f : ff);
	// all P[] and Q[] are not zero
	G2 T[N], negQ[N];
	G1 adjP[N];
	Fp6 d, e;
	for (size_t i = 0; i < n; i++) {
		T[i] = Q[i];
		if (BN::param.useNAF) {
			G2::neg(negQ[i], Q[i]);
		}
		makeAdjP(adjP[i], P[i]);
		dblLine(d, T[i], adjP[i]);
		addLine(e, T[i], Q[i], P[i]);
		if (i == 0) {
			mulSparse2(f, d, e);
		} else {
			Fp12 ft;
			mulSparse2(ft, d, e);
			f *= ft;
		}
	}
	for (size_t j = 2; j < BN::param.siTbl.size(); j++) {
		Fp12::sqr(f, f);
		for (size_t i = 0; i < n; i++) {
			dblLine(e, T[i], adjP[i]);
			mulSparse(f, e);
			int v = BN::param.siTbl[j];
			if (v) {
				if (v > 0) {
					addLine(e, T[i], Q[i], P[i]);
				} else {
					addLine(e, T[i], negQ[i], P[i]);
				}
				mulSparse(f, e);
			}
		}
	}
	if (BN::param.z < 0) {
		Fp6::neg(f.b, f.b);
	}
	if (BN::param.isBLS12) goto EXIT;
	for (size_t i = 0; i < n; i++) {
		if (BN::param.z < 0) {
			G2::neg(T[i], T[i]);
		}
		Frobenius(Q[i], Q[i]);
		addLine(d, T[i], Q[i], P[i]);
		Frobenius(Q[i], Q[i]);
		G2::neg(Q[i], Q[i]);
		addLine(e, T[i], Q[i], P[i]);
		Fp12 ft;
		mulSparse2(ft, d, e);
		f *= ft;
	}
EXIT:
	if (!initF) _f *= f;
}
/*
	_f = prod_{i=0}^{n-1} millerLoop(Pvec[i], Qvec[i])
	if initF:
	  f = _f
	else:
	  f *= _f
*/
inline void millerLoopVec(Fp12& f, const G1* Pvec, const G2* Qvec, size_t n, bool initF = true)
{
	const size_t N = 16;
	size_t remain = fp::min_(N, n);
	millerLoopVecN<N>(f, Pvec, Qvec, remain, initF);
	for (size_t i = remain; i < n; i += N) {
		remain = fp::min_(n - i, N);
		millerLoopVecN<N>(f, Pvec + i, Qvec + i, remain, false);
	}
}

// multi thread version of millerLoopVec
// the num of thread is automatically detected if cpuN = 0
inline void millerLoopVecMT(Fp12& f, const G1* Pvec, const G2* Qvec, size_t n, size_t cpuN = 0)
{
	if (n == 0) {
		f = 1;
		return;
	}
#ifdef MCL_USE_OMP
	const size_t minN = 16;
	if (cpuN == 0) {
		cpuN = omp_get_num_procs();
		if (n < minN * cpuN) {
			cpuN = (n + minN - 1) / minN;
		}
	}
	if (cpuN <= 1 || n <= cpuN) {
		millerLoopVec(f, Pvec, Qvec, n);
		return;
	}
	Fp12 *fs = (Fp12*)CYBOZU_ALLOCA(sizeof(Fp12) * cpuN);
	size_t q = n / cpuN;
	size_t r = n % cpuN;
	#pragma omp parallel for
	for (size_t i = 0; i < cpuN; i++) {
		size_t adj = q * i + fp::min_(i, r);
		millerLoopVec(fs[i], Pvec + adj, Qvec + adj, q + (i < r));
	}
	f = 1;
//	#pragma omp declare reduction(red:Fp12:omp_out *= omp_in) initializer(omp_priv = omp_orig)
//	#pragma omp parallel for reduction(red:f)
	for (size_t i = 0; i < cpuN; i++) {
		f *= fs[i];
	}
#else
	(void)cpuN;
	millerLoopVec(f, Pvec, Qvec, n);
#endif
}

inline bool setMapToMode(int mode)
{
	return BN::nonConstParam.mapTo.setMapToMode(mode);
}
inline int getMapToMode()
{
	return BN::param.mapTo.mapToMode_;
}
inline void mapToG1(bool *pb, G1& P, const Fp& x) { *pb = BN::param.mapTo.calc(P, x); }
inline void mapToG2(bool *pb, G2& P, const Fp2& x) { *pb = BN::param.mapTo.calc(P, x); }
#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void mapToG1(G1& P, const Fp& x)
{
	bool b;
	mapToG1(&b, P, x);
	if (!b) throw cybozu::Exception("mapToG1:bad value") << x;
}
inline void mapToG2(G2& P, const Fp2& x)
{
	bool b;
	mapToG2(&b, P, x);
	if (!b) throw cybozu::Exception("mapToG2:bad value") << x;
}
#endif
inline void hashAndMapToG1(G1& P, const void *buf, size_t bufSize)
{
	int mode = getMapToMode();
	if (mode == MCL_MAP_TO_MODE_HASH_TO_CURVE_07) {
		BN::param.mapTo.mapTo_WB19_.msgToG1(P, buf, bufSize);
		return;
	}
	Fp t;
	t.setHashOf(buf, bufSize);
	bool b;
	mapToG1(&b, P, t);
	// It will not happen that the hashed value is equal to special value
	assert(b);
	(void)b;
}
inline void hashAndMapToG2(G2& P, const void *buf, size_t bufSize)
{
	int mode = getMapToMode();
	if (mode == MCL_MAP_TO_MODE_WB19 || mode >= MCL_MAP_TO_MODE_HASH_TO_CURVE_06) {
		BN::param.mapTo.mapTo_WB19_.msgToG2(P, buf, bufSize);
		return;
	}
	Fp2 t;
	t.a.setHashOf(buf, bufSize);
	t.b.clear();
	bool b;
	mapToG2(&b, P, t);
	// It will not happen that the hashed value is equal to special value
	assert(b);
	(void)b;
}
inline void hashAndMapToG1(G1& P, const void *buf, size_t bufSize, const char *dst, size_t dstSize)
{
	BN::param.mapTo.mapTo_WB19_.msgToG1(P, buf, bufSize, dst, dstSize);
}
inline void hashAndMapToG2(G2& P, const void *buf, size_t bufSize, const char *dst, size_t dstSize)
{
	BN::param.mapTo.mapTo_WB19_.msgToG2(P, buf, bufSize, dst, dstSize);
}
// set the default dst for G1
// return 0 if success else -1
inline bool setDstG1(const char *dst, size_t dstSize)
{
	return BN::nonConstParam.mapTo.mapTo_WB19_.dstG1.set(dst, dstSize);
}
// set the default dst for G2
// return 0 if success else -1
inline bool setDstG2(const char *dst, size_t dstSize)
{
	return BN::nonConstParam.mapTo.mapTo_WB19_.dstG2.set(dst, dstSize);
}
#ifndef CYBOZU_DONT_USE_STRING
inline void hashAndMapToG1(G1& P, const std::string& str)
{
	hashAndMapToG1(P, str.c_str(), str.size());
}
inline void hashAndMapToG2(G2& P, const std::string& str)
{
	hashAndMapToG2(P, str.c_str(), str.size());
}
#endif
inline void verifyOrderG1(bool doVerify)
{
	if (BN::param.isBLS12) {
		G1::setOrder(doVerify ? BN::param.r : 0);
	}
}
inline void verifyOrderG2(bool doVerify)
{
	G2::setOrder(doVerify ? BN::param.r : 0);
}

/*
	Faster Subgroup Checks for BLS12-381
	Sean Bowe, https://eprint.iacr.org/2019/814
	Frob^2(P) - z Frob^3(P) == P
*/
inline bool isValidOrderBLS12(const G2& P)
{
	G2 T2, T3;
	Frobenius2(T2, P);
	Frobenius(T3, T2);
	G2::mulGeneric(T3, T3, BN::param.z);
	T2 -= T3;
	return T2 == P;
}
/*
	z2 = (z^2-1)/3, c2 = (-1 + sqrt(-3))/2
	P = (x, y), T1 = (c2 x, y), T0 = (c2^2 x, y)
	z2(2 T0 - P - T1) == T1
*/
inline bool isValidOrderBLS12(const G1& P)
{
	G1 T0, T1;
	T1 = P;
	T1.x *= BN::param.mapTo.c2_;
	T0 = T1;
	T0.x *= BN::param.mapTo.c2_;
	G1::dbl(T0, T0);
	T0 -= P;
	T0 -= T1;
	G1::mulGeneric(T0, T0, BN::param.mapTo.z2_);
	return T0 == T1;
}

// backward compatibility
using mcl::CurveParam;
static const CurveParam& CurveFp254BNb = BN254;
static const CurveParam& CurveFp382_1 = BN381_1;
static const CurveParam& CurveFp382_2 = BN381_2;
static const CurveParam& CurveFp462 = BN462;
static const CurveParam& CurveSNARK1 = BN_SNARK1;

/*
	FrobeniusOnTwist for Dtype
	p mod 6 = 1, w^6 = xi
	Frob(x', y') = phi Frob phi^-1(x', y')
	= phi Frob (x' w^2, y' w^3)
	= phi (x'^p w^2p, y'^p w^3p)
	= (F(x') w^2(p - 1), F(y') w^3(p - 1))
	= (F(x') g^2, F(y') g^3)

	FrobeniusOnTwist for Dtype
	use (1/g) instead of g
*/
inline void Frobenius(G2& D, const G2& S)
{
	Fp2::Frobenius(D.x, S.x);
	Fp2::Frobenius(D.y, S.y);
	Fp2::Frobenius(D.z, S.z);
	D.x *= BN::param.g2;
	D.y *= BN::param.g3;
}
inline void Frobenius2(G2& D, const G2& S)
{
	Frobenius(D, S);
	Frobenius(D, D);
}
inline void Frobenius3(G2& D, const G2& S)
{
	Frobenius(D, S);
	Frobenius(D, D);
	Frobenius(D, D);
}

namespace BN {

using namespace mcl::bn; // backward compatibility

inline void init(bool *pb, const mcl::CurveParam& cp = mcl::BN254, fp::Mode mode = fp::FP_AUTO)
{
	BN::nonConstParam.init(pb, cp, mode);
	if (!*pb) return;
	G1::setMulArrayGLV(local::GLV1::mulArrayGLV, local::GLV1::mulVecNGLV);
	G2::setMulArrayGLV(local::mulArrayGLV2, local::mulVecNGLV2);
	Fp12::setPowArrayGLV(local::powArrayGLV2, local::powVecNGLV2);
	G1::setCompressedExpression();
	G2::setCompressedExpression();
	verifyOrderG1(false);
	verifyOrderG2(false);
	if (BN::param.isBLS12) {
		G1::setVerifyOrderFunc(isValidOrderBLS12);
		G2::setVerifyOrderFunc(isValidOrderBLS12);
	}
	*pb = true;
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void init(const mcl::CurveParam& cp = mcl::BN254, fp::Mode mode = fp::FP_AUTO)
{
	bool b;
	init(&b, cp, mode);
	if (!b) throw cybozu::Exception("BN:init");
}
#endif

} // mcl::bn::BN

inline void initPairing(bool *pb, const mcl::CurveParam& cp = mcl::BN254, fp::Mode mode = fp::FP_AUTO)
{
	BN::init(pb, cp, mode);
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
inline void initPairing(const mcl::CurveParam& cp = mcl::BN254, fp::Mode mode = fp::FP_AUTO)
{
	bool b;
	BN::init(&b, cp, mode);
	if (!b) throw cybozu::Exception("bn:initPairing");
}
#endif

inline void initG1only(bool *pb, const mcl::EcParam& para)
{
	BN::nonConstParam.initG1only(pb, para);
	if (!*pb) return;
	G1::setMulArrayGLV(0);
	G2::setMulArrayGLV(0);
	Fp12::setPowArrayGLV(0);
	G1::setCompressedExpression();
	G2::setCompressedExpression();
}

inline const G1& getG1basePoint()
{
	return BN::param.basePoint;
}

} } // mcl::bn

namespace mcl { namespace local {
template<>
inline void mulByCofactorBLS12fast(mcl::MCL_NAMESPACE_BN::G2& Q, const mcl::MCL_NAMESPACE_BN::G2& P)
{
	mcl::MCL_NAMESPACE_BN::BN::param.mapTo.mulByCofactorBLS12fast(Q, P);
}
} } // mcl::local
#endif
