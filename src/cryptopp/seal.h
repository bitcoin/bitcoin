// seal.h - written and placed in the public domain by Wei Dai

//! \file seal.h
//! \brief Classes for SEAL stream cipher

#ifndef CRYPTOPP_SEAL_H
#define CRYPTOPP_SEAL_H

#include "strciphr.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class SEAL_Info
//! \brief SEAL stream cipher information
//! \tparam B Endianess of the stream cipher
template <class B = BigEndian>
struct SEAL_Info : public FixedKeyLength<20, SimpleKeyingInterface::INTERNALLY_GENERATED_IV, 4>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return B::ToEnum() == LITTLE_ENDIAN_ORDER ? "SEAL-3.0-LE" : "SEAL-3.0-BE";}
};

//! \class SEAL_Policy
//! \brief SEAL stream cipher operation
//! \tparam B Endianess of the stream cipher
template <class B = BigEndian>
class CRYPTOPP_NO_VTABLE SEAL_Policy : public AdditiveCipherConcretePolicy<word32, 256>, public SEAL_Info<B>
{
protected:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount);
	void CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length);
	bool CipherIsRandomAccess() const {return true;}
	void SeekToIteration(lword iterationCount);

private:
	FixedSizeSecBlock<word32, 512> m_T;
	FixedSizeSecBlock<word32, 256> m_S;
	SecBlock<word32> m_R;

	word32 m_startCount, m_iterationsPerCount;
	word32 m_outsideCounter, m_insideCounter;
};

//! \class SEAL
//! \brief SEAL stream cipher
//! \tparam B Endianess of the stream cipher
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#SEAL-3.0-BE">SEAL</a>
template <class B = BigEndian>
struct SEAL : public SEAL_Info<B>, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<SEAL_Policy<B>, AdditiveCipherTemplate<> >, SEAL_Info<B> > Encryption;
	typedef Encryption Decryption;
};

NAMESPACE_END

#endif
