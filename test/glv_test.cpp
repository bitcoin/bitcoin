#define PUT(x) std::cout << #x "=" << (x) << std::endl;
#include <cybozu/test.hpp>
#include <cybozu/xorshift.hpp>
#include <cybozu/benchmark.hpp>

#if 1
#include <mcl/bn384.hpp>
using namespace mcl::bn384;
#else
#include <mcl/bn256.hpp>
using namespace mcl::bn256;
#endif

#define PUT(x) std::cout << #x "=" << (x) << std::endl;

/*
	Skew Frobenius Map and Efficient Scalar Multiplication for Pairing-Based Cryptography
	Y. Sakemi, Y. Nogami, K. Okeya, H. Kato, Y. Morikawa
*/
struct oldGLV {
	Fp w; // (-1 + sqrt(-3)) / 2
	mpz_class r;
	mpz_class v; // 6z^2 + 4z + 1 > 0
	mpz_class c; // 2z + 1
	void init(const mpz_class& r, const mpz_class& z)
	{
		if (!Fp::squareRoot(w, -3)) throw cybozu::Exception("oldGLV:init");
		w = (w - 1) / 2;
		this->r = r;
		v = 1 + z * (4 + z * 6);
		c = 2 * z + 1;
	}
	/*
		(p^2 mod r) (x, y) = (wx, -y)
	*/
	void mulP2(G1& Q, const G1& P) const
	{
		Fp::mul(Q.x, P.x, w);
		Fp::neg(Q.y, P.y);
		Q.z = P.z;
	}
	/*
		x = ap^2 + b mod r
		assume(x < r);
	*/
	void split(mpz_class& a, mpz_class& b, const mpz_class& x) const
	{
		assert(0 < x && x < r);
		/*
			x = s1 * v + s2                  // s1 = x / v, s2 = x % v
			= s1 * c * p^2 + s2              // vP = cp^2 P
			= (s3 * v + s4) * p^2 + s2       // s3 = (s1 * c) / v, s4 = (s1 * c) % v
			= (s3 * c * p^2 + s4) * p^2 + s2
			= (s3 * c) * p^4 + s4 * p^2 + s2 // s5 = s3 * c, p^4 = p^2 - 1
			= s5 * (p^2 - 1) + s4 * p^2 + s2
			= (s4 + s5) * p^2 + (s2 - s5)
		*/
		mpz_class t;
		mcl::gmp::divmod(a, t, x, v); // a = t / v, t = t % v
		a *= c;
		mcl::gmp::divmod(b, a, a, v); // b = a / v, a = a % v
		b *= c;
		a += b;
		b = t - b;
	}
	template<class G1>
	void mul(G1& Q, const G1& P, const mpz_class& x) const
	{
		G1 A, B;
		mpz_class a, b;
		split(a, b, x);
		mulP2(A, P);
		G1::mul(A, A, a);
		G1::mul(B, P, b);
		G1::add(Q, A, B);
	}
};

template<class GLV1, class GLV2>
void compareLength(const GLV2& lhs)
{
	cybozu::XorShift rg;
	int lt = 0;
	int eq = 0;
	int gt = 0;
	mpz_class R[2];
	mpz_class L0, L1, x;
	mpz_class& R0 = R[0];
	mpz_class& R1 = R[1];
	Fr r;
	for (int i = 1; i < 1000; i++) {
		r.setRand(rg);
		x = r.getMpz();
		mcl::bn::local::GLV1::split(R,x);
		lhs.split(L0, L1, x);

		size_t R0n = mcl::gmp::getBitSize(R0);
		size_t R1n = mcl::gmp::getBitSize(R1);
		size_t L0n = mcl::gmp::getBitSize(L0);
		size_t L1n = mcl::gmp::getBitSize(L1);
		size_t Rn = std::max(R0n, R1n);
		size_t Ln = std::max(L0n, L1n);
		if (Rn == Ln) {
			eq++;
		}
		if (Rn > Ln) {
			gt++;
		}
		if (Rn < Ln) {
			lt++;
		}
	}
	printf("#of{<} = %d, #of{=} = %d #of{>} = %d\n", lt, eq, gt);
}

void testGLV1()
{
	G1 P0, P1, P2;
	mapToG1(P0, 1);
	cybozu::XorShift rg;

	oldGLV oldGlv;
	if (!BN::param.isBLS12) {
		oldGlv.init(BN::param.r, BN::param.z);
	}

	typedef mcl::bn::local::GLV1 GLV1;
	GLV1::initForBN(BN::param.z, BN::param.isBLS12);
	if (!BN::param.isBLS12) {
		compareLength<GLV1>(oldGlv);
	}

	for (int i = 1; i < 100; i++) {
		mapToG1(P0, i);
		Fr s;
		s.setRand(rg);
		mpz_class ss = s.getMpz();
		G1::mulGeneric(P1, P0, ss);
		GLV1::mul(P2, P0, ss);
		CYBOZU_TEST_EQUAL(P1, P2);
		GLV1::mul(P2, P0, ss, true);
		CYBOZU_TEST_EQUAL(P1, P2);
		if (!BN::param.isBLS12) {
			oldGlv.mul(P2, P0, ss);
			CYBOZU_TEST_EQUAL(P1, P2);
		}
	}
	for (int i = -100; i < 100; i++) {
		mpz_class ss = i;
		G1::mulGeneric(P1, P0, ss);
		GLV1::mul(P2, P0, ss);
		CYBOZU_TEST_EQUAL(P1, P2);
		GLV1::mul(P2, P0, ss, true);
		CYBOZU_TEST_EQUAL(P1, P2);
	}
#ifndef NDEBUG
	puts("skip testGLV1 in debug");
	Fr s;
	mapToG1(P0, 123);
	CYBOZU_BENCH_C("Ec::mul", 100, P1 = P0; s.setRand(rg); G1::mulGeneric, P2, P1, s.getMpz());
	CYBOZU_BENCH_C("Ec::glv", 100, P1 = P0; s.setRand(rg); GLV1::mul, P2, P1, s.getMpz());
#endif
}

/*
	lambda = 6 * z * z
	mul (lambda * 2) = FrobeniusOnTwist * 2
*/
void testGLV2()
{
	typedef local::GLV2 GLV2;
	G2 Q0, Q1, Q2;
	mpz_class z = BN::param.z;
	mpz_class r = BN::param.r;
	GLV2::init(z, BN::param.isBLS12);
	mpz_class n;
	cybozu::XorShift rg;
	mapToG2(Q0, 1);
	for (int i = -10; i < 10; i++) {
		n = i;
		G2::mulGeneric(Q1, Q0, n);
		GLV2::mul(Q2, Q0, n);
		CYBOZU_TEST_EQUAL(Q1, Q2);
	}
	for (int i = 1; i < 100; i++) {
		mcl::gmp::getRand(n, GLV2::rBitSize, rg);
		n %= r;
		n -= r/2;
		mapToG2(Q0, i);
		G2::mulGeneric(Q1, Q0, n);
		GLV2::mul(Q2, Q0, n);
		CYBOZU_TEST_EQUAL(Q1, Q2);
	}
#ifndef NDEBUG
	puts("skip testGLV2 in debug");
	Fr s;
	mapToG2(Q0, 123);
	CYBOZU_BENCH_C("G2::mul", 1000, Q2 = Q0; s.setRand(rg); G2::mulGeneric, Q2, Q1, s.getMpz());
	CYBOZU_BENCH_C("G2::glv", 1000, Q1 = Q0; s.setRand(rg); GLV2::mul, Q2, Q1, s.getMpz());
#endif
}

void testGT()
{
	G1 P;
	G2 Q;
	GT x, y, z;
	hashAndMapToG1(P, "abc", 3);
	hashAndMapToG2(Q, "abc", 3);
	pairing(x, P, Q);
	int n = 200;
	y = x;
	for (int i = 0; i < n; i++) {
		y *= y;
	}
	mpz_class t = 1;
	t <<= n;
	GT::pow(z, x, t);
	CYBOZU_TEST_EQUAL(y, z);
}

CYBOZU_TEST_AUTO(glv)
{
	const mcl::CurveParam tbl[] = {
		mcl::BN254,
		mcl::BN381_1,
		mcl::BN381_2,
		mcl::BLS12_381,
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const mcl::CurveParam& cp = tbl[i];
		initPairing(cp);
		testGLV1();
		testGLV2();
		testGT();
	}
}
