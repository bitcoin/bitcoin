// lubyrack.h - written and placed in the public domain by Wei Dai

//! \file lubyrack.h
//! \brief Classes for the Luby-Rackoff block cipher

#ifndef CRYPTOPP_LUBYRACK_H
#define CRYPTOPP_LUBYRACK_H

#include "simple.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

template <class T> struct DigestSizeDoubleWorkaround 	// VC60 workaround
{
	CRYPTOPP_CONSTANT(RESULT = 2*T::DIGESTSIZE)
};

//! \class LR_Info
//! \brief Luby-Rackoff block cipher information
template <class T>
struct LR_Info : public VariableKeyLength<16, 0, 2*(INT_MAX/2), 2>, public FixedBlockSize<DigestSizeDoubleWorkaround<T>::RESULT>
{
	static std::string StaticAlgorithmName() {return std::string("LR/")+T::StaticAlgorithmName();}
};

//! \class LR
//! \brief Luby-Rackoff block cipher
template <class T>
class LR : public LR_Info<T>, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<LR_Info<T> >
	{
	public:
		// VC60 workaround: have to define these functions within class definition
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params)
		{
			this->AssertValidKeyLength(length);

			L = length/2;
			buffer.New(2*S);
			digest.New(S);
			key.Assign(userKey, 2*L);
		}

	protected:
		CRYPTOPP_CONSTANT(S=T::DIGESTSIZE)
		unsigned int L;	// key length / 2
		SecByteBlock key;

		mutable T hm;
		mutable SecByteBlock buffer, digest;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:

#define KL this->key
#define KR this->key+this->L
#define BL this->buffer
#define BR this->buffer+this->S
#define IL inBlock
#define IR inBlock+this->S
#define OL outBlock
#define OR outBlock+this->S

		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
		{
			this->hm.Update(KL, this->L);
			this->hm.Update(IL, this->S);
			this->hm.Final(BR);
			xorbuf(BR, IR, this->S);

			this->hm.Update(KR, this->L);
			this->hm.Update(BR, this->S);
			this->hm.Final(BL);
			xorbuf(BL, IL, this->S);

			this->hm.Update(KL, this->L);
			this->hm.Update(BL, this->S);
			this->hm.Final(this->digest);
			xorbuf(BR, this->digest, this->S);

			this->hm.Update(KR, this->L);
			this->hm.Update(OR, this->S);
			this->hm.Final(this->digest);
			xorbuf(BL, this->digest, this->S);

			if (xorBlock)
				xorbuf(outBlock, xorBlock, this->buffer, 2*this->S);
			else
				memcpy_s(outBlock, 2*this->S, this->buffer, 2*this->S);
		}
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
		{
			this->hm.Update(KR, this->L);
			this->hm.Update(IR, this->S);
			this->hm.Final(BL);
			xorbuf(BL, IL, this->S);

			this->hm.Update(KL, this->L);
			this->hm.Update(BL, this->S);
			this->hm.Final(BR);
			xorbuf(BR, IR, this->S);

			this->hm.Update(KR, this->L);
			this->hm.Update(BR, this->S);
			this->hm.Final(this->digest);
			xorbuf(BL, this->digest, this->S);

			this->hm.Update(KL, this->L);
			this->hm.Update(OL, this->S);
			this->hm.Final(this->digest);
			xorbuf(BR, this->digest, this->S);

			if (xorBlock)
				xorbuf(outBlock, xorBlock, this->buffer, 2*this->S);
			else
				memcpy(outBlock, this->buffer, 2*this->S);
		}
#undef KL
#undef KR
#undef BL
#undef BR
#undef IL
#undef IR
#undef OL
#undef OR
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

NAMESPACE_END

#endif
