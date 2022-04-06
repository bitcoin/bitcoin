#include <mcl/bls12_381.hpp>

using namespace mcl::bn;

void minimum_sample(const G1& P, const G2& Q)
{
	Fr a;
	const Fr b = 456;
	Fp12 e1, e2;
	pairing(e1, P, Q);
	G2 aQ;
	G1 bP;
	a.setHashOf("abc");
	printf("a = %s\n", a.getStr(16).c_str());
	printf("a - b = %s\n", (a - b).getStr(16).c_str());
	G2::mul(aQ, Q, a);
	G1::mul(bP, P, b);
	pairing(e2, bP, aQ);
	Fp12::pow(e1, e1, a * b);
	printf("pairing = %s\n", e1.getStr(16).c_str());
	printf("%s\n", e1 == e2 ? "ok" : "ng");
}

void miller_and_finel_exp(const G1& P, const G2& Q)
{
	Fp12 e1, e2;
	pairing(e1, P, Q);

	millerLoop(e2, P, Q);
	finalExp(e2, e2);
	printf("%s\n", e1 == e2 ? "ok" : "ng");
}

void precomputed(const G1& P, const G2& Q)
{
	Fp12 e1, e2;
	pairing(e1, P, Q);
	std::vector<Fp6> Qcoeff;
	precomputeG2(Qcoeff, Q);
	precomputedMillerLoop(e2, P, Qcoeff);
	finalExp(e2, e2);
	printf("%s\n", e1 == e2 ? "ok" : "ng");
}

int main(int argc, char *[])
{
	if (argc == 1) {
		initPairing(mcl::BLS12_381);
		puts("BLS12_381");
	} else {
		initPairing(mcl::BN254);//, mcl::fp::FP_GMP);
		puts("BN254");
	}
	G1 P;
	G2 Q;
	hashAndMapToG1(P, "abc", 3);
	hashAndMapToG2(Q, "abc", 3);
	printf("P = %s\n", P.serializeToHexStr().c_str());
	printf("Q = %s\n", Q.serializeToHexStr().c_str());

	minimum_sample(P, Q);
	miller_and_finel_exp(P, Q);
	precomputed(P, Q);
}

