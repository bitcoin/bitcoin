// mdc.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_MDC_H
#define CRYPTOPP_MDC_H

//! \file mdc.h
//! \brief Classes for the MDC message digest

#include "seckey.h"
#include "secblock.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class MDC_Info
//! \brief MDC_Info cipher information
template <class T>
struct MDC_Info : public FixedBlockSize<T::DIGESTSIZE>, public FixedKeyLength<T::BLOCKSIZE>
{
	static std::string StaticAlgorithmName() {return std::string("MDC/")+T::StaticAlgorithmName();}
};


//! \class MDC
//! \brief MDC cipher
//! \details MDC() is a construction by Peter Gutmann to turn an iterated hash function into a PRF
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#MDC">MDC</a>
template <class T>
class MDC : public MDC_Info<T>
{
	//! \class Enc
	//! \brief MDC cipher encryption operation
	class CRYPTOPP_NO_VTABLE Enc : public BlockCipherImpl<MDC_Info<T> >
	{
		typedef typename T::HashWordType HashWordType;

	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params)
		{
			this->AssertValidKeyLength(length);
			memcpy_s(m_key, m_key.size(), userKey, this->KEYLENGTH);
			T::CorrectEndianess(Key(), Key(), this->KEYLENGTH);
		}

		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
		{
			T::CorrectEndianess(Buffer(), (HashWordType *)inBlock, this->BLOCKSIZE);
			T::Transform(Buffer(), Key());
			if (xorBlock)
			{
				T::CorrectEndianess(Buffer(), Buffer(), this->BLOCKSIZE);
				xorbuf(outBlock, xorBlock, m_buffer, this->BLOCKSIZE);
			}
			else
				T::CorrectEndianess((HashWordType *)outBlock, Buffer(), this->BLOCKSIZE);
		}

		bool IsPermutation() const {return false;}

		unsigned int OptimalDataAlignment() const {return sizeof(HashWordType);}

	private:
		HashWordType *Key() {return (HashWordType *)m_key.data();}
		const HashWordType *Key() const {return (const HashWordType *)m_key.data();}
		HashWordType *Buffer() const {return (HashWordType *)m_buffer.data();}

		// VC60 workaround: bug triggered if using FixedSizeAllocatorWithCleanup
		FixedSizeSecBlock<byte, MDC_Info<T>::KEYLENGTH, AllocatorWithCleanup<byte> > m_key;
		mutable FixedSizeSecBlock<byte, MDC_Info<T>::BLOCKSIZE, AllocatorWithCleanup<byte> > m_buffer;
	};

public:
	//! use BlockCipher interface
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
};

NAMESPACE_END

#endif
