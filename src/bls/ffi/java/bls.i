%module Bls

%include "std_string.i"
%include "std_except.i"
%include "std_vector.i"

%apply(char *STRING, size_t LENGTH) { (const char *cbuf, size_t bufSize) };
%{
#include <bls/bls384_256.h>

#include "bls_impl.hpp"

%}

%typemap(jtype) void serialize "byte[]"
%typemap(jstype) void serialize "byte[]"
%typemap(jni) void serialize "jbyteArray"
%typemap(javaout) void serialize { return $jnicall; }
%typemap(in, numinputs=0) std::string& out (std::string buf) "$1=&buf;"
%typemap(argout) std::string& out {
  $result = JCALL1(NewByteArray, jenv, $1->size());
  JCALL4(SetByteArrayRegion, jenv, $result, 0, $1->size(), (const jbyte*)$1->c_str());
}

%include "bls_impl.hpp"

%javaconst(1);
#define BN254 0
#define BLS12_381 5

#define MAP_TO_MODE_ORIGINAL 0
#define MAP_TO_MODE_HASH_TO_CURVE 5

#define MSG_SIZE 32

%template(SecretKeyVec) std::vector<SecretKey>;
%template(PublicKeyVec) std::vector<PublicKey>;
%template(SignatureVec) std::vector<Signature>;

