#include <mcl/she.hpp>
#include <mcl/elgamal.hpp>
#include <cybozu/benchmark.hpp>

using namespace mcl::she;

const mcl::EcParam& g_para = mcl::ecparam::secp256k1;
//const mcl::EcParam& g_para = mcl::ecparam::secp160k1;

void elgamal()
{
	puts("elgamal");
	struct TagZn;
	typedef mcl::FpT<> Fp;
	typedef mcl::FpT<TagZn> Zn;
	typedef mcl::EcT<Fp> Ec;
	typedef mcl::ElgamalT<Ec, Zn> ElgamalEc;
	Ec P;
	mcl::initCurve<Ec, Zn>(g_para.curveType, &P);
	const size_t bitSize = Zn::getBitSize();
	cybozu::RandomGenerator rg;

	ElgamalEc::PrivateKey sec;
	sec.init(P, bitSize, rg);
	sec.setCache(0, 60000);
	const ElgamalEc::PublicKey& pub = sec.getPublicKey();

	const int m1 = 123;
	const int m2 = 654;
	ElgamalEc::CipherText c1, c2;
	pub.enc(c1, m1);
	pub.enc(c2, m2);
	c1.add(c2);
	Zn dec;
	sec.dec(dec, c1);
	std::cout << "dec=" << dec << std::endl;
	CYBOZU_BENCH_C("enc", 1000, pub.enc, c1, m1);
}

int main()
	try
{
	// initialize system
	initG1only(g_para, 1024);

	SecretKey sec;
	sec.setByCSPRNG();
	PublicKey pub;
	sec.getPublicKey(pub);
	PrecomputedPublicKey ppub;
	ppub.init(pub);

	int m1 = 123;
	int m2 = 654;

	CipherTextG1 c1, c2;
	ppub.enc(c1, m1);
	ppub.enc(c2, m2);
	CYBOZU_BENCH_C("pub.enc", 1000, pub.enc, c1, m1);
	CYBOZU_BENCH_C("ppub.enc", 1000, ppub.enc, c1, m1);
	add(c1, c1, c2);
	int m = sec.dec(c1);
	printf("Dec(Enc(%d) + Enc(%d)) = %d(%s)\n", m1, m2, m, m == m1 + m2 ? "ok" : "ng");

	elgamal();

} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}

