#define PUT(x) std::cout << #x "=" << (x) << std::endl;
#include <stdlib.h>
#include <stdio.h>
void put(const void *buf, size_t bufSize)
{
	const unsigned char* p = (const unsigned char*)buf;
	for (size_t i = 0; i < bufSize; i++) {
		printf("%02x", p[i]);
	}
	printf("\n");
}
#include <mcl/ecdsa.hpp>
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>

using namespace mcl::ecdsa;

CYBOZU_TEST_AUTO(ecdsa)
{
	init();
	SecretKey sec;
	PublicKey pub;
	sec.setByCSPRNG();
	getPublicKey(pub, sec);
	Signature sig;
	const std::string msg = "hello";
	sign(sig, sec, msg.c_str(), msg.size());
	CYBOZU_TEST_ASSERT(verify(sig, pub, msg.c_str(), msg.size()));
	sig.s += 1;
	CYBOZU_TEST_ASSERT(!verify(sig, pub, msg.c_str(), msg.size()));
}

CYBOZU_TEST_AUTO(mul)
{
	mcl::ecdsa::Fp x = -3, y;
	const mpz_class p = mcl::ecdsa::Fp::getOp().mp;
	for (int i = 0; i < 100; i++) {
		Fp::pow(y, x, p - 1);
		CYBOZU_TEST_EQUAL(y, 1);
		x = x - 1;
	}
	CYBOZU_TEST_EQUAL(Fp(-1) * Fp(-1), 1);
}

void serializeStrTest(const std::string& msg, const std::string& secHex, const std::string& pubHex, const std::string& sigHex)
{
	// old serialization
	setSeriailzeMode(SerializeOld);
	SecretKey sec;
	sec.setStr(secHex, 16);
	CYBOZU_TEST_EQUAL(sec.getStr(16), secHex);
	PublicKey pub;
	getPublicKey(pub, sec);
	pub.normalize();
	const std::string xHex = pubHex.substr(0, 64);
	const std::string yHex = pubHex.substr(64, 64);
	Ec t(Fp(xHex, 16), Fp(yHex, 16));
	CYBOZU_TEST_EQUAL(pub, t);
	Signature sig;
	const std::string rHex = sigHex.substr(0, 64);
	const std::string sHex = sigHex.substr(64, 64);
	sig.r.setStr(rHex, 16);
	sig.s.setStr(sHex, 16);
	normalizeSignature(sig);
	CYBOZU_TEST_ASSERT(verify(sig, pub, msg.c_str(), msg.size()));
	// recover mode
	setSeriailzeMode(SerializeBitcoin);
}

void serializeBinaryTest(const std::string& msg, const std::string& secHex, const std::string& pubHex, const std::string& sigHex)
{
	// old serialization
	setSeriailzeMode(SerializeOld);

	SecretKey sec;
	sec.deserializeHexStr(secHex);
	CYBOZU_TEST_EQUAL(sec.serializeToHexStr(), secHex);
	PublicKey pub;
	getPublicKey(pub, sec);
	pub.normalize();
#if 0
	Ec t;
	t.deserializeHexStr(pubHex);
#else
	const std::string xHex = pubHex.substr(0, 64);
	const std::string yHex = pubHex.substr(64, 64);
	Ec t;
	t.x.deserializeHexStr(xHex);
	t.y.deserializeHexStr(yHex);
	t.z = 1;
#endif
	CYBOZU_TEST_EQUAL(pub, t);
	Signature sig;
	sig.deserializeHexStr(sigHex);
	normalizeSignature(sig);
	CYBOZU_TEST_ASSERT(verify(sig, pub, msg.c_str(), msg.size()));
	// recover mode
	setSeriailzeMode(SerializeBitcoin);
	char buf[100];
	size_t n = sig.serialize(buf, sizeof(buf));
	CYBOZU_TEST_ASSERT(n > 0);
	Signature sig2;
	CYBOZU_TEST_EQUAL(sig2.deserialize(buf, n), n);
	CYBOZU_TEST_EQUAL(sig.r, sig2.r);
	CYBOZU_TEST_EQUAL(sig.s, sig2.s);
}

CYBOZU_TEST_AUTO(value)
{
	const struct Tbl {
		const char *msg;
		const char *sec;
		const char *pub;
		const char *sig;
	} tbl[] = {
		{
			"hello",
			"83ecb3984a4f9ff03e84d5f9c0d7f888a81833643047acc58eb6431e01d9bac8",
			"653bd02ba1367e5d4cd695b6f857d1cd90d4d8d42bc155d85377b7d2d0ed2e7104e8f5da403ab78decec1f19e2396739ea544e2b14159beb5091b30b418b813a",
			"a598a8030da6d86c6bc7f2f5144ea549d28211ea58faa70ebf4c1e665c1fe9b5de5d79a2ba44e311d04fdca263639283965780bce9169822be9cc81756e95a24"
		},
		// generated data from Python:ecdsa with ecdsa.SigningKey.generate(curve=ecdsa.SECP256k1, hashfunc=hashlib.sha256)
		{
			"hello",
			"b1aa6282b14e5ffbf6d12f783612f804e6a20d1a9734ffbb6c9923c670ee8da2",
			"0a09ff142d94bc3f56c5c81b75ea3b06b082c5263fbb5bd88c619fc6393dda3da53e0e930892cdb7799eea8fd45b9fff377d838f4106454289ae8a080b111f8d",
			"50839a97404c24ec39455b996e4888477fd61bcf0ffb960c7ffa3bef104501919671b8315bb5c1611d422d49cbbe7e80c6b463215bfad1c16ca73172155bf31a"
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		serializeStrTest(tbl[i].msg, tbl[i].sec, tbl[i].pub, tbl[i].sig);
		serializeBinaryTest(tbl[i].msg, tbl[i].sec, tbl[i].pub, tbl[i].sig);
	}
}

CYBOZU_TEST_AUTO(serializePublicKey)
{
	// old serialization
	setSeriailzeMode(SerializeOld);

	SecretKey sec;
	PublicKey pub, pub2;
	sec.setByCSPRNG();
	getPublicKey(pub, sec);
	uint8_t buf1[64];
	size_t n = pub.serialize(buf1, 64);
	CYBOZU_TEST_EQUAL(n, 64);
	n = pub2.deserialize(buf1, 64);
	CYBOZU_TEST_EQUAL(n, 64);
	CYBOZU_TEST_EQUAL(pub, pub2);
	pub.normalize();

	// recover mode
	setSeriailzeMode(SerializeBitcoin);
	// uncompressed
	uint8_t buf2[65];
	n = pub.serialize(buf2, 65);
	CYBOZU_TEST_EQUAL(n, 65);
	CYBOZU_TEST_EQUAL(buf2[0], 0x04);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2 + 1, 64);
	pub2.clear();
	n = pub2.deserialize(buf2, 65);
	CYBOZU_TEST_EQUAL(n, 65);
	CYBOZU_TEST_EQUAL(pub, pub2);
	// compressed
	uint8_t buf3[33];
	n = pub.serializeCompressed(buf3, 33);
	CYBOZU_TEST_EQUAL(n, 33);
	CYBOZU_TEST_EQUAL(buf3[0], pub.y.isOdd() ? 0x03 : 0x02);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf3 + 1, 32);
	pub2.clear();
	n = pub2.deserialize(buf3, 33);
	CYBOZU_TEST_EQUAL(n, 33);
	CYBOZU_TEST_EQUAL(pub, pub2);

	// swap even and odd of y
	pub.normalize();
	pub.y = -pub.y;
	uint8_t buf4[33];
	n = pub.serializeCompressed(buf4, 33);
	CYBOZU_TEST_EQUAL(n, 33);
	CYBOZU_TEST_EQUAL(buf4[0], pub.y.isOdd() ? 0x03 : 0x02);
	CYBOZU_TEST_EQUAL_ARRAY(buf3 + 1, buf4 + 1, 32);
}

CYBOZU_TEST_AUTO(serializeDer)
{
	const char *r ="ed81ff192e75a3fd2304004dcadb746fa5e24c5031ccfcf21320b0277457c98f";
	const char *s = "7a986d955c6e0cb35d446a89d3f56100f4d7f67801c31967743a9c8e10615bed";
	Signature sig;
	sig.r.setStr(r, 16);
	sig.s.setStr(s, 16);
	const uint8_t expected[] = {0x30, 0x45, 0x02, 0x21, 0x00, 0xed, 0x81, 0xff, 0x19, 0x2e, 0x75, 0xa3, 0xfd, 0x23, 0x04, 0x00, 0x4d, 0xca, 0xdb, 0x74, 0x6f, 0xa5, 0xe2, 0x4c, 0x50, 0x31, 0xcc, 0xfc, 0xf2, 0x13, 0x20, 0xb0, 0x27, 0x74, 0x57, 0xc9, 0x8f, 0x02, 0x20, 0x7a, 0x98, 0x6d, 0x95, 0x5c, 0x6e, 0x0c, 0xb3, 0x5d, 0x44, 0x6a, 0x89, 0xd3, 0xf5, 0x61, 0x00, 0xf4, 0xd7, 0xf6, 0x78, 0x01, 0xc3, 0x19, 0x67, 0x74, 0x3a, 0x9c, 0x8e, 0x10, 0x61, 0x5b, 0xed};
	uint8_t buf[100];
	size_t n = sig.serialize(buf, sizeof(buf));
	CYBOZU_TEST_EQUAL(n, sizeof(expected));
	CYBOZU_TEST_EQUAL_ARRAY(buf, expected, n);
	Signature sig2;
	CYBOZU_TEST_EQUAL(n, sig2.deserialize(buf, n));
	CYBOZU_TEST_EQUAL(sig.r, sig2.r);
	CYBOZU_TEST_EQUAL(sig.s, sig2.s);
}

CYBOZU_TEST_AUTO(edgeCase)
{
	{
		const Zn tbl[] = { 0, 1, 0x7f, 0x80, 0x7fff, 0x8000, -1 };
		const size_t n = CYBOZU_NUM_OF_ARRAY(tbl);
		for (size_t i = 0; i < n; i++) {
			for (size_t j = 0; j < n; j++) {
				Signature sig;
				sig.r = tbl[i];
				sig.s = tbl[j];
				char buf[128];
				size_t len = sig.serialize(buf, sizeof(buf));
				CYBOZU_TEST_ASSERT(len > 0);
				Signature sig2;
				CYBOZU_TEST_EQUAL(sig2.deserialize(buf, len), len);
				CYBOZU_TEST_EQUAL(sig.r, sig2.r);
				CYBOZU_TEST_EQUAL(sig.s, sig2.s);
			}
		}
	}
	// wrong serialized data
	{
		// correct : { 0x30, 0x06, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00 }
		const uint8_t buf0[] = { 0x31/* bad header */, 0x06, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00 };
		const uint8_t buf1[] = { 0x30, 0x07/* bad length */, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00 };
		const uint8_t buf2[] = { 0x30 };
		const uint8_t buf3[] = { 0x30, 0x80/* too long*/ };
		const uint8_t buf4[] = { 0x30, 0x06, 0x01/* bad marker */, 0x01, 0x00, 0x02, 0x01, 0x00 };
		const uint8_t buf5[] = { 0x30, 0x06, 0x02 };
		const uint8_t buf6[] = { 0x30, 0x06, 0x02, 0x00 };
		const uint8_t buf7[] = { 0x30, 0x06, 0x02, 0x11/* too large */, 0x00, 0x02, 0x01, 0x00 };
		const uint8_t buf8[] = { 0x30, 0x06, 0x02, 0x01, 0x80/*negative*/, 0x02, 0x01, 0x00 };
		// (r, s) where s = char(Zn) + 1
		const uint8_t buf9[] = { 0x30, 0x26, 0x02, 0x01, 0x00, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41 };
		const uint8_t buf10[] = { 0x30, 0x07, 0x02, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00/* redundant zero */ };
		const struct {
			const uint8_t *p;
			size_t n;
		} tbl[] = {
			{ buf0, sizeof(buf0) },
			{ buf1, sizeof(buf1) },
			{ buf2, sizeof(buf2) },
			{ buf3, sizeof(buf3) },
			{ buf4, sizeof(buf4) },
			{ buf5, sizeof(buf5) },
			{ buf6, sizeof(buf6) },
			{ buf7, sizeof(buf7) },
			{ buf8, sizeof(buf8) },
			{ buf9, sizeof(buf9) },
			{ buf10, sizeof(buf10) },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			Signature sig;
			CYBOZU_TEST_EQUAL(sig.deserialize(tbl[i].p, tbl[i].n), 0);
		}
	}
}

#ifdef NDEBUG

template<class T>
void invByPow(T& y, const T& x)
{
	static mpz_class p2 = T::getOp().mp - 2;
	T::pow(y, x, p2);
}

#include <cybozu/xorshift.hpp>

CYBOZU_TEST_AUTO(lowBench)
{
	const int C = 10000;
	{
		cybozu::XorShift rg;
		Fp x, y;
		x.setByCSPRNG(rg);
		y.setByCSPRNG(rg);
		for (int i = 0; i < 4; i++) {
			Fp t1, t2;
			Fp::inv(t1, x);
			invByPow(t2, x);
//			Fp::pow(t2, x, Fp::getOp().mp - 2);
			CYBOZU_TEST_EQUAL(t1, t2);
			x = t1 + x;
		}
		CYBOZU_BENCH_C("Fp::add", C, Fp::add, x, x, y);
		CYBOZU_BENCH_C("Fp::sub", C, Fp::sub, x, x, y);
		CYBOZU_BENCH_C("Fp::mul", C, Fp::mul, x, x, y);
		CYBOZU_BENCH_C("Fp::inv", C, Fp::inv, x, x);
		CYBOZU_BENCH_C("Fp::pow", C, Fp::pow, x, x, y);
		CYBOZU_BENCH_C("Fp::invByPow", C, invByPow, x, x);
	}
	{
		Zn x, y;
		x.setByCSPRNG();
		y.setByCSPRNG();
		CYBOZU_BENCH_C("Zn::add", C, Zn::add, x, x, y);
		CYBOZU_BENCH_C("Zn::sub", C, Zn::sub, x, x, y);
		CYBOZU_BENCH_C("Zn::mul", C, Zn::mul, x, x, y);
		CYBOZU_BENCH_C("Zn::inv", C, Zn::inv, x, x);
		CYBOZU_BENCH_C("Zn::pow", C, Zn::pow, x, x, y);
		CYBOZU_BENCH_C("Zn::invByPow", C, invByPow, x, x);
	}
	{
		Ec P, Q;
		Zn x;
		x.setByCSPRNG();
		P = local::getParam().P;
		Ec::mul(Q, P, x);
		CYBOZU_BENCH_C("Ec::add", C, Ec::add, P, P, Q);
		CYBOZU_BENCH_C("Ec::dbl", C, Ec::dbl, P, P);
		CYBOZU_BENCH_C("Ec::mul", C, Ec::mul, P, P, x);
	}
}

CYBOZU_TEST_AUTO(bench)
{
	const std::string msg = "hello";
	SecretKey sec;
	PublicKey pub;
	PrecomputedPublicKey ppub;
	sec.setByCSPRNG();
	getPublicKey(pub, sec);
	ppub.init(pub);
	Signature sig;
	CYBOZU_BENCH_C("sign", 1000, sign, sig, sec, msg.c_str(), msg.size());
	CYBOZU_BENCH_C("pub.verify ", 1000, verify, sig, pub, msg.c_str(), msg.size());
	CYBOZU_BENCH_C("ppub.verify", 1000, verify, sig, ppub, msg.c_str(), msg.size());
}
#endif
