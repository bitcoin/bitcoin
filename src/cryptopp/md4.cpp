// md4.cpp - modified by Wei Dai from Andrew M. Kuchling's md4.c
// The original code and all modifications are in the public domain.

// This is the original introductory comment:

/*
 *  md4.c : MD4 hash algorithm.
 *
 * Part of the Python Cryptography Toolkit, version 1.1
 *
 * Distribute and use freely; there are no restrictions on further
 * dissemination and usage except those imposed by the laws of your
 * country of residence.
 *
 */

#include "pch.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md4.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)
namespace Weak1 {

void MD4::InitState(HashWordType *state)
{
	state[0] = 0x67452301L;
	state[1] = 0xefcdab89L;
	state[2] = 0x98badcfeL;
	state[3] = 0x10325476L;
}

void MD4::Transform (word32 *digest, const word32 *in)
{
// #define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

    word32 A, B, C, D;

	A=digest[0];
	B=digest[1];
	C=digest[2];
	D=digest[3];

#define function(a,b,c,d,k,s) a=rotlFixed(a+F(b,c,d)+in[k],s);
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 1, 7);
	  function(C,D,A,B, 2,11);
	  function(B,C,D,A, 3,19);
	  function(A,B,C,D, 4, 3);
	  function(D,A,B,C, 5, 7);
	  function(C,D,A,B, 6,11);
	  function(B,C,D,A, 7,19);
	  function(A,B,C,D, 8, 3);
	  function(D,A,B,C, 9, 7);
	  function(C,D,A,B,10,11);
	  function(B,C,D,A,11,19);
	  function(A,B,C,D,12, 3);
	  function(D,A,B,C,13, 7);
	  function(C,D,A,B,14,11);
	  function(B,C,D,A,15,19);

#undef function
#define function(a,b,c,d,k,s) a=rotlFixed(a+G(b,c,d)+in[k]+0x5a827999,s);
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 4, 5);
	  function(C,D,A,B, 8, 9);
	  function(B,C,D,A,12,13);
	  function(A,B,C,D, 1, 3);
	  function(D,A,B,C, 5, 5);
	  function(C,D,A,B, 9, 9);
	  function(B,C,D,A,13,13);
	  function(A,B,C,D, 2, 3);
	  function(D,A,B,C, 6, 5);
	  function(C,D,A,B,10, 9);
	  function(B,C,D,A,14,13);
	  function(A,B,C,D, 3, 3);
	  function(D,A,B,C, 7, 5);
	  function(C,D,A,B,11, 9);
	  function(B,C,D,A,15,13);

#undef function
#define function(a,b,c,d,k,s) a=rotlFixed(a+H(b,c,d)+in[k]+0x6ed9eba1,s);
	  function(A,B,C,D, 0, 3);
	  function(D,A,B,C, 8, 9);
	  function(C,D,A,B, 4,11);
	  function(B,C,D,A,12,15);
	  function(A,B,C,D, 2, 3);
	  function(D,A,B,C,10, 9);
	  function(C,D,A,B, 6,11);
	  function(B,C,D,A,14,15);
	  function(A,B,C,D, 1, 3);
	  function(D,A,B,C, 9, 9);
	  function(C,D,A,B, 5,11);
	  function(B,C,D,A,13,15);
	  function(A,B,C,D, 3, 3);
	  function(D,A,B,C,11, 9);
	  function(C,D,A,B, 7,11);
	  function(B,C,D,A,15,15);

	digest[0]+=A;
	digest[1]+=B;
	digest[2]+=C;
	digest[3]+=D;
}

}
NAMESPACE_END
