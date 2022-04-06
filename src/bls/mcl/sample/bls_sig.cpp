/**
	@file
	@brief a sample of BLS signature
	see https://github.com/herumi/bls
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause

*/
#include <mcl/bn256.hpp>
#include <iostream>

using namespace mcl::bn256;

void Hash(G1& P, const std::string& m)
{
	Fp t;
	t.setHashOf(m);
	mapToG1(P, t);
}

void KeyGen(Fr& s, G2& pub, const G2& Q)
{
	s.setRand();
	G2::mul(pub, Q, s); // pub = sQ
}

void Sign(G1& sign, const Fr& s, const std::string& m)
{
	G1 Hm;
	Hash(Hm, m);
	G1::mul(sign, Hm, s); // sign = s H(m)
}

bool Verify(const G1& sign, const G2& Q, const G2& pub, const std::string& m)
{
	Fp12 e1, e2;
	G1 Hm;
	Hash(Hm, m);
	pairing(e1, sign, Q); // e1 = e(sign, Q)
	pairing(e2, Hm, pub); // e2 = e(Hm, sQ)
	return e1 == e2;
}

int main(int argc, char *argv[])
{
	std::string m = argc == 1 ? "hello mcl" : argv[1];

	// setup parameter
	initPairing();
	G2 Q;
	mapToG2(Q, 1);

	// generate secret key and public key
	Fr s;
	G2 pub;
	KeyGen(s, pub, Q);
	std::cout << "secret key " << s << std::endl;
	std::cout << "public key " << pub << std::endl;

	// sign
	G1 sign;
	Sign(sign, s, m);
	std::cout << "msg " << m << std::endl;
	std::cout << "sign " << sign << std::endl;

	// verify
	bool ok = Verify(sign, Q, pub, m);
	std::cout << "verify " << (ok ? "ok" : "ng") << std::endl;
}
