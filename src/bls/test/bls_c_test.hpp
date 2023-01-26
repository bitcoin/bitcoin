#include <cybozu/test.hpp>
#include <cybozu/inttype.hpp>
#include <bls/bls.h>
#include <string.h>
#include <cybozu/benchmark.hpp>
#include <mcl/gmp_util.hpp>

size_t pubSize(size_t FrSize)
{
#ifdef BLS_ETH
	return FrSize;
#else
	return FrSize * 2;
#endif
}
size_t sigSize(size_t FrSize)
{
#ifdef BLS_ETH
	return FrSize * 2;
#else
	return FrSize;
#endif
}

void bls_use_stackTest()
{
	blsSecretKey sec;
	blsPublicKey pub;
	blsSignature sig;
	const char *msg = "this is a pen";
	const size_t msgSize = strlen(msg);

	blsSecretKeySetByCSPRNG(&sec);

	blsGetPublicKey(&pub, &sec);

	blsSign(&sig, &sec, msg, msgSize);

	CYBOZU_TEST_ASSERT(blsVerify(&sig, &pub, msg, msgSize));

	CYBOZU_TEST_ASSERT(!blsSecretKeyIsZero(&sec));
	CYBOZU_TEST_ASSERT(!blsPublicKeyIsZero(&pub));
	CYBOZU_TEST_ASSERT(!blsSignatureIsZero(&sig));
	memset(&sec, 0, sizeof(sec));
	memset(&pub, 0, sizeof(pub));
	memset(&sig, 0, sizeof(sig));
	CYBOZU_TEST_ASSERT(blsSecretKeyIsZero(&sec));
	CYBOZU_TEST_ASSERT(blsPublicKeyIsZero(&pub));
	CYBOZU_TEST_ASSERT(blsSignatureIsZero(&sig));
}

void blsDataTest()
{
	const char *msg = "test test";
	const size_t msgSize = strlen(msg);
	const size_t FrSize = blsGetFrByteSize();
	const size_t FpSize = blsGetG1ByteSize();
	blsSecretKey sec1, sec2;
	blsSecretKeySetByCSPRNG(&sec1);
	char buf[1024];
	size_t n;
	size_t ret;
	n = blsSecretKeyGetHexStr(buf, sizeof(buf), &sec1);
	CYBOZU_TEST_ASSERT(0 < n && n <= FrSize * 2);
	ret = blsSecretKeySetHexStr(&sec2, buf, n);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));

	memset(&sec2, 0, sizeof(sec2));
	n = blsSecretKeySerialize(buf, sizeof(buf), &sec1);
	CYBOZU_TEST_EQUAL(n, FrSize);
	CYBOZU_TEST_EQUAL(n, blsGetSerializedSecretKeyByteSize());
	ret = blsSecretKeyDeserialize(&sec2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));

	blsPublicKey pub1, pub2;
	blsGetPublicKey(&pub1, &sec1);
	n = blsPublicKeySerialize(buf, sizeof(buf), &pub1);
	CYBOZU_TEST_EQUAL(n, pubSize(FpSize));
	CYBOZU_TEST_EQUAL(n, blsGetSerializedPublicKeyByteSize());
	ret = blsPublicKeyDeserialize(&pub2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub1, &pub2));
	blsSignature sig1, sig2;
	blsSign(&sig1, &sec1, msg, msgSize);
	n = blsSignatureSerialize(buf, sizeof(buf), &sig1);
	CYBOZU_TEST_EQUAL(n, sigSize(FpSize));
	CYBOZU_TEST_EQUAL(n, blsGetSerializedSignatureByteSize());
	ret = blsSignatureDeserialize(&sig2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig1, &sig2));
}

void blsOrderTest(const char *curveOrder/*Fr*/, const char *fieldOrder/*Fp*/)
{
	char buf[1024];
	size_t len;
	len = blsGetCurveOrder(buf, sizeof(buf));
	CYBOZU_TEST_ASSERT(len > 0);
	CYBOZU_TEST_EQUAL(buf, curveOrder);
	len = blsGetFieldOrder(buf, sizeof(buf));
	CYBOZU_TEST_ASSERT(len > 0);
	CYBOZU_TEST_EQUAL(buf, fieldOrder);
}

void blsSerializeTest()
{
	const size_t FrSize = blsGetFrByteSize();
	const size_t FpSize = blsGetG1ByteSize();
	printf("FrSize=%d, FpSize=%d\n", (int)FrSize, (int)FpSize);
	blsId id1, id2;
	blsSecretKey sec1, sec2;
	blsPublicKey pub1, pub2;
	blsSignature sig1, sig2;
	char buf[1024];
	size_t n;
	size_t expectSize;
	size_t ret;
	const char dummyChar = '1';

	// Id
	expectSize = FrSize;
	blsIdSetInt(&id1, -1);
	n = blsIdSerialize(buf, sizeof(buf), &id1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = blsIdDeserialize(&id2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsIdIsEqual(&id1, &id2));

	ret = blsIdDeserialize(&id2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&id2, 0, sizeof(id2));
	buf[n] = dummyChar;
	ret = blsIdDeserialize(&id2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsIdIsEqual(&id1, &id2));

	n = blsIdSerialize(buf, expectSize, &id1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// SecretKey
	expectSize = FrSize;
	blsSecretKeySetDecStr(&sec1, "-1", 2);
	n = blsSecretKeySerialize(buf, sizeof(buf), &sec1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	ret = blsSecretKeyDeserialize(&sec2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));

	ret = blsSecretKeyDeserialize(&sec2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&sec2, 0, sizeof(sec2));
	buf[n] = dummyChar;
	ret = blsSecretKeyDeserialize(&sec2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));

	n = blsSecretKeySerialize(buf, expectSize, &sec1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// PublicKey
	expectSize = pubSize(FpSize);
	blsGetPublicKey(&pub1, &sec1);
	n = blsPublicKeySerialize(buf, sizeof(buf), &pub1);
	CYBOZU_TEST_EQUAL(n, expectSize);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsValidOrder(&pub1));

	ret = blsPublicKeyDeserialize(&pub2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub1, &pub2));

	ret = blsPublicKeyDeserialize(&pub2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&pub2, 0, sizeof(pub2));
	buf[n] = dummyChar;
	ret = blsPublicKeyDeserialize(&pub2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub1, &pub2));

	n = blsPublicKeySerialize(buf, expectSize, &pub1);
	CYBOZU_TEST_EQUAL(n, expectSize);

	// Signature
#ifdef BLS_ETH
	expectSize = FpSize * 2;
#else
	expectSize = FpSize;
#endif
	blsSign(&sig1, &sec1, "abc", 3);
	n = blsSignatureSerialize(buf, sizeof(buf), &sig1);
	CYBOZU_TEST_EQUAL(n, expectSize);
	CYBOZU_TEST_ASSERT(blsSignatureIsValidOrder(&sig1));

	ret = blsSignatureDeserialize(&sig2, buf, n);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig1, &sig2));

	ret = blsSignatureDeserialize(&sig2, buf, n - 1);
	CYBOZU_TEST_EQUAL(ret, 0);

	memset(&sig2, 0, sizeof(sig2));
	buf[n] = dummyChar;
	ret = blsSignatureDeserialize(&sig2, buf, n + 1);
	CYBOZU_TEST_EQUAL(ret, n);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig1, &sig2));

	n = blsSignatureSerialize(buf, expectSize, &sig1);
	CYBOZU_TEST_EQUAL(n, expectSize);
}

void blsVerifyOrderTest()
{
	puts("blsVerifyOrderTest");
#ifdef BLS_ETH
	const uint8_t Qs[] =
#else
	const uint8_t Ps[] =
#endif
	{
0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	};
#ifdef BLS_ETH
	const uint8_t Ps[] =
#else
	const uint8_t Qs[] =
#endif
	{
0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	};
	size_t n;
	blsPublicKey pub;
	blsPublicKeyVerifyOrder(1);
	n = blsPublicKeyDeserialize(&pub, Ps, sizeof(Ps));
	CYBOZU_TEST_EQUAL(n, 0);
	blsPublicKeyVerifyOrder(0);
	n = blsPublicKeyDeserialize(&pub, Ps, sizeof(Ps));
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_ASSERT(!blsPublicKeyIsValidOrder(&pub));
	blsPublicKeyVerifyOrder(1);

	blsSignature sig;
	blsSignatureVerifyOrder(1);
	n = blsSignatureDeserialize(&sig, Qs, sizeof(Qs));
	CYBOZU_TEST_EQUAL(n, 0);
	blsSignatureVerifyOrder(0);
	n = blsSignatureDeserialize(&sig, Qs, sizeof(Qs));
	CYBOZU_TEST_ASSERT(n > 0);
	CYBOZU_TEST_ASSERT(!blsSignatureIsValidOrder(&sig));
	blsSignatureVerifyOrder(1);
}

void blsAddSubTest()
{
	blsSecretKey sec[3];
	blsPublicKey pub[3];
	blsSignature sig[3];
	const char *msg = "this is a pen";
	const size_t msgSize = strlen(msg);

	const char *secHexStr[8] = { "12", "34" };
	for (int i = 0; i < 2; i++) {
		blsSecretKeySetHexStr(&sec[i], secHexStr[i], strlen(secHexStr[i]));
		blsGetPublicKey(&pub[i], &sec[i]);
		blsSign(&sig[i], &sec[i], msg, msgSize);
	}
	sec[2] = sec[0];
	blsSecretKeyAdd(&sec[2], &sec[1]);
	char buf[1024];
	size_t n = blsSecretKeyGetHexStr(buf, sizeof(buf), &sec[2]);
	CYBOZU_TEST_EQUAL(n, 2);
	CYBOZU_TEST_EQUAL(buf, "46"); // "12" + "34"

	pub[2] = pub[0];
	blsPublicKeyAdd(&pub[2], &pub[1]);
	sig[2] = sig[0];
	blsSignatureAdd(&sig[2], &sig[1]); // sig[2] = sig[0] + sig[1]
	blsSignature sig2;
	blsSign(&sig2, &sec[2], msg, msgSize); // sig2 = signature by sec[2]
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig2, &sig[2]));
	CYBOZU_TEST_ASSERT(blsVerify(&sig[2], &pub[2], msg, msgSize)); // verify by pub[2]

	blsSecretKeySub(&sec[2], &sec[1]);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec[2], &sec[0]));
	blsPublicKeySub(&pub[2], &pub[1]);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub[2], &pub[0]));
	blsSignatureSub(&sig[2], &sig[1]);
	CYBOZU_TEST_ASSERT(blsSignatureIsEqual(&sig[2], &sig[0]));
}

void blsTrivialShareTest()
{
	blsSecretKey sec1, sec2;
	blsPublicKey pub1, pub2;
	blsId id;
	blsIdSetInt(&id, 123);

	blsSecretKeySetByCSPRNG(&sec1);
	blsGetPublicKey(&pub1, &sec1);
	int ret;

	memset(&sec2, 0, sizeof(sec2));
	ret = blsSecretKeyShare(&sec2, &sec1, 1, &id);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));
	memset(&sec2, 0, sizeof(sec2));
	ret = blsSecretKeyRecover(&sec2, &sec1, &id, 1);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(blsSecretKeyIsEqual(&sec1, &sec2));

	memset(&pub2, 0, sizeof(pub2));
	ret = blsPublicKeyShare(&pub2, &pub1, 1, &id);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub1, &pub2));
	memset(&pub2, 0, sizeof(pub2));
	ret = blsPublicKeyRecover(&pub2, &pub1, &id, 1);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_ASSERT(blsPublicKeyIsEqual(&pub1, &pub2));
}

void modTest(const char *rStr)
{
	unsigned char buf[1024] = {};
	int ret;
	blsSecretKey sec;
	const size_t maxByte = 64; // 512-bit
	memset(buf, 0xff, maxByte);
	ret = blsSecretKeySetLittleEndianMod(&sec, buf, maxByte);
	CYBOZU_TEST_EQUAL(ret, 0);
	const mpz_class x = (mpz_class(1) << (maxByte * 8)) - 1; // 512-bit 0xff....ff
	const mpz_class r(rStr);
	size_t n = blsSecretKeySerialize(buf, sizeof(buf), &sec);
	CYBOZU_TEST_ASSERT(n > 0);
#ifndef BLS_ETH
	// serialized data to mpz_class
	mpz_class y = 0;
	for (size_t i = 0; i < n; i++) {
		y <<= 8;
		y += buf[n - 1 - i];
	}
	CYBOZU_TEST_EQUAL(y, x % r);
#endif
}

void blsBench()
{
#ifndef NDEBUG
	puts("skip blsBench");
	return;
#endif
	blsSecretKey sec;
	blsPublicKey pub;
	blsSignature sig;
	const char *msg = "this is a pen";
	const size_t msgSize = strlen(msg);

	blsSecretKeySetByCSPRNG(&sec);

	blsGetPublicKey(&pub, &sec);

	CYBOZU_BENCH_C("sign", 300, blsSign, &sig, &sec, msg, msgSize);
	CYBOZU_BENCH_C("verify", 300, blsVerify, &sig, &pub, msg, msgSize);
}

void blsMultiAggregateTest()
{
	const size_t N = 40;
	const size_t nTbl[] = { 0, 1, 2, 16, 17, N };
	blsPublicKey pubVec[N];
	blsSignature sigVec[N];
	const char *msg = "abcdefg";
	const size_t msgSize = strlen(msg);
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(nTbl); i++) {
		const size_t n = nTbl[i];
		for (size_t j = 0; j < n; j++) {
			blsSecretKey sec;
			blsSecretKeySetByCSPRNG(&sec);
			blsGetPublicKey(&pubVec[j], &sec);
			blsSign(&sigVec[j], &sec, msg, msgSize);
		}
		blsPublicKey aggPub;
		blsSignature aggSig;
		memset(&aggPub, -1, sizeof(aggPub));
		memset(&aggSig, -1, sizeof(aggSig));
		blsMultiAggregatePublicKey(&aggPub, pubVec, n);
		blsMultiAggregateSignature(&aggSig, sigVec, pubVec, n);
		if (n == 0) {
			CYBOZU_TEST_ASSERT(blsPublicKeyIsZero(&aggPub));
			CYBOZU_TEST_ASSERT(blsSignatureIsZero(&aggSig));
			CYBOZU_TEST_ASSERT(!blsVerify(&aggSig, &aggPub, msg, msgSize));
		} else {
			CYBOZU_TEST_ASSERT(blsVerify(&aggSig, &aggPub, msg, msgSize));
		}
	}
}

CYBOZU_TEST_AUTO(all)
{
	const struct {
		int curveType;
		const char *r;
		const char *p;
	} tbl[] = {
		{
			MCL_BN254,
			"16798108731015832284940804142231733909759579603404752749028378864165570215949",
			"16798108731015832284940804142231733909889187121439069848933715426072753864723",
		},
#if MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 6
		{
			MCL_BN381_1,
			"5540996953667913971058039301942914304734176495422447785042938606876043190415948413757785063597439175372845535461389",
			"5540996953667913971058039301942914304734176495422447785045292539108217242186829586959562222833658991069414454984723",
		},
#endif
#if MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE >= 4
		{
			MCL_BLS12_381,
			"52435875175126190479447740508185965837690552500527637822603658699938581184513",
			"4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787",
		},
#endif
#if MCLBN_FP_UNIT_SIZE == 8
		{
			MCL_BN462,
			"6701817056313037086248947066310538444882082605308124576230408038843354961099564416871567745979441241809893679037520753402159179772451651597",
			"6701817056313037086248947066310538444882082605308124576230408038843357549886356779857393369967010764802541005796711440355753503701056323603",
		},
#endif
	};
	for (size_t i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
		printf("i=%d\n", (int)i);
		int ret = blsInit(tbl[i].curveType, MCLBN_COMPILED_TIME_VAR);
		CYBOZU_TEST_EQUAL(ret, 0);
		if (ret) {
			printf("ERR %d\n", ret);
			exit(1);
		}
		blsMultiAggregateTest();
		bls_use_stackTest();
		blsDataTest();
		blsOrderTest(tbl[i].r, tbl[i].p);
		blsSerializeTest();
#ifndef BLS_ETH
		if (tbl[i].curveType == MCL_BLS12_381) blsVerifyOrderTest();
#endif
		blsAddSubTest();
		blsTrivialShareTest();
		modTest(tbl[i].r);
		blsBench();
	}
}
