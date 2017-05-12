// pssr.h - written and placed in the public domain by Wei Dai

//! \file pssr.h
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

//! \brief PSSR Message Encoding Method interface
class CRYPTOPP_DLL PSSR_MEM_Base : public PK_RecoverableSignatureMessageEncodingMethod
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PSSR_MEM_Base() {}
#endif

private:
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

//! \brief PSSR Message Encoding Method with Hash Identifier
//! \tparam USE_HASH_ID flag indicating whether the HashId is used
template <bool USE_HASH_ID> class PSSR_MEM_BaseWithHashId;

//! \brief PSSR Message Encoding Method with Hash Identifier
//! \tparam true flag indicating HashId is used
template<> class PSSR_MEM_BaseWithHashId<true> : public EMSA2HashIdLookup<PSSR_MEM_Base> {};

//! \brief PSSR Message Encoding Method without Hash Identifier
//! \tparam false flag indicating HashId is not used
template<> class PSSR_MEM_BaseWithHashId<false> : public PSSR_MEM_Base {};

//! \brief PSSR Message Encoding Method
//! \tparam ALLOW_RECOVERY flag indicating whether the scheme provides message recovery
//! \tparam MGF mask generation function
//! \tparam SALT_LEN length of the salt
//! \tparam MIN_PAD_LEN minimum length of the pad
//! \tparam USE_HASH_ID flag indicating whether the HashId is used
//! \details If ALLOW_RECOVERY is true, the the signature scheme provides message recovery. If
//!  ALLOW_RECOVERY is false, the the signature scheme is appendix, and the message must be
//!  provided during verification.
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

//! \brief Probabilistic Signature Scheme with Recovery
//! \details Signature Schemes with Recovery encode the message with the signature.
//! \sa <a href="http://www.weidai.com/scan-mirror/sig.html#sem_PSSR-MGF1">PSSR-MGF1</a>
struct PSSR : public SignatureStandard
{
	typedef PSSR_MEM<true> SignatureMessageEncodingMethod;
};

//! \brief Probabilistic Signature Scheme with Appendix
//! \details Signature Schemes with Appendix require the message to be provided during verification.
//! \sa <a href="http://www.weidai.com/scan-mirror/sig.html#sem_PSS-MGF1">PSS-MGF1</a>
struct PSS : public SignatureStandard
{
	typedef PSSR_MEM<false> SignatureMessageEncodingMethod;
};

NAMESPACE_END

#endif
