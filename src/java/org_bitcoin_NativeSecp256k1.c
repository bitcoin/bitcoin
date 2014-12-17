#include <stdlib.h>
#include <string.h>
#include "org_bitcoin_NativeSecp256k1.h"
#include "include/secp256k1.h"
#include "include/secp256k1_recovery.h"


SECP256K1_API jlong JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ctx_1clone
  (JNIEnv* env, jclass classObject, jlong ctx_l)
{
  const secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  jlong ctx_clone_l = (jlong) secp256k1_context_clone(ctx);

  (void)classObject;(void)env;

  return (jlong)ctx_clone_l;

}

SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1context_1randomize
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  const unsigned char* seed = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);

  (void)classObject;

  return secp256k1_context_randomize(ctx, seed);

}

SECP256K1_API void JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1destroy_1context
  (JNIEnv* env, jclass classObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  secp256k1_context_destroy(ctx);

  (void)classObject;(void)env;
}

SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1verify
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint siglen, jint publen)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  int result;
  unsigned char* data = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* sigdata = {  (unsigned char*) (data + 32) };
  const unsigned char* pubdata = { (unsigned char*) (data + siglen + 32) };

  secp256k1_ecdsa_signature sig;
  secp256k1_pubkey pubkey;

  int ret = secp256k1_ecdsa_signature_parse_der(ctx, &sig, sigdata, siglen);

  if( ret ) {
    ret = secp256k1_ec_pubkey_parse(ctx, &pubkey, pubdata, publen);
  }

  /*Debug
  printf("\nData: ");
  int i;
  for( i = 0; i < 32; i++) printf("%x", data[i]);
  printf("\nSig: ");
  for( i = 0; i < 64; i++) printf("%x", sig->data[i]);
  printf("\nPub: ");
  for( i = 0; i < 64; i++) printf("%x", pub->data[i]);
  */

  (void)classObject;

  result = secp256k1_ecdsa_verify(ctx, &sig, data, &pubkey);

  return result;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1sign
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* data = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  unsigned char* secKey = (unsigned char*) (data + 32);

  jobjectArray retArray;
  jbyteArray sigArray, intsByteArray;
  unsigned char intsarray[2];

  secp256k1_ecdsa_signature sig[72];

  int ret = secp256k1_ecdsa_sign(ctx, sig, data, secKey, NULL, NULL );

  unsigned char outputSer[72];
  size_t outputLen = 72;

  if( ret ) {
    int ret2 = secp256k1_ecdsa_signature_serialize_der(ctx,outputSer, &outputLen, sig ); (void)ret2;
  }

  intsarray[0] = outputLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  sigArray = (*env)->NewByteArray(env, outputLen);
  (*env)->SetByteArrayRegion(env, sigArray, 0, outputLen, (jbyte*)outputSer);
  (*env)->SetObjectArrayElement(env, retArray, 0, sigArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1seckey_1verify
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* secKey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);

  (void)classObject;

  return secp256k1_ec_seckey_verify(ctx, secKey);
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1pubkey_1create
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  const unsigned char* secKey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);

  secp256k1_pubkey pubkey;

  jobjectArray retArray;
  jbyteArray pubkeyArray, intsByteArray;
  unsigned char intsarray[2];

  int ret = secp256k1_ec_pubkey_create(ctx, &pubkey, secKey);

  unsigned char outputSer[65];
  size_t outputLen = 65;

  if( ret ) {
    int ret2 = secp256k1_ec_pubkey_serialize(ctx,outputSer, &outputLen, &pubkey,SECP256K1_EC_UNCOMPRESSED );(void)ret2;
  }

  intsarray[0] = outputLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  pubkeyArray = (*env)->NewByteArray(env, outputLen);
  (*env)->SetByteArrayRegion(env, pubkeyArray, 0, outputLen, (jbyte*)outputSer);
  (*env)->SetObjectArrayElement(env, retArray, 0, pubkeyArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;

}

/* TODO replace with contrib/ code */
SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1privkey_1export
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint privLen, jint compressed)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  const unsigned char* secKey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);

  int i, ret;
  jobjectArray retArray;
  jbyteArray privkeyArray, intsByteArray;
  unsigned char intsarray[2];

  unsigned char* privkey = (unsigned char*) malloc(sizeof(unsigned char)*279);

  for(i = 0; i < privLen; i++) privkey[i] = secKey[i];

  ret = secp256k1_ec_privkey_export(ctx, privkey , (size_t*)&privLen, secKey, compressed);

  intsarray[0] = privLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  privkeyArray = (*env)->NewByteArray(env, privLen);
  (*env)->SetByteArrayRegion(env, privkeyArray, 0, privLen, (jbyte*)privkey);
  (*env)->SetObjectArrayElement(env, retArray, 0, privkeyArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

/* TODO replace with contrib/ code */
SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1privkey_1import
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint privLen)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;

  const unsigned char* secKey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);

  jobjectArray retArray;
  jbyteArray privkeyArray, intsByteArray;
  unsigned char intsarray[2];

  unsigned char privkey[32];

  int ret = secp256k1_ec_privkey_import(ctx, privkey, secKey, privLen);

  privLen = 32;

  intsarray[0] = privLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  privkeyArray = (*env)->NewByteArray(env, privLen);
  (*env)->SetByteArrayRegion(env, privkeyArray, 0, privLen, (jbyte*)privkey);
  (*env)->SetObjectArrayElement(env, retArray, 0, privkeyArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}


/* TODO replace with serialize_compact() code */
/*SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1sign_1compact
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  const unsigned char* data = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* secKey = (unsigned char*) (data + 32);

  jobjectArray retArray;
  jbyteArray sigArray, intsByteArray;
  unsigned char intsarray[2];

  unsigned char sig[64];
  int siglen = 64;
  int recID;

  int ret = secp256k1_ecdsa_sign_compact(ctx, data, sig, secKey, NULL, NULL, &recID );

  intsarray[0] = recID;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  sigArray = (*env)->NewByteArray(env, siglen);
  (*env)->SetByteArrayRegion(env, sigArray, 0, siglen, (jbyte*)sig);
  (*env)->SetObjectArrayElement(env, retArray, 0, sigArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}*/


/* TODO replace with recover_compact() code */
/*
SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1recover_1compact
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint compressed, jint recid)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  const unsigned char* msg = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* sig = (unsigned char*) (msg + 32);

  jobjectArray retArray;
  jbyteArray pubArray, intsByteArray;
  unsigned char intsarray[2];

  unsigned char pubkey[64];
  int pubkeylen;

  int ret = secp256k1_ecdsa_recover_compact(ctx, msg, sig, pubkey, &pubkeylen, compressed, recid );

  intsarray[0] = pubkeylen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  pubArray = (*env)->NewByteArray(env, pubkeylen);
  (*env)->SetByteArrayRegion(env, pubArray, 0, pubkeylen, (jbyte*)pubkey);
  (*env)->SetObjectArrayElement(env, retArray, 0, pubArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}*/

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1privkey_1tweak_1add
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* privkey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* tweak = (unsigned char*) (privkey + 32);

  jobjectArray retArray;
  jbyteArray privArray, intsByteArray;
  unsigned char intsarray[2];

  int privkeylen = 32;

  int ret = secp256k1_ec_privkey_tweak_add(ctx, privkey, tweak);

  intsarray[0] = privkeylen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  privArray = (*env)->NewByteArray(env, privkeylen);
  (*env)->SetByteArrayRegion(env, privArray, 0, privkeylen, (jbyte*)privkey);
  (*env)->SetObjectArrayElement(env, retArray, 0, privArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1privkey_1tweak_1mul
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* privkey = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* tweak = (unsigned char*) (privkey + 32);

  jobjectArray retArray;
  jbyteArray privArray, intsByteArray;
  unsigned char intsarray[2];

  int privkeylen = 32;

  int ret = secp256k1_ec_privkey_tweak_mul(ctx, privkey, tweak);

  intsarray[0] = privkeylen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  privArray = (*env)->NewByteArray(env, privkeylen);
  (*env)->SetByteArrayRegion(env, privArray, 0, privkeylen, (jbyte*)privkey);
  (*env)->SetObjectArrayElement(env, retArray, 0, privArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1pubkey_1tweak_1add
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint publen)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
/*  secp256k1_pubkey* pubkey = (secp256k1_pubkey*) (*env)->GetDirectBufferAddress(env, byteBufferObject);*/
  unsigned char* pkey = (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* tweak = (unsigned char*) (pkey + publen);

  jobjectArray retArray;
  jbyteArray pubArray, intsByteArray;
  unsigned char intsarray[2];
  unsigned char outputSer[65];
  size_t outputLen = 65;

  secp256k1_pubkey pubkey;
  int ret = secp256k1_ec_pubkey_parse(ctx, &pubkey, pkey, publen);

  if( ret ) {
    ret = secp256k1_ec_pubkey_tweak_add(ctx, &pubkey, tweak);
  }

  if( ret ) {
    int ret2 = secp256k1_ec_pubkey_serialize(ctx,outputSer, &outputLen, &pubkey,SECP256K1_EC_UNCOMPRESSED );(void)ret2;
  }

  intsarray[0] = outputLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  pubArray = (*env)->NewByteArray(env, outputLen);
  (*env)->SetByteArrayRegion(env, pubArray, 0, outputLen, (jbyte*)outputSer);
  (*env)->SetObjectArrayElement(env, retArray, 0, pubArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1pubkey_1tweak_1mul
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint publen)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* pkey = (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* tweak = (unsigned char*) (pkey + publen);

  jobjectArray retArray;
  jbyteArray pubArray, intsByteArray;
  unsigned char intsarray[2];
  unsigned char outputSer[65];
  size_t outputLen = 65;

  secp256k1_pubkey pubkey;
  int ret = secp256k1_ec_pubkey_parse(ctx, &pubkey, pkey, publen);

  if ( ret ) {
    ret = secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, tweak);
  }

  if( ret ) {
    int ret2 = secp256k1_ec_pubkey_serialize(ctx,outputSer, &outputLen, &pubkey,SECP256K1_EC_UNCOMPRESSED );(void)ret2;
  }

  intsarray[0] = outputLen;
  intsarray[1] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  pubArray = (*env)->NewByteArray(env, outputLen);
  (*env)->SetByteArrayRegion(env, pubArray, 0, outputLen, (jbyte*)outputSer);
  (*env)->SetObjectArrayElement(env, retArray, 0, pubArray);

  intsByteArray = (*env)->NewByteArray(env, 2);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 2, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jlong JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1pubkey_1combine
  (JNIEnv * env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint numkeys)
{
  (void)classObject;(void)env;(void)byteBufferObject;(void)ctx_l;(void)numkeys;

  return 0;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1schnorr_1sign
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  unsigned char* data = (unsigned char*) (*env)->GetDirectBufferAddress(env, byteBufferObject);
  unsigned char* secKey = (unsigned char*) (data + 32);

  jobjectArray retArray;
  jbyteArray sigArray, intsByteArray;
  unsigned char intsarray[1];
  unsigned char sig[64];

  int ret = secp256k1_schnorr_sign(ctx, sig, data, secKey, NULL, NULL);

  intsarray[0] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  sigArray = (*env)->NewByteArray(env, 64);
  (*env)->SetByteArrayRegion(env, sigArray, 0, 64, (jbyte*)sig);
  (*env)->SetObjectArrayElement(env, retArray, 0, sigArray);

  intsByteArray = (*env)->NewByteArray(env, 1);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 1, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}

SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdh
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint publen)
{
  secp256k1_context *ctx = (secp256k1_context*)ctx_l;
  const unsigned char* secdata = (*env)->GetDirectBufferAddress(env, byteBufferObject);
  const unsigned char* pubdata = (const unsigned char*) (secdata + 32);

  jobjectArray retArray;
  jbyteArray outArray, intsByteArray;
  unsigned char intsarray[1];
  secp256k1_pubkey pubkey;
  unsigned char nonce_res[32];
  size_t outputLen = 32;

  int ret = secp256k1_ec_pubkey_parse(ctx, &pubkey, pubdata, publen);

  if (ret) {
    ret = secp256k1_ecdh(
      ctx,
      nonce_res,
      &pubkey,
      secdata
    );
  }

  intsarray[0] = ret;

  retArray = (*env)->NewObjectArray(env, 2,
    (*env)->FindClass(env, "[B"),
    (*env)->NewByteArray(env, 1));

  outArray = (*env)->NewByteArray(env, outputLen);
  (*env)->SetByteArrayRegion(env, outArray, 0, 32, (jbyte*)nonce_res);
  (*env)->SetObjectArrayElement(env, retArray, 0, outArray);

  intsByteArray = (*env)->NewByteArray(env, 1);
  (*env)->SetByteArrayRegion(env, intsByteArray, 0, 1, (jbyte*)intsarray);
  (*env)->SetObjectArrayElement(env, retArray, 1, intsByteArray);

  (void)classObject;

  return retArray;
}