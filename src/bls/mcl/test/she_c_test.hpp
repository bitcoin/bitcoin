#include <mcl/she.h>
#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include <cybozu/option.hpp>
#include <fstream>

const size_t hashSize = 1 << 10;
const size_t tryNum = 1024;

CYBOZU_TEST_AUTO(init)
{
#if MCLBN_FP_UNIT_SIZE == 4
	int curve = MCL_BN254;
#elif MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 4
	int curve = MCL_BLS12_381;
#elif MCLBN_FP_UNIT_SIZE == 6 && MCLBN_FR_UNIT_SIZE == 6
	int curve = MCL_BN381_1;
#elif MCLBN_FP_UNIT_SIZE == 8
	int curve = MCL_BN462;
#endif
	int ret;
	printf("curve=%d\n", curve);
	ret = sheInit(curve, MCLBN_COMPILED_TIME_VAR);
	CYBOZU_TEST_EQUAL(ret, 0);
	ret = sheSetRangeForDLP(hashSize);
	CYBOZU_TEST_EQUAL(ret, 0);
	sheSetTryNum(tryNum);
}

CYBOZU_TEST_AUTO(encDec)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	int64_t m = 123;
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheCipherTextGT ct;
	sheEncG1(&c1, &pub, m);
	sheEncG2(&c2, &pub, m);
	sheEncGT(&ct, &pub, m);

	int64_t dec;
	CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c1), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecG1ViaGT(&dec, &sec, &c1), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c2), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecG2ViaGT(&dec, &sec, &c2), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, m);

	for (int m = -30; m < 30; m++) {
		dec = 0;
		sheEncG1(&c1, &pub, m);
		CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c1), 0);
		CYBOZU_TEST_EQUAL(dec, m);
		CYBOZU_TEST_EQUAL(sheIsZeroG1(&sec, &c1), m == 0);
		dec = 0;
		sheEncG2(&c2, &pub, m);
		CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c2), 0);
		CYBOZU_TEST_EQUAL(dec, m);
		CYBOZU_TEST_EQUAL(sheIsZeroG2(&sec, &c2), m == 0);
		dec = 0;
		sheEncGT(&ct, &pub, m);
		CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
		CYBOZU_TEST_EQUAL(dec, m);
		CYBOZU_TEST_EQUAL(sheIsZeroGT(&sec, &ct), m == 0);
	}
	for (int m = -30; m < 30; m++) {
		dec = 0;
		sheEncG1(&c1, &pub, 1);
		sheMulG1(&c1, &c1, m);
		CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c1), 0);
		CYBOZU_TEST_EQUAL(dec, m);
		dec = 0;
		sheEncG2(&c2, &pub, 1);
		sheMulG2(&c2, &c2, m);
		CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c2), 0);
		CYBOZU_TEST_EQUAL(dec, m);
		dec = 0;
		sheEncGT(&ct, &pub, 1);
		sheMulGT(&ct, &ct, m);
		CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
		CYBOZU_TEST_EQUAL(dec, m);
	}
}

CYBOZU_TEST_AUTO(addMul)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	int64_t m1 = 12;
	int64_t m2 = -9;
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheCipherTextGT ct;
	sheEncG1(&c1, &pub, m1);
	sheEncG2(&c2, &pub, m2);
	sheMul(&ct, &c1, &c2);

	int64_t dec;
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, m1 * m2);
}

CYBOZU_TEST_AUTO(allOp)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	int64_t m1 = 12;
	int64_t m2 = -9;
	int64_t m3 = 12;
	int64_t m4 = -9;
	int64_t dec;
	sheCipherTextG1 c11, c12;
	sheCipherTextG2 c21, c22;
	sheCipherTextGT ct;

	sheEncG1(&c11, &pub, m1);
	sheNegG1(&c12, &c11);
	CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c12), 0);
	CYBOZU_TEST_EQUAL(dec, -m1);

	sheEncG1(&c12, &pub, m2);
	sheSubG1(&c11, &c11, &c12); // m1 - m2
	sheMulG1(&c11, &c11, 4); // 4 * (m1 - m2)

	sheEncG2(&c21, &pub, m3);
	sheNegG2(&c22, &c21);
	CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c22), 0);
	CYBOZU_TEST_EQUAL(dec, -m3);
	sheEncG2(&c22, &pub, m4);
	sheSubG2(&c21, &c21, &c22); // m3 - m4
	sheMulG2(&c21, &c21, -5); // -5 * (m3 - m4)
	sheMul(&ct, &c11, &c21); // -20 * (m1 - m2) * (m3 - m4)
	sheAddGT(&ct, &ct, &ct); // -40 * (m1 - m2) * (m3 - m4)
	sheMulGT(&ct, &ct, -4); // 160 * (m1 - m2) * (m3 - m4)

	int64_t t = 160 * (m1 - m2) * (m3 - m4);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, t);

	sheCipherTextGT ct2;
	sheNegGT(&ct2, &ct);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct2), 0);
	CYBOZU_TEST_EQUAL(dec, -t);
}

CYBOZU_TEST_AUTO(rerand)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	int64_t m1 = 12;
	int64_t m2 = -9;
	int64_t m3 = 12;
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheCipherTextGT ct1, ct2;
	sheEncG1(&c1, &pub, m1);
	sheReRandG1(&c1, &pub);

	sheEncG2(&c2, &pub, m2);
	sheReRandG2(&c2, &pub);

	sheEncGT(&ct1, &pub, m3);
	sheReRandGT(&ct1, &pub);

	sheMul(&ct2, &c1, &c2);
	sheReRandGT(&ct2, &pub);
	sheAddGT(&ct1, &ct1, &ct2);

	int64_t dec;
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct1), 0);
	CYBOZU_TEST_EQUAL(dec, m1 * m2 + m3);
}

CYBOZU_TEST_AUTO(serialize)
{
	sheSecretKey sec1, sec2;
	sheSecretKeySetByCSPRNG(&sec1);
	shePublicKey pub1, pub2;
	sheGetPublicKey(&pub1, &sec1);

	char buf1[4096], buf2[4096];
	size_t n1, n2;
	size_t r, size;
	const size_t sizeofFr = mclBn_getFrByteSize();
	const size_t sizeofFp = mclBn_getG1ByteSize();

	size = sizeofFr * 2;
	n1 = sheSecretKeySerialize(buf1, sizeof(buf1), &sec1);
	CYBOZU_TEST_EQUAL(n1, size);
	r = sheSecretKeyDeserialize(&sec2, buf1, n1);
	CYBOZU_TEST_EQUAL(r, n1);
	n2 = sheSecretKeySerialize(buf2, sizeof(buf2), &sec2);
	CYBOZU_TEST_EQUAL(n2, size);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n2);

	size = sizeofFp * 3;
	n1 = shePublicKeySerialize(buf1, sizeof(buf1), &pub1);
	CYBOZU_TEST_EQUAL(n1, size);
	r = shePublicKeyDeserialize(&pub2, buf1, n1);
	CYBOZU_TEST_EQUAL(r, n1);
	n2 = shePublicKeySerialize(buf2, sizeof(buf2), &pub2);
	CYBOZU_TEST_EQUAL(n2, size);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n2);

	int m = 123;
	sheCipherTextG1 c11, c12;
	sheCipherTextG2 c21, c22;
	sheCipherTextGT ct1, ct2;
	sheEncG1(&c11, &pub2, m);
	sheEncG2(&c21, &pub2, m);
	sheEncGT(&ct1, &pub2, m);

	size = sizeofFp * 2;
	n1 = sheCipherTextG1Serialize(buf1, sizeof(buf1), &c11);
	CYBOZU_TEST_EQUAL(n1, size);
	r = sheCipherTextG1Deserialize(&c12, buf1, n1);
	CYBOZU_TEST_EQUAL(r, n1);
	n2 = sheCipherTextG1Serialize(buf2, sizeof(buf2), &c12);
	CYBOZU_TEST_EQUAL(n2, size);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n2);

	size = sizeofFp * 4;
	n1 = sheCipherTextG2Serialize(buf1, sizeof(buf1), &c21);
	CYBOZU_TEST_EQUAL(n1, size);
	r = sheCipherTextG2Deserialize(&c22, buf1, n1);
	CYBOZU_TEST_EQUAL(r, n1);
	n2 = sheCipherTextG2Serialize(buf2, sizeof(buf2), &c22);
	CYBOZU_TEST_EQUAL(n2, size);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n2);

	size = sizeofFp * 12 * 4;
	n1 = sheCipherTextGTSerialize(buf1, sizeof(buf1), &ct1);
	CYBOZU_TEST_EQUAL(n1, size);
	r = sheCipherTextGTDeserialize(&ct2, buf1, n1);
	CYBOZU_TEST_EQUAL(r, n1);
	n2 = sheCipherTextGTSerialize(buf2, sizeof(buf2), &ct2);
	CYBOZU_TEST_EQUAL(n2, size);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n2);
}

CYBOZU_TEST_AUTO(convert)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	sheCipherTextGT ct;
	const int64_t m = 123;
	int64_t dec;
	sheCipherTextG1 c1;
	sheEncG1(&c1, &pub, m);
	CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c1), 0);
	CYBOZU_TEST_EQUAL(dec, 123);
	sheConvertG1(&ct, &pub, &c1);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, 123);

	sheCipherTextG2 c2;
	sheEncG2(&c2, &pub, m);
	CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c2), 0);
	CYBOZU_TEST_EQUAL(dec, 123);
	sheConvertG2(&ct, &pub, &c2);
	dec = 0;
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, 123);
}

CYBOZU_TEST_AUTO(precomputed)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	shePrecomputedPublicKey *ppub = shePrecomputedPublicKeyCreate();
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyInit(ppub, &pub), 0);
	const int64_t m = 152;
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheCipherTextGT ct;
	int64_t dec = 0;
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyEncG1(&c1, ppub, m), 0);
	CYBOZU_TEST_EQUAL(sheDecG1(&dec, &sec, &c1), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyEncG2(&c2, ppub, m), 0);
	CYBOZU_TEST_EQUAL(sheDecG2(&dec, &sec, &c2), 0);
	CYBOZU_TEST_EQUAL(dec, m);
	dec = 0;
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyEncGT(&ct, ppub, m), 0);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, m);

	shePrecomputedPublicKeyDestroy(ppub);
}

template<class CT, class PK, class encWithZkpFunc, class decFunc, class verifyFunc>
void ZkpBinTest(const sheSecretKey *sec, const PK *pub, encWithZkpFunc encWithZkp, decFunc dec, verifyFunc verify)
{
	CT c;
	sheZkpBin zkp;
	for (int m = 0; m < 2; m++) {
		CYBOZU_TEST_EQUAL(encWithZkp(&c, &zkp, pub, m), 0);
		mclInt mDec;
		CYBOZU_TEST_EQUAL(dec(&mDec, sec, &c), 0);
		CYBOZU_TEST_EQUAL(mDec, m);
		CYBOZU_TEST_EQUAL(verify(pub, &c, &zkp), 1);
		{
			char buf[4096];
			size_t n = sheZkpBinSerialize(buf, sizeof(buf), &zkp);
			CYBOZU_TEST_EQUAL(n, mclBn_getFrByteSize() * CYBOZU_NUM_OF_ARRAY(zkp.d));
			sheZkpBin zkp2;
			size_t r = sheZkpBinDeserialize(&zkp2, buf, n);
			CYBOZU_TEST_EQUAL(r, n);
			CYBOZU_TEST_EQUAL(r, n);
			for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(zkp.d); i++) {
				CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&zkp.d[i], &zkp2.d[i]));
			}
		}
		zkp.d[0].d[0]++;
		CYBOZU_TEST_EQUAL(verify(pub, &c, &zkp), 0);
	}
	CYBOZU_TEST_ASSERT(encWithZkp(&c, &zkp, pub, 2) != 0);
}

template<class CT, class PK, class encWithZkpFunc, class decFunc, class verifyFunc>
void ZkpSetTest(const sheSecretKey *sec, const PK *pub, encWithZkpFunc encWithZkp, decFunc dec, verifyFunc verify)
{
	const int mVec[] = { -5, -1, 0, 1, 2, 3, 9 };
	const size_t N = CYBOZU_NUM_OF_ARRAY(mVec);
	CT c;
	mclBnFr zkp[N * 2];
	for (size_t mSize = 1; mSize <= N; mSize++) {
		for (size_t i = 0; i < mSize; i++) {
			int m = mVec[i];
			CYBOZU_TEST_EQUAL(encWithZkp(&c, zkp, pub, m, mVec, mSize), 0);
			mclInt mDec;
			CYBOZU_TEST_EQUAL(dec(&mDec, sec, &c), 0);
			CYBOZU_TEST_EQUAL(mDec, m);
			CYBOZU_TEST_EQUAL(verify(pub, &c, zkp, mVec, mSize), 1);
#if 0
			{
				char buf[4096];
				size_t n = sheZkpSetSerialize(buf, sizeof(buf), &zkp);
				CYBOZU_TEST_EQUAL(n, mclBn_getFrByteSize() * CYBOZU_NUM_OF_ARRAY(zkp.d));
				sheZkpSet zkp2;
				size_t r = sheZkpSetDeserialize(&zkp2, buf, n);
				CYBOZU_TEST_EQUAL(r, n);
				CYBOZU_TEST_EQUAL(r, n);
				for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(zkp.d); i++) {
					CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&zkp.d[i], &zkp2.d[i]));
				}
			}
#endif
			zkp[0].d[0]++;
			CYBOZU_TEST_EQUAL(verify(pub, &c, zkp, mVec, mSize), 0);
		}
		CYBOZU_TEST_ASSERT(encWithZkp(&c, zkp, pub, 12345, mVec, mSize) != 0);
	}
}

CYBOZU_TEST_AUTO(ZkpSet)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	shePrecomputedPublicKey *ppub = shePrecomputedPublicKeyCreate();
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyInit(ppub, &pub), 0);

	ZkpSetTest<sheCipherTextG1>(&sec, &pub, sheEncWithZkpSetG1, sheDecG1, sheVerifyZkpSetG1);
	ZkpSetTest<sheCipherTextG1>(&sec, ppub, shePrecomputedPublicKeyEncWithZkpSetG1, sheDecG1, shePrecomputedPublicKeyVerifyZkpSetG1);

	shePrecomputedPublicKeyDestroy(ppub);
}

CYBOZU_TEST_AUTO(ZkpBin)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	ZkpBinTest<sheCipherTextG1>(&sec, &pub, sheEncWithZkpBinG1, sheDecG1, sheVerifyZkpBinG1);
	ZkpBinTest<sheCipherTextG2>(&sec, &pub, sheEncWithZkpBinG2, sheDecG2, sheVerifyZkpBinG2);

	shePrecomputedPublicKey *ppub = shePrecomputedPublicKeyCreate();
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyInit(ppub, &pub), 0);

	ZkpBinTest<sheCipherTextG1>(&sec, ppub, shePrecomputedPublicKeyEncWithZkpBinG1, sheDecG1, shePrecomputedPublicKeyVerifyZkpBinG1);
	ZkpBinTest<sheCipherTextG2>(&sec, ppub, shePrecomputedPublicKeyEncWithZkpBinG2, sheDecG2, shePrecomputedPublicKeyVerifyZkpBinG2);

	shePrecomputedPublicKeyDestroy(ppub);
}

template<class PK, class encWithZkpFunc, class verifyFunc>
void ZkpBinEqTest(const sheSecretKey *sec, const PK *pub, encWithZkpFunc encWithZkp, verifyFunc verify)
{
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheZkpBinEq zkp;
	for (int m = 0; m < 2; m++) {
		CYBOZU_TEST_EQUAL(encWithZkp(&c1, &c2, &zkp, pub, m), 0);
		mclInt mDec = -1;
		CYBOZU_TEST_EQUAL(sheDecG1(&mDec, sec, &c1), 0);
		CYBOZU_TEST_EQUAL(mDec, m);
		mDec = -1;
		CYBOZU_TEST_EQUAL(sheDecG2(&mDec, sec, &c2), 0);
		CYBOZU_TEST_EQUAL(mDec, m);
		CYBOZU_TEST_EQUAL(verify(pub, &c1, &c2, &zkp), 1);
		{
			char buf[2048];
			size_t n = sheZkpBinEqSerialize(buf, sizeof(buf), &zkp);
			CYBOZU_TEST_EQUAL(n, mclBn_getFrByteSize() * CYBOZU_NUM_OF_ARRAY(zkp.d));
			sheZkpBinEq zkp2;
			size_t r = sheZkpBinEqDeserialize(&zkp2, buf, n);
			CYBOZU_TEST_EQUAL(r, n);
			for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(zkp.d); i++) {
				CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&zkp.d[i], &zkp2.d[i]));
			}
		}
		zkp.d[0].d[0]++;
		CYBOZU_TEST_EQUAL(verify(pub, &c1, &c2, &zkp), 0);
	}
	CYBOZU_TEST_ASSERT(encWithZkp(&c1, &c2, &zkp, pub, 2) != 0);
}

CYBOZU_TEST_AUTO(ZkpBinEq)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	ZkpBinEqTest(&sec, &pub, sheEncWithZkpBinEq, sheVerifyZkpBinEq);

	shePrecomputedPublicKey *ppub = shePrecomputedPublicKeyCreate();
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyInit(ppub, &pub), 0);

	ZkpBinEqTest(&sec, ppub, shePrecomputedPublicKeyEncWithZkpBinEq, shePrecomputedPublicKeyVerifyZkpBinEq);

	shePrecomputedPublicKeyDestroy(ppub);
}

template<class PK, class encWithZkpFunc, class verifyFunc>
void ZkpEqTest(const sheSecretKey *sec, const PK *pub, encWithZkpFunc encWithZkp, verifyFunc verify)
{
	sheCipherTextG1 c1;
	sheCipherTextG2 c2;
	sheZkpEq zkp;
	for (int m = -5; m < 5; m++) {
		CYBOZU_TEST_EQUAL(encWithZkp(&c1, &c2, &zkp, pub, m), 0);
		mclInt mDec = -1;
		CYBOZU_TEST_EQUAL(sheDecG1(&mDec, sec, &c1), 0);
		CYBOZU_TEST_EQUAL(mDec, m);
		mDec = -1;
		CYBOZU_TEST_EQUAL(sheDecG2(&mDec, sec, &c2), 0);
		CYBOZU_TEST_EQUAL(mDec, m);
		CYBOZU_TEST_EQUAL(verify(pub, &c1, &c2, &zkp), 1);
		{
			char buf[2048];
			size_t n = sheZkpEqSerialize(buf, sizeof(buf), &zkp);
			CYBOZU_TEST_EQUAL(n, mclBn_getFrByteSize() * CYBOZU_NUM_OF_ARRAY(zkp.d));
			sheZkpEq zkp2;
			size_t r = sheZkpEqDeserialize(&zkp2, buf, n);
			CYBOZU_TEST_EQUAL(r, n);
			for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(zkp.d); i++) {
				CYBOZU_TEST_ASSERT(mclBnFr_isEqual(&zkp.d[i], &zkp2.d[i]));
			}
		}
		zkp.d[0].d[0]++;
		CYBOZU_TEST_EQUAL(verify(pub, &c1, &c2, &zkp), 0);
	}
}

CYBOZU_TEST_AUTO(ZkpDecG1)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	int m = 123;
	sheCipherTextG1 c1;
	sheEncG1(&c1, &pub, m);
	sheZkpDec zkp;
	int64_t dec;
	CYBOZU_TEST_EQUAL(sheDecWithZkpDecG1(&dec, &zkp, &sec, &c1, &pub), 0);
	CYBOZU_TEST_EQUAL(m, dec);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecG1(&pub, &c1, m, &zkp), 1);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecG1(&pub, &c1, m + 1, &zkp), 0);
	sheCipherTextG1 c2;
	sheEncG1(&c2, &pub, m);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecG1(&pub, &c2, m, &zkp), 0);
	zkp.d[0].d[0]++;
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecG1(&pub, &c1, m, &zkp), 0);
}

CYBOZU_TEST_AUTO(ZkpDecGT)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	sheAuxiliaryForZkpDecGT aux;
	sheGetAuxiliaryForZkpDecGT(&aux, &pub);
	int m = 123;
	sheCipherTextGT c1;
	sheEncGT(&c1, &pub, m);
	sheZkpDecGT zkp;
	int64_t dec;
	CYBOZU_TEST_EQUAL(sheDecWithZkpDecGT(&dec, &zkp, &sec, &c1, &aux), 0);
	CYBOZU_TEST_EQUAL(m, dec);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecGT(&aux, &c1, m, &zkp), 1);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecGT(&aux, &c1, m + 1, &zkp), 0);
	sheCipherTextGT c2;
	sheEncGT(&c2, &pub, m);
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecGT(&aux, &c2, m, &zkp), 0);
	zkp.d[0].d[0]++;
	CYBOZU_TEST_EQUAL(sheVerifyZkpDecGT(&aux, &c1, m, &zkp), 0);
}

CYBOZU_TEST_AUTO(ZkpEq)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);

	ZkpEqTest(&sec, &pub, sheEncWithZkpEq, sheVerifyZkpEq);

	shePrecomputedPublicKey *ppub = shePrecomputedPublicKeyCreate();
	CYBOZU_TEST_EQUAL(shePrecomputedPublicKeyInit(ppub, &pub), 0);

	ZkpEqTest(&sec, ppub, shePrecomputedPublicKeyEncWithZkpEq, shePrecomputedPublicKeyVerifyZkpEq);

	shePrecomputedPublicKeyDestroy(ppub);
}

template<class CT, class ENC, class ENCV, class DEC, class SUB, class MUL>
void IntVecTest(const sheSecretKey& sec, const shePublicKey& pub, const ENC& enc, const ENCV& encv, const DEC& dec, const SUB& sub, const MUL& mul, uint8_t *buf, size_t bufSize)
{
	CT c1, c2;
	int ret;
	ret = encv(&c1, &pub, buf, bufSize);
	CYBOZU_TEST_EQUAL(ret, 0);
	buf[0] += 5;
	enc(&c2, &pub, 1);
	ret = mul(&c2, &c2, buf, bufSize);
	CYBOZU_TEST_EQUAL(ret, 0);
	sub(&c2, &c2, &c1);
	int64_t d;
	ret = dec(&d, &sec, &c2);
	CYBOZU_TEST_EQUAL(ret, 0);
	CYBOZU_TEST_EQUAL(d, 5);
}

CYBOZU_TEST_AUTO(IntVec)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	uint8_t buf[48];
	size_t n = 32;
	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = uint8_t(i + 5);
	}
	IntVecTest<sheCipherTextG1>(sec, pub, sheEncG1, sheEncIntVecG1, sheDecG1, sheSubG1, sheMulIntVecG1, buf, n);
	IntVecTest<sheCipherTextG2>(sec, pub, sheEncG2, sheEncIntVecG2, sheDecG2, sheSubG2, sheMulIntVecG2, buf, n);
	IntVecTest<sheCipherTextGT>(sec, pub, sheEncGT, sheEncIntVecGT, sheDecGT, sheSubGT, sheMulIntVecGT, buf, n);
}

CYBOZU_TEST_AUTO(finalExp)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	const int64_t m11 = 5;
	const int64_t m12 = 7;
	const int64_t m21 = -3;
	const int64_t m22 = 9;
	sheCipherTextG1 c11, c12;
	sheCipherTextG2 c21, c22;
	sheCipherTextGT ct1, ct2;
	sheCipherTextGT ct;
	sheEncG1(&c11, &pub, m11);
	sheEncG1(&c12, &pub, m12);
	sheEncG2(&c21, &pub, m21);
	sheEncG2(&c22, &pub, m22);

	int64_t dec;
	// sheMul = sheMulML + sheFinalExpGT
	sheMul(&ct1, &c11, &c21);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct1), 0);
	CYBOZU_TEST_EQUAL(dec, m11 * m21);

	sheMulML(&ct1, &c11, &c21);
	sheFinalExpGT(&ct, &ct1);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, m11 * m21);

	sheMulML(&ct2, &c12, &c22);
	sheFinalExpGT(&ct, &ct2);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, m12 * m22);

	/*
		Mul(c11, c21) + Mul(c21, c22)
		= finalExp(ML(c11, c21) + ML(c21, c22))
	*/
	sheAddGT(&ct, &ct1, &ct2);
	sheFinalExpGT(&ct, &ct);
	CYBOZU_TEST_EQUAL(sheDecGT(&dec, &sec, &ct), 0);
	CYBOZU_TEST_EQUAL(dec, (m11 * m21) + (m12 * m22));
}

int g_hashBitSize = 8;
std::string g_tableName;

CYBOZU_TEST_AUTO(saveLoad)
{
	sheSecretKey sec;
	sheSecretKeySetByCSPRNG(&sec);
	shePublicKey pub;
	sheGetPublicKey(&pub, &sec);
	const size_t hashSize = 1 << g_hashBitSize;
	const size_t byteSizePerEntry = 8;
	sheSetRangeForGTDLP(hashSize);
	std::string buf;
	buf.resize(hashSize * byteSizePerEntry + 1024);
	const size_t n1 = sheSaveTableForGTDLP(&buf[0], buf.size());
	CYBOZU_TEST_ASSERT(n1 > 0);
	if (!g_tableName.empty()) {
		printf("use table=%s\n", g_tableName.c_str());
		std::ofstream ofs(g_tableName.c_str(), std::ios::binary);
		ofs.write(buf.c_str(), n1);
	}
	const int64_t m = hashSize - 1;
	sheCipherTextGT ct;
	CYBOZU_TEST_ASSERT(sheEncGT(&ct, &pub, m) == 0);
	sheSetRangeForGTDLP(1);
	sheSetTryNum(1);
	int64_t dec = 0;
	CYBOZU_TEST_ASSERT(sheDecGT(&dec, &sec, &ct) != 0);
	if (!g_tableName.empty()) {
		std::ifstream ifs(g_tableName.c_str(), std::ios::binary);
		buf.clear();
		buf.resize(n1);
		ifs.read(&buf[0], n1);
	}
	const size_t n2 = sheLoadTableForGTDLP(&buf[0], n1);
	CYBOZU_TEST_ASSERT(n2 > 0);
	CYBOZU_TEST_ASSERT(sheDecGT(&dec, &sec, &ct) == 0);
	CYBOZU_TEST_EQUAL(dec, m);
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	opt.appendOpt(&g_hashBitSize, 8, "bit", ": hashBitSize");
	opt.appendOpt(&g_tableName, "", "f", ": table name");
	opt.appendHelp("h", ": show this message");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	return cybozu::test::autoRun.run(argc, argv);
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}
