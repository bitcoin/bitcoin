/*
	sample of somewhat homomorphic encryption(SHE)
*/
#define PUT(x) std::cout << #x << "=" << (x) << std::endl;
#include <cybozu/benchmark.hpp>
#include <mcl/she.hpp>

using namespace mcl::she;

void miniSample()
{
	// init library
	SHE::init();

	SecretKey sec;

	// init secret key by random_device
	sec.setByCSPRNG();

	// set range to decode GT DLP
	SHE::setRangeForDLP(1000);

	PublicKey pub;
	// get public key
	sec.getPublicKey(pub);

	const int N = 5;
	int a[] = { 1, 5, -3, 4, 6 };
	int b[] = { 4, 2, 1, 9, -2 };
	// compute correct value
	int sum = 0;
	for (size_t i = 0; i < N; i++) {
		sum += a[i] * b[i];
	}

	std::vector<CipherText> ca(N), cb(N);

	// encrypt each a[] and b[]
	for (size_t i = 0; i < N; i++) {
		pub.enc(ca[i], a[i]);
		pub.enc(cb[i], b[i]);
	}
	CipherText c;
	c.clearAsMultiplied(); // clear as multiplied before using c.add()
	// inner product of encrypted vector
	for (size_t i = 0; i < N; i++) {
		CipherText t;
		CipherText::mul(t, ca[i], cb[i]); // t = ca[i] * cb[i]
		c.add(t); // c += t
	}
	// decode it
	int m = (int)sec.dec(c);
	// verify the value
	if (m == sum) {
		puts("ok");
	} else {
		printf("err correct %d err %d\n", sum, m);
	}
}

void usePrimitiveCipherText()
{
	// init library
	SHE::init();

	SecretKey sec;

	// init secret key by random_device
	sec.setByCSPRNG();

	// set range to decode GT DLP
	SHE::setRangeForGTDLP(100);

	PublicKey pub;
	// get public key
	sec.getPublicKey(pub);

	int a1 = 1, a2 = 2;
	int b1 = 5, b2 = -4;
	CipherTextG1 c1, c2; // size of CipherTextG1 = N * 2 ; N = 256-bit for CurveFp254BNb
	CipherTextG2 d1, d2; // size of CipherTextG2 = N * 4
	pub.enc(c1, a1);
	pub.enc(c2, a2);
	pub.enc(d1, b1);
	pub.enc(d2, b2);
	c1.add(c2); // CipherTextG1 is additive HE
	d1.add(d2); // CipherTextG2 is additive HE
	CipherTextGT cm; // size of CipherTextGT = N * 12 * 4
	CipherTextGT::mul(cm, c1, d1); // cm = c1 * d1
	cm.add(cm); // 2cm
	int m = (int)sec.dec(cm);
	int ok = (a1 + a2) * (b1 + b2) * 2;
	if (m == ok) {
		puts("ok");
	} else {
		printf("err m=%d ok=%d\n", m, ok);
	}
	std::string s;
	s = c1.getStr(mcl::IoSerialize); // serialize
	printf("c1 data size %d byte\n", (int)s.size());

	c2.setStr(s, mcl::IoSerialize);
	printf("deserialize %s\n", c1 == c2 ? "ok" : "ng");

	s = d1.getStr(mcl::IoSerialize); // serialize
	printf("d1 data size %d byte\n", (int)s.size());
	d2.setStr(s, mcl::IoSerialize);
	printf("deserialize %s\n", d1 == d2 ? "ok" : "ng");

	s = cm.getStr(mcl::IoSerialize); // serialize
	printf("cm data size %d byte\n", (int)s.size());
	CipherTextGT cm2;
	cm2.setStr(s, mcl::IoSerialize);
	printf("deserialize %s\n", cm == cm2 ? "ok" : "ng");
}

int main()
	try
{
	miniSample();
	usePrimitiveCipherText();
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	return 1;
}
