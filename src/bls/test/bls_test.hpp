#include <bls/bls.hpp>
#include <cybozu/test.hpp>
#include <cybozu/inttype.hpp>
#include <cybozu/benchmark.hpp>
#include <cybozu/sha2.hpp>
#include <cybozu/atoi.hpp>
#include <cybozu/file.hpp>
#include <cybozu/xorshift.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

typedef std::vector<uint8_t> Uint8Vec;
typedef std::vector<std::string> StringVec;

Uint8Vec fromHexStr(const std::string& s)
{
	Uint8Vec ret(s.size() / 2);
	for (size_t i = 0; i < s.size(); i += 2) {
		ret[i / 2] = cybozu::hextoi(&s[i], 2);
	}
	return ret;
}

template<class T>
void streamTest(const T& t)
{
	std::ostringstream oss;
	oss << t;
	std::istringstream iss(oss.str());
	T t2;
	iss >> t2;
	CYBOZU_TEST_EQUAL(t, t2);
}

template<class T>
void testSetForBN254()
{
	/*
		mask value to be less than r if the value >= (1 << (192 + 62))
	*/
	const uint64_t fff = uint64_t(-1);
	const uint64_t one = uint64_t(1);
	const struct {
		uint64_t in;
		uint64_t expected;
	} tbl[] = {
		{ fff, (one << 61) - 1 }, // masked with (1 << 61) - 1
		{ one << 62, 0 }, // masked
		{ (one << 62) | (one << 61), (one << 61) }, // masked
		{ (one << 61) - 1, (one << 61) - 1 }, // same
	};
	T t1, t2;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		uint64_t v1[] = { fff, fff, fff, tbl[i].in };
		uint64_t v2[] = { fff, fff, fff, tbl[i].expected };
		t1.set(v1);
		t2.set(v2);
		CYBOZU_TEST_EQUAL(t1, t2);
	}
}

void testForBN254()
{
	CYBOZU_TEST_EQUAL(bls::getOpUnitSize(), 4);
	bls::Id id;
	CYBOZU_TEST_ASSERT(id.isZero());
	id = 5;
	CYBOZU_TEST_EQUAL(id, 5);
	{
		const uint64_t id1[] = { 1, 2, 3, 4 };
		id.set(id1);
		std::ostringstream os;
		os << id;
		CYBOZU_TEST_EQUAL(os.str(), "0x4000000000000000300000000000000020000000000000001");
	}
	testSetForBN254<bls::Id>();
	testSetForBN254<bls::SecretKey>();
}

void setFp2Serialize(char s[96])
{
	mclBnFp r;
	mclBnFp_setByCSPRNG(&r);
	mclBnFp_serialize(s, 48, &r);
	mclBnFp_setByCSPRNG(&r);
	mclBnFp_serialize(s + 48, 48, &r);
}

void hashTest(int type)
{
#ifdef BLS_ETH
	if (type != MCL_BLS12_381) return;
	bls::SecretKey sec;
	sec.init();
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	char h[96];
	setFp2Serialize(h);
	bls::Signature sig;
	sec.signHash(sig, h, sizeof(h));
	CYBOZU_TEST_ASSERT(sig.verifyHash(pub, h, sizeof(h)));
	CYBOZU_TEST_ASSERT(!sig.verifyHash(pub, "\x01\x02\04"));
#else
	bls::SecretKey sec;
	sec.init();
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	const std::string h = "\x01\x02\x03";
	bls::Signature sig;
	sec.signHash(sig, h);
	CYBOZU_TEST_ASSERT(sig.verifyHash(pub, h));
	CYBOZU_TEST_ASSERT(!sig.verifyHash(pub, "\x01\x02\04"));
	if (type == MCL_BN254) {
		CYBOZU_TEST_EXCEPTION(sec.signHash(sig, "", 0), std::exception);
		CYBOZU_TEST_EXCEPTION(sec.signHash(sig, "\x00", 1), std::exception);
		CYBOZU_TEST_EXCEPTION(sec.signHash(sig, "\x00\x00", 2), std::exception);
#ifndef BLS_ETH
		const uint64_t c1[] = { 0x0c00000000000004ull, 0xcf0f000000000006ull, 0x26cd890000000003ull, 0x2523648240000001ull };
		const uint64_t mc1[] = { 0x9b0000000000000full, 0x921200000000000dull, 0x9366c48000000004ull };
		uint8_t buf[64];
		bls::local::convertArray(buf, c1, 4);
		CYBOZU_TEST_EXCEPTION(sec.signHash(sig, buf, 32), std::exception);
		bls::local::convertArray(buf, mc1, 3);
		CYBOZU_TEST_EXCEPTION(sec.signHash(sig, buf, 24), std::exception);
#endif
	}
#endif
	{
		bls::SecretKey sec;
		sec.clear();
		bls::PublicKey pub;
		sec.getPublicKey(pub);
		bls::Signature sig;
		const char msg[] = "abc";
		const size_t msgSize = strlen(msg);
		sec.signHash(sig, msg, msgSize);
		CYBOZU_TEST_ASSERT(!sig.verifyHash(pub, msg, msgSize));
	}
}

void blsTest()
{
	bls::SecretKey sec;
	sec.init();
	streamTest(sec);
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	streamTest(pub);
	for (int i = 0; i < 5; i++) {
		std::string m = "hello";
		m += char('0' + i);
		bls::Signature sig;
		sec.sign(sig, m);
		CYBOZU_TEST_ASSERT(sig.verify(pub, m));
		CYBOZU_TEST_ASSERT(!sig.verify(pub, m + "a"));
		streamTest(sig);
#ifdef NDEBUG
		CYBOZU_BENCH_C("sign", 300, sec.sign, sig, m);
		CYBOZU_BENCH_C("verify", 300, sig.verify, pub, m);
#endif
	}
}

void k_of_nTest()
{
	const std::string m = "abc";
	const int n = 5;
	const int k = 3;
	bls::SecretKey sec0;
	sec0.init();
	bls::Signature sig0;
	sec0.sign(sig0, m);
	bls::PublicKey pub0;
	sec0.getPublicKey(pub0);
	CYBOZU_TEST_ASSERT(sig0.verify(pub0, m));

	bls::SecretKeyVec msk;
	sec0.getMasterSecretKey(msk, k);

	bls::SecretKeyVec allPrvVec(n);
	bls::IdVec allIdVec(n);
	for (int i = 0; i < n; i++) {
		int id = i + 1;
		allPrvVec[i].set(msk, id);
		allIdVec[i] = id;

		bls::SecretKey p;
		p.set(msk.data(), k, id);
		CYBOZU_TEST_EQUAL(allPrvVec[i], p);
	}

	bls::SignatureVec allSigVec(n);
	for (int i = 0; i < n; i++) {
		CYBOZU_TEST_ASSERT(allPrvVec[i] != sec0);
		allPrvVec[i].sign(allSigVec[i], m);
		bls::PublicKey pub;
		allPrvVec[i].getPublicKey(pub);
		CYBOZU_TEST_ASSERT(pub != pub0);
		CYBOZU_TEST_ASSERT(allSigVec[i].verify(pub, m));
	}

	/*
		3-out-of-n
		can recover
	*/
	bls::SecretKeyVec secVec(3);
	bls::IdVec idVec(3);
	for (int a = 0; a < n; a++) {
		secVec[0] = allPrvVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			secVec[1] = allPrvVec[b];
			idVec[1] = allIdVec[b];
			for (int c = b + 1; c < n; c++) {
				secVec[2] = allPrvVec[c];
				idVec[2] = allIdVec[c];
				bls::SecretKey sec;
				sec.recover(secVec, idVec);
				CYBOZU_TEST_EQUAL(sec, sec0);
				bls::SecretKey sec2;
				sec2.recover(secVec.data(), idVec.data(), secVec.size());
				CYBOZU_TEST_EQUAL(sec, sec2);
			}
		}
	}
	{
		secVec[0] = allPrvVec[0];
		secVec[1] = allPrvVec[1];
		secVec[2] = allPrvVec[0]; // same of secVec[0]
		idVec[0] = allIdVec[0];
		idVec[1] = allIdVec[1];
		idVec[2] = allIdVec[0];
		bls::SecretKey sec;
		CYBOZU_TEST_EXCEPTION_MESSAGE(sec.recover(secVec, idVec), std::exception, "same id");
	}
	{
		/*
			n-out-of-n
			can recover
		*/
		bls::SecretKey sec;
		sec.recover(allPrvVec, allIdVec);
		CYBOZU_TEST_EQUAL(sec, sec0);
	}
	/*
		2-out-of-n
		can't recover
	*/
	secVec.resize(2);
	idVec.resize(2);
	for (int a = 0; a < n; a++) {
		secVec[0] = allPrvVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			secVec[1] = allPrvVec[b];
			idVec[1] = allIdVec[b];
			bls::SecretKey sec;
			sec.recover(secVec, idVec);
			CYBOZU_TEST_ASSERT(sec != sec0);
		}
	}
	/*
		3-out-of-n
		can recover
	*/
	bls::SignatureVec sigVec(3);
	idVec.resize(3);
	for (int a = 0; a < n; a++) {
		sigVec[0] = allSigVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			sigVec[1] = allSigVec[b];
			idVec[1] = allIdVec[b];
			for (int c = b + 1; c < n; c++) {
				sigVec[2] = allSigVec[c];
				idVec[2] = allIdVec[c];
				bls::Signature sig;
				sig.recover(sigVec, idVec);
				CYBOZU_TEST_EQUAL(sig, sig0);
			}
		}
	}
	{
		sigVec[0] = allSigVec[1]; idVec[0] = allIdVec[1];
		sigVec[1] = allSigVec[4]; idVec[1] = allIdVec[4];
		sigVec[2] = allSigVec[3]; idVec[2] = allIdVec[3];
#ifdef NDEBUG
		bls::Signature sig;
		CYBOZU_BENCH_C("sig.recover", 100, sig.recover, sigVec, idVec);
#endif
	}
	{
		/*
			n-out-of-n
			can recover
		*/
		bls::Signature sig;
		sig.recover(allSigVec, allIdVec);
		CYBOZU_TEST_EQUAL(sig, sig0);
	}
	/*
		2-out-of-n
		can't recover
	*/
	sigVec.resize(2);
	idVec.resize(2);
	for (int a = 0; a < n; a++) {
		sigVec[0] = allSigVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			sigVec[1] = allSigVec[b];
			idVec[1] = allIdVec[b];
			bls::Signature sig;
			sig.recover(sigVec, idVec);
			CYBOZU_TEST_ASSERT(sig != sig0);
		}
	}
	// return same value if n = 1
	sigVec.resize(1);
	idVec.resize(1);
	sigVec[0] = allSigVec[0];
	idVec[0] = allIdVec[0];
	{
		bls::Signature sig;
		sig.recover(sigVec, idVec);
		CYBOZU_TEST_EQUAL(sig, sigVec[0]);
	}
	// share and recover publicKey
	{
		bls::PublicKeyVec pubVec(k);
		idVec.resize(k);
		// select [0, k) publicKey
		for (int i = 0; i < k; i++) {
			allPrvVec[i].getPublicKey(pubVec[i]);
			idVec[i] = allIdVec[i];
		}
		bls::PublicKey pub;
		pub.recover(pubVec, idVec);
		CYBOZU_TEST_EQUAL(pub, pub0);
		bls::PublicKey pub2;
		pub2.recover(pubVec.data(), idVec.data(), pubVec.size());
		CYBOZU_TEST_EQUAL(pub, pub2);
	}
}

void popTest()
{
	const size_t k = 3;
	const size_t n = 6;
	const std::string m = "pop test";
	bls::SecretKey sec0;
	sec0.init();
	bls::PublicKey pub0;
	sec0.getPublicKey(pub0);
	bls::Signature sig0;
	sec0.sign(sig0, m);
	CYBOZU_TEST_ASSERT(sig0.verify(pub0, m));

	bls::SecretKeyVec msk;
	sec0.getMasterSecretKey(msk, k);

	bls::PublicKeyVec mpk;
	bls::getMasterPublicKey(mpk, msk);
	bls::SignatureVec  popVec;
	bls::getPopVec(popVec, msk);

	for (size_t i = 0; i < popVec.size(); i++) {
		CYBOZU_TEST_ASSERT(popVec[i].verify(mpk[i]));
	}

	const int idTbl[n] = {
		3, 5, 193, 22, 15
	};
	bls::SecretKeyVec secVec(n);
	bls::PublicKeyVec pubVec(n);
	bls::SignatureVec sVec(n);
	for (size_t i = 0; i < n; i++) {
		int id = idTbl[i];
		secVec[i].set(msk, id);
		secVec[i].getPublicKey(pubVec[i]);
		bls::PublicKey pub;
		pub.set(mpk, id);
		CYBOZU_TEST_EQUAL(pubVec[i], pub);

		bls::Signature pop;
		secVec[i].getPop(pop);
		CYBOZU_TEST_ASSERT(pop.verify(pubVec[i]));

		secVec[i].sign(sVec[i], m);
		CYBOZU_TEST_ASSERT(sVec[i].verify(pubVec[i], m));
	}
	secVec.resize(k);
	sVec.resize(k);
	bls::IdVec idVec(k);
	for (size_t i = 0; i < k; i++) {
		idVec[i] = idTbl[i];
	}
	bls::SecretKey sec;
	sec.recover(secVec, idVec);
	CYBOZU_TEST_EQUAL(sec, sec0);
	bls::Signature sig;
	sig.recover(sVec, idVec);
	CYBOZU_TEST_EQUAL(sig, sig0);
	bls::Signature sig2;
	sig2.recover(sVec.data(), idVec.data(), sVec.size());
	CYBOZU_TEST_EQUAL(sig, sig2);
}

void addTest()
{
	bls::SecretKey sec1, sec2;
	sec1.init();
	sec2.init();
	CYBOZU_TEST_ASSERT(sec1 != sec2);

	bls::PublicKey pub1, pub2;
	sec1.getPublicKey(pub1);
	sec2.getPublicKey(pub2);

	const std::string m = "doremi";
	bls::Signature sig1, sig2;
	sec1.sign(sig1, m);
	sec2.sign(sig2, m);
	CYBOZU_TEST_ASSERT((sig1 + sig2).verify(pub1 + pub2, m));
}

void aggregateTest()
{
	const size_t n = 10;
	bls::SecretKey secs[n];
	bls::PublicKey pubs[n], pub;
	bls::Signature sigs[n], sig;
	const std::string m = "abc";
	for (size_t i = 0; i < n; i++) {
		secs[i].init();
		secs[i].getPublicKey(pubs[i]);
		secs[i].sign(sigs[i], m);
	}
	pub = pubs[0];
	sig = sigs[0];
	for (size_t i = 1; i < n; i++) {
		pub.add(pubs[i]);
		sig.add(sigs[i]);
	}
	CYBOZU_TEST_ASSERT(sig.verify(pub, m));
	blsSignature sig2;
	const blsSignature *p = (const blsSignature *)&sigs[0];
	blsAggregateSignature(&sig2, p, n);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig2, (const blsSignature*)&sig));
	memset(&sig, 0, sizeof(sig));
	blsAggregateSignature((blsSignature*)&sig2, 0, 0);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig2, (const blsSignature*)&sig));
}

void dataTest()
{
	const size_t FrSize = bls::getFrByteSize();
	const size_t FpSize = bls::getG1ByteSize();
	bls::SecretKey sec;
	sec.init();
	std::string str;
	sec.getStr(str, bls::IoFixedByteSeq);
	{
		CYBOZU_TEST_EQUAL(str.size(), FrSize);
		bls::SecretKey sec2;
		sec2.setStr(str, bls::IoFixedByteSeq);
		CYBOZU_TEST_EQUAL(sec, sec2);
	}
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	pub.getStr(str, bls::IoFixedByteSeq);
	{
#ifdef BLS_ETH
		CYBOZU_TEST_EQUAL(str.size(), FpSize);
#else
		CYBOZU_TEST_EQUAL(str.size(), FpSize * 2);
#endif
		bls::PublicKey pub2;
		pub2.setStr(str, bls::IoFixedByteSeq);
		CYBOZU_TEST_EQUAL(pub, pub2);
	}
	std::string m = "abc";
	bls::Signature sign;
	sec.sign(sign, m);
	sign.getStr(str, bls::IoFixedByteSeq);
	{
#ifdef BLS_ETH
		CYBOZU_TEST_EQUAL(str.size(), FpSize * 2);
#else
		CYBOZU_TEST_EQUAL(str.size(), FpSize);
#endif
		bls::Signature sign2;
		sign2.setStr(str, bls::IoFixedByteSeq);
		CYBOZU_TEST_EQUAL(sign, sign2);
	}
	bls::Id id;
	const uint64_t v[] = { 1, 2, 3, 4, 5, 6, };
	id.set(v);
	id.getStr(str, bls::IoFixedByteSeq);
	{
		CYBOZU_TEST_EQUAL(str.size(), FrSize);
		bls::Id id2;
		id2.setStr(str, bls::IoFixedByteSeq);
		CYBOZU_TEST_EQUAL(id, id2);
	}
}

void testAggregatedHashes(size_t n)
{
#ifdef BLS_ETH
	const size_t sizeofHash = 48 * 2;
#else
	const size_t sizeofHash = 32;
#endif
	struct Hash { char data[sizeofHash]; };
	bls::PublicKeyVec pubs(n);
	bls::SignatureVec sigs(n);
	std::vector<Hash> h(n);
	for (size_t i = 0; i < n; i++) {
		bls::SecretKey sec;
		sec.init();
#ifdef BLS_ETH
		setFp2Serialize(h[i].data);
#else
		char msg[128];
		CYBOZU_SNPRINTF(msg, sizeof(msg), "abc-%d", (int)i);
		const size_t msgSize = strlen(msg);
		cybozu::Sha256().digest(h[i].data, sizeofHash, msg, msgSize);
#endif
		sec.getPublicKey(pubs[i]);
		sec.signHash(sigs[i], h[i].data, sizeofHash);
	}
	bls::Signature sig = sigs[0];
	for (size_t i = 1; i < n; i++) {
		sig.add(sigs[i]);
	}
	CYBOZU_TEST_ASSERT(sig.verifyAggregatedHashes(pubs.data(), h.data(), sizeofHash, n));
	bls::Signature invalidSig = sigs[0] + sigs[0];
	CYBOZU_TEST_ASSERT(!invalidSig.verifyAggregatedHashes(pubs.data(), h.data(), sizeofHash, n));
#ifndef BLS_ETH
	h[0].data[0]++;
	CYBOZU_TEST_ASSERT(!sig.verifyAggregatedHashes(pubs.data(), h.data(), sizeofHash, n));
#endif
	printf("n=%2d ", (int)n);
#ifdef NDEBUG
	CYBOZU_BENCH_C("aggregate", 50, sig.verifyAggregatedHashes, pubs.data(), h.data(), sizeofHash, n);
#endif
}

void verifyAggregateTest(int type)
{
	(void)type;
#ifdef BLS_ETH
	if (type != MCL_BLS12_381) return;
#endif
	const size_t nTbl[] = { 1, 2, 15, 16, 17, 30, 31, 32, 33, 50 };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(nTbl); i++) {
		testAggregatedHashes(nTbl[i]);
	}
}

unsigned int writeSeq(void *self, void *buf, unsigned int bufSize)
{
	int& seq = *(int*)self;
	char *p = (char *)buf;
	for (unsigned int i = 0; i < bufSize; i++) {
		p[i] = char(seq++);
	}
	return bufSize;
}

void setRandFuncTest(int type)
{
	(void)type;
#ifdef BLS_ETH
	if (type == MCL_BLS12_381) return;
#endif
	blsSecretKey sec;
	const int seqInit1 = 5;
	int seq = seqInit1;
	blsSetRandFunc(&seq, writeSeq);
	blsSecretKeySetByCSPRNG(&sec);
	unsigned char buf[128];
	size_t n = blsSecretKeySerialize(buf, sizeof(buf), &sec);
	CYBOZU_TEST_ASSERT(n > 0);
	for (size_t i = 0; i < n - 1; i++) {
		// ommit buf[n - 1] because it may be masked
		CYBOZU_TEST_EQUAL(buf[i], seqInit1 + i);
	}
	// use default CSPRNG
	blsSetRandFunc(0, 0);
	blsSecretKeySetByCSPRNG(&sec);
	n = blsSecretKeySerialize(buf, sizeof(buf), &sec);
	CYBOZU_TEST_ASSERT(n > 0);
	printf("sec=");
	for (size_t i = 0; i < n; i++) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

#ifdef BLS_ETH
bls::Signature deserializeSignatureFromHexStr(const std::string& sigHex)
{
	bls::Signature sig;
	try {
		sig.deserializeHexStr(sigHex);
		CYBOZU_TEST_EQUAL(blsSignatureIsValidOrder(sig.getPtr()), 1);
	} catch (...) {
		printf("bad signature %s\n", sigHex.c_str());
		sig.clear();
	}
	return sig;
}

void ethSignOneTest(const std::string& secHex, const std::string& msgHex, const std::string& sigHex)
{
	const Uint8Vec msg = fromHexStr(msgHex);
	bls::SecretKey sec;
	sec.setStr(secHex, 16);
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	bls::Signature sig;
	sec.sign(sig, msg.data(), msg.size());
	CYBOZU_TEST_EQUAL(sig.serializeToHexStr(), sigHex);
	CYBOZU_TEST_ASSERT(sig.verify(pub, msg.data(), msg.size()));
}

void ethSignFileTest(const std::string& dir)
{
	std::string fileName = cybozu::GetExePath() + "../test/eth/" + dir + "/sign.txt";
	std::ifstream ifs(fileName.c_str());
	CYBOZU_TEST_ASSERT(ifs);
	for (;;) {
		std::string h1, h2, h3, sec, msg, sig;
		ifs >>  h1 >> sec >> h2 >> msg >> h3 >> sig;
		if (h1.empty()) break;
		if (h1 != "sec" || h2 != "msg" || h3 != "out") {
			throw cybozu::Exception("bad format") << fileName << h1 << h2 << h3;
		}
		ethSignOneTest(sec, msg, sig);
	}
}

void ethVerifyOneTest(const std::string& pubHex, const std::string& msgHex, const std::string& sigHex, const std::string& outStr)
{
	const Uint8Vec msg = fromHexStr(msgHex);
	bls::PublicKey pub;
	pub.deserializeHexStr(pubHex);
	bool expect = outStr == "true";
	bls::Signature sig = deserializeSignatureFromHexStr(sigHex);
	bool b = sig.verify(pub, msg.data(), msg.size());
	CYBOZU_TEST_EQUAL(b, expect);
}

void ethVerifyFileTest(const std::string& dir)
{
	std::string fileName = cybozu::GetExePath() + "../test/eth/" + dir + "/verify.txt";
	std::ifstream ifs(fileName.c_str());
	CYBOZU_TEST_ASSERT(ifs);
	for (;;) {
		std::string h1, h2, h3, h4, pub, msg, sig, out;
		ifs >>  h1 >> pub >> h2 >> msg >> h3 >> sig >> h4 >> out;
		if (h1.empty()) break;
		if (h1 != "pub" || h2 != "msg" || h3 != "sig" || h4 != "out") {
			throw cybozu::Exception("bad format") << fileName << h1 << h2 << h3 << h4;
		}
		ethVerifyOneTest(pub, msg, sig, out);
	}
}


void ethFastAggregateVerifyTest(const std::string& dir)
{
	puts("ethFastAggregateVerifyTest");
	std::string fileName = cybozu::GetExePath() + "../test/eth/" + dir + "/fast_aggregate_verify.txt";
	std::ifstream ifs(fileName.c_str());
	CYBOZU_TEST_ASSERT(ifs);
	int i = 0;
	for (;;) {
		bls::PublicKeyVec pubVec;
		Uint8Vec msg;
		int output;
		std::string h;
		std::string s;
		for (;;) {
			ifs >> h;
			if (h.empty()) return;
			if (h != "pub") break;
			bls::PublicKey pub;
			ifs >> s;
			pub.deserializeHexStr(s);
			pubVec.push_back(pub);
		}
		printf("i=%d\n", i++);
		if (h != "msg") throw cybozu::Exception("bad msg") << h;
		ifs >> s;
		msg = fromHexStr(s);
		ifs >> h;
		if (h != "sig") throw cybozu::Exception("bad sig") << h;
		ifs >> s;
		bls::Signature sig = deserializeSignatureFromHexStr(s);
		ifs >> h;
		if (h != "out") throw cybozu::Exception("bad out") << h;
		ifs >> s;
		if (s == "false") {
			output = 0;
		} else if (s == "true") {
			output = 1;
		} else {
			throw cybozu::Exception("bad out") << s;
		}
		int r = blsFastAggregateVerify(sig.getPtr(), pubVec[0].getPtr(), pubVec.size(), msg.data(), msg.size());
		CYBOZU_TEST_EQUAL(r, output);
	}
}

void blsAggregateVerifyNoCheckTestOne(size_t n, bool hasZero = false)
{
	const size_t msgSize = 32;
	bls::PublicKeyVec pubs(n);
	bls::SignatureVec sigs(n);
	std::string msgs(msgSize * n, 0);
	for (size_t i = 0; i < n; i++) {
		bls::SecretKey sec;
		sec.init();
		if (hasZero && i == n/2) {
			sec.clear();
		}
		sec.getPublicKey(pubs[i]);
		msgs[msgSize * i] = i;
		sec.sign(sigs[i], &msgs[msgSize * i], msgSize);
	}
	blsSignature aggSig;
	blsAggregateSignature(&aggSig, sigs[0].getPtr(), n);
	if (hasZero) {
		CYBOZU_TEST_EQUAL(!blsAggregateVerifyNoCheck(&aggSig, pubs[0].getPtr(), msgs.data(), msgSize, n), 1);
		return;
	}
	CYBOZU_TEST_EQUAL(blsAggregateVerifyNoCheck(&aggSig, pubs[0].getPtr(), msgs.data(), msgSize, n), 1);
#ifdef NDEBUG
	CYBOZU_BENCH_C("blsAggregateVerifyNoCheck", 50, blsAggregateVerifyNoCheck, &aggSig, pubs[0].getPtr(), msgs.data(), msgSize, n);
#endif
	(*(char*)(&aggSig))++;
	CYBOZU_TEST_EQUAL(blsAggregateVerifyNoCheck(&aggSig, pubs[0].getPtr(), msgs.data(), msgSize, n), 0);
}

void blsAggregateVerifyNoCheckTest()
{
	const size_t nTbl[] = { 1, 2, 15, 16, 17, 30, 31, 32, 33, 50 };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(nTbl); i++) {
		blsAggregateVerifyNoCheckTestOne(nTbl[i]);
	}
	blsAggregateVerifyNoCheckTestOne(10, true);
}

void draft07Test()
{
	blsSetETHmode(BLS_ETH_MODE_DRAFT_07);
	blsSecretKey sec;
	blsSecretKeySetHexStr(&sec, "1", 1);
	blsSignature sig;
	const char *msg = "asdf";
	const char *tbl[] = {
		"2525875563870715639912451285996878827057943937903727288399283574780255586622124951113038778168766058972461529282986",
		"3132482115871619853374334004070359337604487429071253737901486558733107203612153024147084489564256619439711974285977",
		"2106640002084734620850657217129389007976098691731730501862206029008913488613958311385644530040820978748080676977912",
		"2882649322619140307052211460282445786973517746532934590265600680988689024512167659295505342688129634612479405019290",
	};
	blsSign(&sig, &sec, msg, strlen(msg));
	mclBnG2_normalize(&sig.v, &sig.v);
	{
		const bls::Signature& b = *(const bls::Signature*)(&sig);
		printf("draft07-sig(%s) by sec=1 =%s\n", msg, b.serializeToHexStr().c_str());
	}
	const mclBnFp *p = &sig.v.x.d[0];
	for (int i = 0; i < 4; i++) {
		char buf[128];
		mclBnFp_getStr(buf, sizeof(buf), &p[i], 10);
		CYBOZU_TEST_EQUAL(buf, tbl[i]);
	}
}

void ethAggregateVerifyOneTest(const StringVec& pubHexVec, const StringVec& msgHexVec, const std::string& sigHex, bool out)
{
	const size_t n = pubHexVec.size();
	if (n != msgHexVec.size()) {
		throw cybozu::Exception("bad size");
	}
	bls::PublicKeyVec pubs;
	Uint8Vec msgVec;

	size_t msgSize = 0;
	for (size_t i = 0; i < n; i++) {
		bls::PublicKey pub;
		pub.deserializeHexStr(pubHexVec[i]);
		pubs.push_back(pub);
		Uint8Vec t = fromHexStr(msgHexVec[i]);
		if (i == 0) msgSize = t.size();
		msgVec.insert(msgVec.end(), t.begin(), t.end());
	}
	bls::Signature sig = deserializeSignatureFromHexStr(sigHex);
	CYBOZU_TEST_EQUAL(blsAggregateVerifyNoCheck(sig.getPtr(), pubs[0].getPtr(), msgVec.data(), msgSize, n), out);
}

void ethAggregateVerifyTest(const std::string& dir)
{
	puts("ethAggregateVerifyTest");
	// exceptional test
	const char *sigTbl[] = {
		"c00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
		"000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(sigTbl); i++) {
		StringVec pubHexVec;
		StringVec msgHexVec;
		ethAggregateVerifyOneTest(pubHexVec, msgHexVec, sigTbl[i], false);
	}
	// tests from file
	std::string fileName = cybozu::GetExePath() + "../test/eth/" + dir + "/aggregate_verify.txt";
	std::ifstream ifs(fileName.c_str());
	CYBOZU_TEST_ASSERT(ifs);
	for (;;) {
		StringVec pubHexVec;
		StringVec msgHexVec;
		std::string sigHex;
		bool out;
		std::string h;
		std::string s;
		for (;;) {
			ifs >> h;
			if (h.empty()) return;
			if (h != "pub") break;
			ifs >> s;
			pubHexVec.push_back(s);
		}
		for (;;) {
			if (h != "msg") break;
			ifs >> s;
			msgHexVec.push_back(s);
			ifs >> h;
		}
		if (h != "sig") throw cybozu::Exception("bad sig") << h;
		ifs >> sigHex;
		ifs >> h;
		if (h != "out") throw cybozu::Exception("bad out") << h;
		ifs >> s;
		if (s == "false") {
			out = false;
		} else if (s == "true") {
			out = true;
		} else {
			throw cybozu::Exception("bad out") << s;
		}
		ethAggregateVerifyOneTest(pubHexVec, msgHexVec, sigHex, out);
	}
}

void ethAggregateTest(const std::string& dir)
{
	puts("ethAggregateTest");
	std::string fileName = cybozu::GetExePath() + "../test/eth/" + dir + "/aggregate.txt";
	std::ifstream ifs(fileName.c_str());
	CYBOZU_TEST_ASSERT(ifs);
	for (;;) {
		bls::SignatureVec sigs;
		std::string h;
		std::string s;
		for (;;) {
			ifs >> h;
			if (h.empty()) return;
			if (h != "sig") break;
			ifs >> s;
			sigs.push_back(deserializeSignatureFromHexStr(s));
		}
		if (h != "out") throw cybozu::Exception("bad out") << h;
		ifs >> s;
		bls::Signature expect = deserializeSignatureFromHexStr(s);
		blsSignature agg;
		blsAggregateSignature(&agg, sigs[0].getPtr(), sigs.size());
		CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&agg, expect.getPtr()));
	}
}

void ethMultiVerifyTestOne(size_t n)
{
	printf("n=%zd\n", n);
	const size_t msgSize = 32;
	cybozu::XorShift rg;
	bls::PublicKeyVec pubs(n);
	bls::SignatureVec sigs(n);
	std::string msgs(msgSize * n, 0);
	const size_t randSize = 8;
	std::vector<uint8_t> rands(randSize * n);

	rg.read(&rands[0], rands.size());
	rg.read(&msgs[0], msgs.size());
	for (size_t i = 0; i < n; i++) {
		bls::SecretKey sec;
		sec.init();
		sec.sign(sigs[i], &msgs[i * msgSize], msgSize);
		sec.getPublicKey(pubs[i]);
	}
#ifndef DISABLE_THREAD_TEST
	for (int threadN = 1; threadN < 32; threadN += 4) {
		printf("threadN=%d\n", threadN);
		CYBOZU_TEST_EQUAL(blsMultiVerify(sigs[0].getPtr(), pubs[0].getPtr(), msgs.data(), msgSize, rands.data(), randSize, n, threadN), 1);
#ifdef NDEBUG
		CYBOZU_BENCH_C("multiVerify", 10, blsMultiVerify, sigs[0].getPtr(), pubs[0].getPtr(), msgs.data(), msgSize, rands.data(), randSize, n, threadN);
#endif
	}
	msgs[msgs.size() - 1]--;
	CYBOZU_TEST_EQUAL(blsMultiVerify(sigs[0].getPtr(), pubs[0].getPtr(), msgs.data(), msgSize, rands.data(), randSize, n, 0), 0);
#endif
}

void ethMultiVerifyZeroTest()
{
	const int n = 50;
	const int zeroPos = 41;
	const size_t msgSize = 32;
	cybozu::XorShift rg;
	bls::PublicKeyVec pubs(n);
	bls::SignatureVec sigs(n);
	std::string msgs(msgSize * n, 0);
	const size_t randSize = 8;
	std::vector<uint8_t> rands(randSize * n);

	rg.read(&rands[0], rands.size());
	rg.read(&msgs[0], msgs.size());
	for (size_t i = 0; i < n; i++) {
		bls::SecretKey sec;
		if (i == zeroPos) {
			sec.clear();
		} else {
			sec.init();
		}
		sec.sign(sigs[i], &msgs[i * msgSize], msgSize);
		sec.getPublicKey(pubs[i]);
	}
#ifndef DISABLE_THREAD_TEST
	CYBOZU_TEST_EQUAL(blsMultiVerify(sigs[0].getPtr(), pubs[0].getPtr(), msgs.data(), msgSize, rands.data(), randSize, n, 2), 0);
#endif
}

void ethMultiVerifyTest()
{
	puts("ethMultiVerifyTest");
	const size_t nTbl[] = { 1, 2, 15, 16, 17, 30, 31, 32, 33, 50, 400 };
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(nTbl); i++) {
		ethMultiVerifyTestOne(nTbl[i]);
	}
}

void makePublicKeyVec(blsSignature *aggSig, blsPublicKey *pubVec, size_t n, int mode, const char *msg, size_t msgSize)
{
	blsPublicKey pub;
	memset(&pub, 0, sizeof(pub));
	memset(aggSig, 0, sizeof(blsSignature));
	if (mode == 0) {
		// n/2 is zero public key
		for (size_t i = 0; i < n; i++) {
			blsSecretKey sec;
			if (i == n/2) {
				memset(&sec, 0, sizeof(sec));
			} else {
				blsSecretKeySetByCSPRNG(&sec);
			}
			blsGetPublicKey(&pubVec[i], &sec);
			blsSignature sig;
			blsSign(&sig, &sec, msg, msgSize);
			blsSignatureAdd(aggSig, &sig);
		}
	} else {
		// sum of public key is zero
		for (size_t i = 0; i < n - 1; i++) {
			blsSecretKey sec;
			blsSecretKeySetByCSPRNG(&sec);
			blsGetPublicKey(&pubVec[i], &sec);
			blsPublicKeyAdd(&pub, &pubVec[i]);
		}
		mclBnG1_neg(&pubVec[n-1].v, &pub.v);
	}
}

void ethZeroTest()
{
	{
		bls::SecretKey sec;
		sec.clear();
		bls::PublicKey pub;
		sec.getPublicKey(pub);
		std::string m = "abc";
		bls::Signature sig;
		sec.sign(sig, m);
		CYBOZU_TEST_ASSERT(!sig.verify(pub, m));
	}
	{
		const size_t n = 8;
		const char *msg = "abc";
		const size_t msgSize = strlen(msg);
		blsPublicKey pubVec[n];
		blsSignature aggSig;
		// n/2 is zero public key
		makePublicKeyVec(&aggSig, pubVec, n, 0, msg, msgSize);
		CYBOZU_TEST_EQUAL(blsFastAggregateVerify(&aggSig, pubVec, n, msg, msgSize), 0);
		// sum of public key is zero
		makePublicKeyVec(&aggSig, pubVec, n, 1, msg, msgSize);
		CYBOZU_TEST_EQUAL(blsFastAggregateVerify(&aggSig, pubVec, n, msg, msgSize), 0);
	}
	ethMultiVerifyZeroTest();
}

void ethTest(int type)
{
	if (type != MCL_BLS12_381) return;
	ethZeroTest();
	ethMultiVerifyTest();
	blsAggregateVerifyNoCheckTest();
	draft07Test();
	ethSignFileTest("draft07");
	ethFastAggregateVerifyTest("draft07");
	ethVerifyFileTest("draft07");
	ethAggregateVerifyTest("draft07");
	ethAggregateTest("draft07");
}
#endif
void generatorTest()
{
	puts("generatorTest");
	blsPublicKey gen, save;
#ifdef BLS_ETH
	mclBnG1_hashAndMapTo(&gen.v, "abc", 3);
#else
	mclBnG2_hashAndMapTo(&gen.v, "abc", 3);
#endif
	blsGetGeneratorOfPublicKey(&save);
	blsSetGeneratorOfPublicKey(&gen);
	blsSecretKey sec;
	mclBnFr_setInt32(&sec.v, 1);
	blsPublicKey pub;
	blsGetPublicKey(&pub, &sec);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub, &gen));
	blsTest();
	blsSetGeneratorOfPublicKey(&save);
}

void testAll(int type)
{
#if 1
	blsTest();
	k_of_nTest();
	popTest();
	addTest();
	dataTest();
	aggregateTest();
	verifyAggregateTest(type);
	setRandFuncTest(type);
	hashTest(type);
	generatorTest();
#endif
#ifdef BLS_ETH
	ethTest(type);
#endif
}
CYBOZU_TEST_AUTO(all)
{
	const struct {
		int type;
		const char *name;
	} tbl[] = {
		{ MCL_BN254, "BN254" },
#if MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 6
		{ MCL_BN381_1, "BN381_1" },
#endif
#if MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 4
		{ MCL_BLS12_381, "BLS12_381" },
#endif
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		int type = tbl[i].type;
#ifdef MCL_STATIC_CODE
		if (type != MCL_BLS12_381) continue;
#endif
		printf("curve=%s\n", tbl[i].name);
		bls::init(type);
		if (type == MCL_BN254) {
			testForBN254();
		}
		testAll(type);
	}
}
