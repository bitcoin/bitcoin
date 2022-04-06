/*
	dirty hack to make multi instance of pairing functions
*/
#include <iostream>
// BLS12-381 ; sizeof(Fp) = 48, sizeof(Fr) = 32
#define MCL_MAX_FP_BIT_SIZE 384
#define MCL_MAX_FR_BIT_SIZE 256
#include <mcl/bn.hpp>
// remove include gurad of bn.hpp
#undef MCL_INCLUDE_MCL_BN_HPP
// define other fp size
// BN254 ; sizeof(Fp) = 32, sizeof(Fr) = 32
#undef MCL_MAX_FP_BIT_SIZE
#define MCL_MAX_FP_BIT_SIZE 256
// define another namespace instead of bn
#undef MCL_NAMESPACE_BN
#define MCL_NAMESPACE_BN bn2
#include <mcl/bn.hpp>

#define PUT(x) std::cout << #x "=" << (x) << std::endl;
int main()
	try
{
	using namespace mcl;
	mpz_class a = 123;
	mpz_class b = 456;
	bn::initPairing(mcl::BLS12_381);
	bn2::initPairing(mcl::BN254);

	bn::G1 P1;
	bn::G2 Q1;
	bn::GT e1, f1;

	bn2::G1 P2;
	bn2::G2 Q2;
	bn2::GT e2, f2;

	bn::hashAndMapToG1(P1, "abc", 3);
	bn2::hashAndMapToG1(P2, "abc", 3);
	PUT(P1);
	PUT(P2);

	bn::hashAndMapToG2(Q1, "abc", 3);
	bn2::hashAndMapToG2(Q2, "abc", 3);

	PUT(Q1);
	PUT(Q2);
	P1 += P1;
	Q2 += Q2;

	bn::pairing(e1, P1, Q1);
	bn2::pairing(e2, P2, Q2);
	P1 *= a;
	Q1 *= b;
	P2 *= a;
	Q2 *= b;
	bn::pairing(f1, P1, Q1);
	bn2::pairing(f2, P2, Q2);
	bn::GT::pow(e1, e1, a * b);
	bn2::GT::pow(e2, e2, a * b);
	printf("eq %d %d\n", e1 == f1, e2 == f2);
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	return 1;
}
