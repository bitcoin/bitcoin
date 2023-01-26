%module Mcl

%include "std_string.i"
%include "std_except.i"

%apply(char *STRING, size_t LENGTH) { (const char *cbuf, size_t bufSize) };
%{
#include <mcl/bls12_381.hpp>

#include "mcl_impl.hpp"

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

%include "mcl_impl.hpp"

%javaconst(1);
#define BN254 0
#define BLS12_381 5
#define SECP256K1 102

