// emsa2.h - written and placed in the public domain by Wei Dai

//! \file emsa2.h
//! \brief Classes and functions for various padding schemes used in public key algorithms

#ifndef CRYPTOPP_EMSA2_H
#define CRYPTOPP_EMSA2_H

#include "cryptlib.h"
#include "pubkey.h"
#include "misc.h"

#ifdef CRYPTOPP_IS_DLL
# include "sha.h"
#endif

NAMESPACE_BEGIN(CryptoPP)

template <class H> class EMSA2HashId
{
public:
	static const byte id;
};

template <class BASE>
class EMSA2HashIdLookup : public BASE
{
public:
	struct HashIdentifierLookup
	{
		template <class H> struct HashIdentifierLookup2
		{
			static HashIdentifier Lookup()
			{
				return HashIdentifier(&EMSA2HashId<H>::id, 1);
			}
		};
	};
};

// EMSA2HashId can be instantiated with the following classes.
class SHA1;
class SHA224;
class SHA256;
class SHA384;
class SHA512;
class RIPEMD128;
class RIPEMD160;
class Whirlpool;
// end of list

#ifdef CRYPTOPP_IS_DLL
CRYPTOPP_DLL_TEMPLATE_CLASS EMSA2HashId<SHA1>;
CRYPTOPP_DLL_TEMPLATE_CLASS EMSA2HashId<SHA224>;
CRYPTOPP_DLL_TEMPLATE_CLASS EMSA2HashId<SHA256>;
CRYPTOPP_DLL_TEMPLATE_CLASS EMSA2HashId<SHA384>;
CRYPTOPP_DLL_TEMPLATE_CLASS EMSA2HashId<SHA512>;
#endif

//! _
class CRYPTOPP_DLL EMSA2Pad : public EMSA2HashIdLookup<PK_DeterministicSignatureMessageEncodingMethod>
{
public:
	CRYPTOPP_CONSTEXPR static const char * CRYPTOPP_API StaticAlgorithmName() {return "EMSA2";}

	size_t MinRepresentativeBitLength(size_t hashIdentifierLength, size_t digestLength) const
		{CRYPTOPP_UNUSED(hashIdentifierLength); return 8*digestLength + 31;}

	void ComputeMessageRepresentative(RandomNumberGenerator &rng,
		const byte *recoverableMessage, size_t recoverableMessageLength,
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, size_t representativeBitLength) const;
};

//! EMSA2, for use with RWSS and RSA_ISO
/*! Only the following hash functions are supported by this signature standard:
	\dontinclude emsa2.h
	\skip EMSA2HashId can be instantiated
	\until end of list
*/
struct P1363_EMSA2 : public SignatureStandard
{
	typedef EMSA2Pad SignatureMessageEncodingMethod;
};

NAMESPACE_END

#endif
