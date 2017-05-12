// cmac.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "cmac.h"

NAMESPACE_BEGIN(CryptoPP)

static void MulU(byte *k, unsigned int length)
{
	byte carry = 0;

	for (int i=length-1; i>=1; i-=2)
	{
		byte carry2 = k[i] >> 7;
		k[i] += k[i] + carry;
		carry = k[i-1] >> 7;
		k[i-1] += k[i-1] + carry2;
	}

	if (carry)
	{
		switch (length)
		{
		case 8:
			k[7] ^= 0x1b;
			break;
		case 16:
			k[15] ^= 0x87;
			break;
		case 32:
			k[30] ^= 4;
			k[31] ^= 0x23;
			break;
		default:
			throw InvalidArgument("CMAC: " + IntToString(length) + " is not a supported cipher block size");
		}
	}
}

void CMAC_Base::UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
{
	BlockCipher &cipher = AccessCipher();
	unsigned int blockSize = cipher.BlockSize();

	cipher.SetKey(key, length, params);
	m_reg.CleanNew(3*blockSize);
	m_counter = 0;

	cipher.ProcessBlock(m_reg, m_reg+blockSize);
	MulU(m_reg+blockSize, blockSize);
	memcpy(m_reg+2*blockSize, m_reg+blockSize, blockSize);
	MulU(m_reg+2*blockSize, blockSize);
}

void CMAC_Base::Update(const byte *input, size_t length)
{
	CRYPTOPP_ASSERT((input && length) || !(input || length));
	if (!length)
		return;

	BlockCipher &cipher = AccessCipher();
	unsigned int blockSize = cipher.BlockSize();

	if (m_counter > 0)
	{
		const unsigned int len = UnsignedMin(blockSize - m_counter, length);
		if (len)
		{
			xorbuf(m_reg+m_counter, input, len);
			length -= len;
			input += len;
			m_counter += len;
		}

		if (m_counter == blockSize && length > 0)
		{
			cipher.ProcessBlock(m_reg);
			m_counter = 0;
		}
	}

	if (length > blockSize)
	{
		CRYPTOPP_ASSERT(m_counter == 0);
		size_t leftOver = 1 + cipher.AdvancedProcessBlocks(m_reg, input, m_reg, length-1, BlockTransformation::BT_DontIncrementInOutPointers|BlockTransformation::BT_XorInput);
		input += (length - leftOver);
		length = leftOver;
	}

	if (length > 0)
	{
		CRYPTOPP_ASSERT(m_counter + length <= blockSize);
		xorbuf(m_reg+m_counter, input, length);
		m_counter += (unsigned int)length;
	}

	CRYPTOPP_ASSERT(m_counter > 0);
}

void CMAC_Base::TruncatedFinal(byte *mac, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	BlockCipher &cipher = AccessCipher();
	unsigned int blockSize = cipher.BlockSize();

	if (m_counter < blockSize)
	{
		m_reg[m_counter] ^= 0x80;
		cipher.AdvancedProcessBlocks(m_reg, m_reg+2*blockSize, m_reg, blockSize, BlockTransformation::BT_DontIncrementInOutPointers|BlockTransformation::BT_XorInput);
	}
	else
		cipher.AdvancedProcessBlocks(m_reg, m_reg+blockSize, m_reg, blockSize, BlockTransformation::BT_DontIncrementInOutPointers|BlockTransformation::BT_XorInput);

	memcpy(mac, m_reg, size);

	m_counter = 0;
	memset(m_reg, 0, blockSize);
}

NAMESPACE_END

#endif
