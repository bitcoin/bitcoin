// pssr.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile pssr.h
//! \brief Classes for probablistic signature schemes

#ifndef CRYPTOPP_PSSR_H
#define CRYPTOPP_PSSR_H

#include "cryptlib.h"
#include "pubkey.h"
#include "emsa2.h"

#ifdef CRYPTOPP_IS_DLL
#include "sha.h"
#endif

NAMESPACE_BEGIN(CryptoPP)

class CRYPTOPP_DLL PSSR_MEM_Base : public PK_RecoverableSignatureMessageEncodingMethod
{
	virtual bool AllowRecovery() const =0;
	virtual size_t SaltLen(size_t hashLen) const =0;
	virtual size_t MinPadLen(size_t hashLen) const =0;
	virtual const MaskGeneratingFunction & GetMGF() const =0;

public:
	size_t MinRepresentativeBitLength(size_t hashIdentifierLength, size_t digestLength) const;
	size_t MaxRecoverableLength(size_t representativeBitLength, size_t hashIdentifierLength, size_t digestLength) const;
	bool IsProbabilistic() const;
	bool AllowNonrecoverablePart() const;
	bool RecoverablePartFirst() const;
	void ComputeMessageRepresentative(RandomNumberGenerator &rng, 
		const byte *recoverableMessage, size_t recoverableMessageLength,
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, size_t representativeBitLength) const;
	DecodingResult RecoverMessageFromRepresentative(
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, size_t representativeBitLength,
		byte *recoverableMessage) const;
};

template <bool USE_HASH_ID> class PSSR_MEM_BaseWithHashId;
template<> class PSSR_MEM_BaseWithHashId<true> : public EMSA2HashIdLookup<PSSR_MEM_Base> {};
template<> class PSSR_MEM_BaseWithHashId<false> : public PSSR_MEM_Base {};

template <bool ALLOW_RECOVERY, class MGF=P1363_MGF1, int SALT_LEN=-1, int MIN_PAD_LEN=0, bool USE_HASH_ID=false>
class PSSR_MEM : public PSSR_MEM_BaseWithHashId<USE_HASH_ID>
{
	virtual bool AllowRecovery() const {return ALLOW_RECOVERY;}
	virtual size_t SaltLen(size_t hashLen) const {return SALT_LEN < 0 ? hashLen : SALT_LEN;}
	virtual size_t MinPadLen(size_t hashLen) const {return MIN_PAD_LEN < 0 ? hashLen : MIN_PAD_LEN;}
	virtual const MaskGeneratingFunction & GetMGF() const {static MGF mgf; return mgf;}

public:
	static std::string CRYPTOPP_API StaticAlgorithmName() {return std::string(ALLOW_RECOVERY ? "PSSR-" : "PSS-") + MGF::StaticAlgorithmName();}
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#sem_PSSR-MGF1">PSSR-MGF1</a>
struct PSSR : public SignatureStandard
{
	typedef PSSR_MEM<true> SignatureMessageEncodingMethod;
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#sem_PSS-MGF1">PSS-MGF1</a>
struct PSS : public SignatureStandard
{
	typedef PSSR_MEM<false> SignatureMessageEncodingMethod;
};

NAMESPACE_END

#endif
