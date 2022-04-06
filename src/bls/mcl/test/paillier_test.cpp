#include <cybozu/test.hpp>
#include <mcl/paillier.hpp>

CYBOZU_TEST_AUTO(paillier)
{
	using namespace mcl::paillier;
	SecretKey sec;
#ifndef NDEBUG
	sec.init(512);
#else
	sec.init(2048);
#endif
	PublicKey pub;
	sec.getPublicKey(pub);
	mpz_class m1("12342340928409"), m2("23049820498204");
	mpz_class c1, c2, c3;
	pub.enc(c1, m1);
	pub.enc(c2, m2);
	std::cout << std::hex << "c1=" << c1 << "\nc2=" << c2 << std::endl;
	pub.add(c3, c1, c2);
	mpz_class d1, d2, d3;
	sec.dec(d1, c1);
	sec.dec(d2, c2);
	sec.dec(d3, c3);
	CYBOZU_TEST_EQUAL(m1, d1);
	CYBOZU_TEST_EQUAL(m2, d2);
	CYBOZU_TEST_EQUAL(m1 + m2, d3);
}
