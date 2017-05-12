// ripemd.h - written and placed in the public domain by Wei Dai

//! \file
//! \brief Classes for RIPEMD message digest

#ifndef CRYPTOPP_RIPEMD_H
#define CRYPTOPP_RIPEMD_H

#include "iterhash.h"

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.weidai.com/scan-mirror/md.html#RIPEMD-160">RIPEMD-160</a>
/*! Digest Length = 160 bits */
class RIPEMD160 : public IteratedHashWithStaticTransform<word32, LittleEndian, 64, 20, RIPEMD160>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "RIPEMD-160";}
};

/*! Digest Length = 320 bits, Security is similar to RIPEMD-160 */
class RIPEMD320 : public IteratedHashWithStaticTransform<word32, LittleEndian, 64, 40, RIPEMD320>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "RIPEMD-320";}
};

/*! \warning RIPEMD-128 is considered insecure, and should not be used
	unless you absolutely need it for compatibility. */
class RIPEMD128 : public IteratedHashWithStaticTransform<word32, LittleEndian, 64, 16, RIPEMD128>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "RIPEMD-128";}
};

/*! \warning RIPEMD-256 is considered insecure, and should not be used
	unless you absolutely need it for compatibility. */
class RIPEMD256 : public IteratedHashWithStaticTransform<word32, LittleEndian, 64, 32, RIPEMD256>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "RIPEMD-256";}
};

NAMESPACE_END

#endif
