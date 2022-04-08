#include <mcl/ecdsa.h>
#include <cybozu/test.hpp>
#include <string.h>
#include <stdint.h>

template<class T, class Serializer, class Deserializer>
void serializeTest(const T& x, const Serializer& serialize, const Deserializer& deserialize)
{
	uint8_t buf1[128];
	uint8_t buf2[128];
	size_t n = serialize(buf1, sizeof(buf1), &x);
	CYBOZU_TEST_ASSERT(n > 0);
	T y;
	size_t m = deserialize(&y, buf1, n);
	CYBOZU_TEST_EQUAL(m, n);
	size_t n2 = serialize(buf2, sizeof(buf2), &y);
	CYBOZU_TEST_EQUAL(n, n2);
	CYBOZU_TEST_EQUAL_ARRAY(buf1, buf2, n);
}

CYBOZU_TEST_AUTO(ecdsa)
{
	int ret;
	ret = ecdsaInit();
	CYBOZU_TEST_EQUAL(ret, 0);
	ecdsaSecretKey sec;
	ecdsaPublicKey pub;
	ecdsaPrecomputedPublicKey *ppub;
	ecdsaSignature sig;
	const char *msg = "hello";
	mclSize msgSize = strlen(msg);

	ret = ecdsaSecretKeySetByCSPRNG(&sec);
	CYBOZU_TEST_EQUAL(ret, 0);
	serializeTest(sec, ecdsaSecretKeySerialize, ecdsaSecretKeyDeserialize);

	ecdsaGetPublicKey(&pub, &sec);
	serializeTest(pub, ecdsaPublicKeySerialize, ecdsaPublicKeyDeserialize);
	ecdsaSign(&sig, &sec, msg, msgSize);
	serializeTest(sig, ecdsaSignatureSerialize, ecdsaSignatureDeserialize);
	CYBOZU_TEST_ASSERT(ecdsaVerify(&sig, &pub, msg, msgSize));
	ecdsaNormalizeSignature(&sig);
	CYBOZU_TEST_ASSERT(ecdsaVerify(&sig, &pub, msg, msgSize));

	ppub = ecdsaPrecomputedPublicKeyCreate();
	CYBOZU_TEST_ASSERT(ppub);
	ret = ecdsaPrecomputedPublicKeyInit(ppub, &pub);
	CYBOZU_TEST_EQUAL(ret, 0);

	CYBOZU_TEST_ASSERT(ecdsaVerifyPrecomputed(&sig, ppub, msg, msgSize));

	sig.d[0]++;
	CYBOZU_TEST_ASSERT(!ecdsaVerify(&sig, &pub, msg, msgSize));
	CYBOZU_TEST_ASSERT(!ecdsaVerifyPrecomputed(&sig, ppub, msg, msgSize));

	ecdsaPrecomputedPublicKeyDestroy(ppub);
}
