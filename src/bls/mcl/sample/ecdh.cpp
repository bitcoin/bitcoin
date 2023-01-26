/*
	sample of Elliptic Curve Diffie-Hellman key sharing
*/
#include <iostream>
#include <fstream>
#include <mcl/ec.hpp>

typedef mcl::FpT<mcl::FpTag, 256> Fp;
typedef mcl::FpT<mcl::ZnTag, 256> Fr;
typedef mcl::EcT<Fp> Ec;

void put(const char *msg, const Ec& P)
{
	std::cout << msg << P.getStr(mcl::IoEcAffine | 16) << std::endl;
}

int main()
{
	/*
		Ec is an elliptic curve over Fp
		the cyclic group of <P> is isomorphic to Fr
	*/
	Ec P;
	mcl::initCurve<Ec, Fr>(MCL_SECP256K1, &P);
	put("P=", P);

	/*
		Alice setups a private key a and public key aP
	*/
	Fr a;
	Ec aP;

	a.setByCSPRNG();
	Ec::mul(aP, P, a); // aP = a * P;

	put("aP=", aP);

	/*
		Bob setups a private key b and public key bP
	*/
	Fr b;
	Ec bP;

	b.setByCSPRNG();
	Ec::mul(bP, P, b); // bP = b * P;

	put("bP=", bP);

	Ec abP, baP;

	// Alice uses bP(B's public key) and a(A's priavte key)
	Ec::mul(abP, bP, a); // abP = a * (bP)

	// Bob uses aP(A's public key) and b(B's private key)
	Ec::mul(baP, aP, b); // baP = b * (aP)

	if (abP == baP) {
		std::cout << "key sharing succeed:" << abP << std::endl;
	} else {
		std::cout << "ERR(not here)" << std::endl;
	}
}

