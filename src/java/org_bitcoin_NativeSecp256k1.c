#include "org_bitcoin_NativeSecp256k1.h"
#include "include/secp256k1.h"

JNIEXPORT jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1verify
  (JNIEnv* env, jclass classObject, jobject byteBufferObject)
{
	unsigned char* data = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
	int sigLen = *((int*)(data + 32));
	int pubLen = *((int*)(data + 32 + 4));

	return secp256k1_ecdsa_verify(data, 32, data+32+8, sigLen, data+32+8+sigLen, pubLen);
}

static void __javasecp256k1_attach(void) __attribute__((constructor));
static void __javasecp256k1_detach(void) __attribute__((destructor));

static void __javasecp256k1_attach(void) {
	secp256k1_start(SECP256K1_START_VERIFY);
}

static void __javasecp256k1_detach(void) {
	secp256k1_stop();
}
