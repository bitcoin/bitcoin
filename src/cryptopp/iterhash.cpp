// iterhash.cpp - written and placed in the public domain by Wei Dai

#ifndef __GNUC__
#define CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#endif

#include "iterhash.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

template <class T, class BASE> void IteratedHashBase<T, BASE>::Update(const byte *input, size_t len)
{
	HashWordType oldCountLo = m_countLo, oldCountHi = m_countHi;
	if ((m_countLo = oldCountLo + HashWordType(len)) < oldCountLo)
		m_countHi++;             // carry from low to high
	m_countHi += (HashWordType)SafeRightShift<8*sizeof(HashWordType)>(len);
	if (m_countHi < oldCountHi || SafeRightShift<2*8*sizeof(HashWordType)>(len) != 0)
		throw HashInputTooLong(this->AlgorithmName());

	const unsigned int blockSize = this->BlockSize();
	unsigned int num = ModPowerOf2(oldCountLo, blockSize);

	T* dataBuf = this->DataBuf();
	byte* data = (byte *)dataBuf;
	CRYPTOPP_ASSERT(dataBuf && data);

	if (num != 0)	// process left over data
	{
		if (num+len >= blockSize)
		{
			if (data && input) {memcpy(data+num, input, blockSize-num);}
			HashBlock(dataBuf);
			input += (blockSize-num);
			len -= (blockSize-num);
			num = 0;
			// drop through and do the rest
		}
		else
		{
			if (data && input && len) {memcpy(data+num, input, len);}
			return;
		}
	}

	// now process the input data in blocks of blockSize bytes and save the leftovers to m_data
	if (len >= blockSize)
	{
		if (input == data)
		{
			CRYPTOPP_ASSERT(len == blockSize);
			HashBlock(dataBuf);
			return;
		}
		else if (IsAligned<T>(input))
		{
			size_t leftOver = HashMultipleBlocks((T *)(void*)input, len);
			input += (len - leftOver);
			len = leftOver;
		}
		else
			do
			{   // copy input first if it's not aligned correctly
				if (data && input) memcpy(data, input, blockSize);
				HashBlock(dataBuf);
				input+=blockSize;
				len-=blockSize;
			} while (len >= blockSize);
	}

	if (data && input && len && data != input)
		memcpy(data, input, len);
}

template <class T, class BASE> byte * IteratedHashBase<T, BASE>::CreateUpdateSpace(size_t &size)
{
	unsigned int blockSize = this->BlockSize();
	unsigned int num = ModPowerOf2(m_countLo, blockSize);
	size = blockSize - num;
	return (byte *)DataBuf() + num;
}

template <class T, class BASE> size_t IteratedHashBase<T, BASE>::HashMultipleBlocks(const T *input, size_t length)
{
	unsigned int blockSize = this->BlockSize();
	bool noReverse = NativeByteOrderIs(this->GetByteOrder());
	T* dataBuf = this->DataBuf();
	do
	{
		if (noReverse)
			this->HashEndianCorrectedBlock(input);
		else
		{
			ByteReverse(dataBuf, input, this->BlockSize());
			this->HashEndianCorrectedBlock(dataBuf);
		}

		input += blockSize/sizeof(T);
		length -= blockSize;
	}
	while (length >= blockSize);
	return length;
}

template <class T, class BASE> void IteratedHashBase<T, BASE>::PadLastBlock(unsigned int lastBlockSize, byte padFirst)
{
	unsigned int blockSize = this->BlockSize();
	unsigned int num = ModPowerOf2(m_countLo, blockSize);
	T* dataBuf = this->DataBuf();
	byte* data = (byte *)dataBuf;
	data[num++] = padFirst;
	if (num <= lastBlockSize)
		memset(data+num, 0, lastBlockSize-num);
	else
	{
		memset(data+num, 0, blockSize-num);
		HashBlock(dataBuf);
		memset(data, 0, lastBlockSize);
	}
}

template <class T, class BASE> void IteratedHashBase<T, BASE>::Restart()
{
	m_countLo = m_countHi = 0;
	Init();
}

template <class T, class BASE> void IteratedHashBase<T, BASE>::TruncatedFinal(byte *digest, size_t size)
{
	this->ThrowIfInvalidTruncatedSize(size);

	T* dataBuf = this->DataBuf();
	T* stateBuf = this->StateBuf();
	unsigned int blockSize = this->BlockSize();
	ByteOrder order = this->GetByteOrder();

	PadLastBlock(blockSize - 2*sizeof(HashWordType));
	dataBuf[blockSize/sizeof(T)-2+order] = ConditionalByteReverse(order, this->GetBitCountLo());
	dataBuf[blockSize/sizeof(T)-1-order] = ConditionalByteReverse(order, this->GetBitCountHi());

	HashBlock(dataBuf);

	if (IsAligned<HashWordType>(digest) && size%sizeof(HashWordType)==0)
		ConditionalByteReverse<HashWordType>(order, (HashWordType *)(void*)digest, stateBuf, size);
	else
	{
		ConditionalByteReverse<HashWordType>(order, stateBuf, stateBuf, this->DigestSize());
		memcpy(digest, stateBuf, size);
	}

	this->Restart();		// reinit for next use
}

#ifdef __GNUC__
	template class IteratedHashBase<word64, HashTransformation>;
	template class IteratedHashBase<word64, MessageAuthenticationCode>;

	template class IteratedHashBase<word32, HashTransformation>;
	template class IteratedHashBase<word32, MessageAuthenticationCode>;
#endif

NAMESPACE_END
