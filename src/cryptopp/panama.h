// panama.h - written and placed in the public domain by Wei Dai

//! \file panama.h
//! \brief Classes for Panama stream cipher

#ifndef CRYPTOPP_PANAMA_H
#define CRYPTOPP_PANAMA_H

#include "strciphr.h"
#include "iterhash.h"
#include "secblock.h"

#if CRYPTOPP_BOOL_X32
# define CRYPTOPP_DISABLE_PANAMA_ASM
#endif

NAMESPACE_BEGIN(CryptoPP)

/// base class, do not use directly
template <class B>
class CRYPTOPP_NO_VTABLE Panama
{
public:
	void Reset();
	void Iterate(size_t count, const word32 *p=NULL, byte *output=NULL, const byte *input=NULL, KeystreamOperation operation=WRITE_KEYSTREAM);

protected:
	typedef word32 Stage[8];
	CRYPTOPP_CONSTANT(STAGES = 32)

	FixedSizeAlignedSecBlock<word32, 20 + 8*32> m_state;
};

namespace Weak {
/// <a href="http://www.weidai.com/scan-mirror/md.html#Panama">Panama Hash</a>
template <class B = LittleEndian>
class PanamaHash : protected Panama<B>, public AlgorithmImpl<IteratedHash<word32, NativeByteOrder, 32>, PanamaHash<B> >
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = 32)
	PanamaHash() {Panama<B>::Reset();}
	unsigned int DigestSize() const {return DIGESTSIZE;}
	void TruncatedFinal(byte *hash, size_t size);
	static const char * StaticAlgorithmName() {return B::ToEnum() == BIG_ENDIAN_ORDER ? "Panama-BE" : "Panama-LE";}

protected:
	void Init() {Panama<B>::Reset();}
	void HashEndianCorrectedBlock(const word32 *data) {this->Iterate(1, data);}	// push
	size_t HashMultipleBlocks(const word32 *input, size_t length);
	word32* StateBuf() {return NULL;}
};
}

//! MAC construction using a hermetic hash function
template <class T_Hash, class T_Info = T_Hash>
class HermeticHashFunctionMAC : public AlgorithmImpl<SimpleKeyingInterfaceImpl<TwoBases<MessageAuthenticationCode, VariableKeyLength<32, 0, INT_MAX> > >, T_Info>
{
public:
	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
	{
		CRYPTOPP_UNUSED(params);

		m_key.Assign(key, length);
		Restart();
	}

	void Restart()
	{
		m_hash.Restart();
		m_keyed = false;
	}

	void Update(const byte *input, size_t length)
	{
		if (!m_keyed)
			KeyHash();
		m_hash.Update(input, length);
	}

	void TruncatedFinal(byte *digest, size_t digestSize)
	{
		if (!m_keyed)
			KeyHash();
		m_hash.TruncatedFinal(digest, digestSize);
		m_keyed = false;
	}

	unsigned int DigestSize() const
		{return m_hash.DigestSize();}
	unsigned int BlockSize() const
		{return m_hash.BlockSize();}
	unsigned int OptimalBlockSize() const
		{return m_hash.OptimalBlockSize();}
	unsigned int OptimalDataAlignment() const
		{return m_hash.OptimalDataAlignment();}

protected:
	void KeyHash()
	{
		m_hash.Update(m_key, m_key.size());
		m_keyed = true;
	}

	T_Hash m_hash;
	bool m_keyed;
	SecByteBlock m_key;
};

namespace Weak {
/// Panama MAC
template <class B = LittleEndian>
class PanamaMAC : public HermeticHashFunctionMAC<PanamaHash<B> >
{
public:
 	PanamaMAC() {}
	PanamaMAC(const byte *key, unsigned int length)
		{this->SetKey(key, length);}
};
}

//! algorithm info
template <class B>
struct PanamaCipherInfo : public FixedKeyLength<32, SimpleKeyingInterface::UNIQUE_IV, 32>
{
	static const char * StaticAlgorithmName() {return B::ToEnum() == BIG_ENDIAN_ORDER ? "Panama-BE" : "Panama-LE";}
};

//! _
template <class B>
class PanamaCipherPolicy : public AdditiveCipherConcretePolicy<word32, 8>, 
							public PanamaCipherInfo<B>,
							protected Panama<B>
{
protected:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount);
	bool CipherIsRandomAccess() const {return false;}
	void CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length);
	unsigned int GetAlignment() const;

	FixedSizeSecBlock<word32, 8> m_key;
};

//! <a href="http://www.cryptolounge.org/wiki/PANAMA">Panama Stream Cipher</a>
template <class B = LittleEndian>
struct PanamaCipher : public PanamaCipherInfo<B>, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<PanamaCipherPolicy<B>, AdditiveCipherTemplate<> >, PanamaCipherInfo<B> > Encryption;
	typedef Encryption Decryption;
};

NAMESPACE_END

#endif
