#define ECDSA_DLL_EXPORT
#include <mcl/ecdsa.h>
#include <mcl/ecdsa.hpp>
#include <new>

using namespace mcl::ecdsa;

static SecretKey *cast(ecdsaSecretKey *p) { return reinterpret_cast<SecretKey*>(p); }
static const SecretKey *cast(const ecdsaSecretKey *p) { return reinterpret_cast<const SecretKey*>(p); }

static PublicKey *cast(ecdsaPublicKey *p) { return reinterpret_cast<PublicKey*>(p); }
static const PublicKey *cast(const ecdsaPublicKey *p) { return reinterpret_cast<const PublicKey*>(p); }

static Signature *cast(ecdsaSignature *p) { return reinterpret_cast<Signature*>(p); }
static const Signature *cast(const ecdsaSignature *p) { return reinterpret_cast<const Signature*>(p); }

static PrecomputedPublicKey *cast(ecdsaPrecomputedPublicKey *p) { return reinterpret_cast<PrecomputedPublicKey*>(p); }
static const PrecomputedPublicKey *cast(const ecdsaPrecomputedPublicKey *p) { return reinterpret_cast<const PrecomputedPublicKey*>(p); }

#ifdef __EMSCRIPTEN__
// use these functions forcibly
extern "C" ECDSA_DLL_API void *ecdsaMalloc(size_t n)
{
	return malloc(n);
}
extern "C" ECDSA_DLL_API void ecdsaFree(void *p)
{
	free(p);
}
#endif

int ecdsaInit(void)
{
	bool b;
	init(&b);
	return b ? 0 : -1;
}

int ecdsaSetSerializeMode(int mode)
{
	return setSeriailzeMode(mode);
}

mclSize ecdsaSecretKeySerialize(void *buf, mclSize maxBufSize, const ecdsaSecretKey *sec)
{
	return (mclSize)cast(sec)->serialize(buf, maxBufSize);
}
mclSize ecdsaPublicKeySerialize(void *buf, mclSize maxBufSize, const ecdsaPublicKey *pub)
{
	return (mclSize)cast(pub)->serialize(buf, maxBufSize);
}
mclSize ecdsaPublicKeySerializeCompressed(void *buf, mclSize maxBufSize, const ecdsaPublicKey *pub)
{
	return (mclSize)cast(pub)->serializeCompressed(buf, maxBufSize);
}
mclSize ecdsaSignatureSerialize(void *buf, mclSize maxBufSize, const ecdsaSignature *sig)
{
	return (mclSize)cast(sig)->serialize(buf, maxBufSize);
}

mclSize ecdsaSecretKeyDeserialize(ecdsaSecretKey* sec, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(sec)->deserialize(buf, bufSize);
}
mclSize ecdsaPublicKeyDeserialize(ecdsaPublicKey* pub, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(pub)->deserialize(buf, bufSize);
}
mclSize ecdsaSignatureDeserialize(ecdsaSignature* sig, const void *buf, mclSize bufSize)
{
	return (mclSize)cast(sig)->deserialize(buf, bufSize);
}

//	return 0 if success
int ecdsaSecretKeySetByCSPRNG(ecdsaSecretKey *sec)
{
	bool b;
	cast(sec)->setByCSPRNG(&b);
	if (!b) return -1;
	return 0;
}

void ecdsaGetPublicKey(ecdsaPublicKey *pub, const ecdsaSecretKey *sec)
{
	getPublicKey(*cast(pub), *cast(sec));
}

void ecdsaSign(ecdsaSignature *sig, const ecdsaSecretKey *sec, const void *m, mclSize size)
{
	sign(*cast(sig), *cast(sec), m, size);
}

void ecdsaNormalizeSignature(ecdsaSignature *sig)
{
	normalizeSignature(*cast(sig));
}

int ecdsaVerify(const ecdsaSignature *sig, const ecdsaPublicKey *pub, const void *m, mclSize size)
{
	return verify(*cast(sig), *cast(pub), m, size);
}
int ecdsaVerifyPrecomputed(const ecdsaSignature *sig, const ecdsaPrecomputedPublicKey *ppub, const void *m, mclSize size)
{
	return verify(*cast(sig), *cast(ppub), m, size);
}

ecdsaPrecomputedPublicKey *ecdsaPrecomputedPublicKeyCreate()
{
	PrecomputedPublicKey *ppub = (PrecomputedPublicKey*)malloc(sizeof(PrecomputedPublicKey));
	if (ppub == 0) return 0;
	new(ppub) PrecomputedPublicKey();
	return reinterpret_cast<ecdsaPrecomputedPublicKey*>(ppub);
}

void ecdsaPrecomputedPublicKeyDestroy(ecdsaPrecomputedPublicKey *ppub)
{
	cast(ppub)->~PrecomputedPublicKey();
	free(ppub);
}

int ecdsaPrecomputedPublicKeyInit(ecdsaPrecomputedPublicKey *ppub, const ecdsaPublicKey *pub)
{
	bool b;
	cast(ppub)->init(&b, *cast(pub));
	return b ? 0 : -1;
}
