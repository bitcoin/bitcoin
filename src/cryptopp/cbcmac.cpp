#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "cbcmac.h"

NAMESPACE_BEGIN(CryptoPP)

void CBC_MAC_Base::UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
{
	AccessCipher().SetKey(key, length, params);
	m_reg.CleanNew(AccessCipher().BlockSize());
	m_counter = 0;
}

void CBC_MAC_Base::Update(const byte *input, size_t length)
{
	unsigned int blockSize = AccessCipher().BlockSize();

	while (m_counter && length)
	{
		m_reg[m_counter++] ^= *input++;
		if (m_counter == blockSize)
			ProcessBuf();
		length--;
	}

	if (length >= blockSize)
	{
		size_t leftOver = AccessCipher().AdvancedProcessBlocks(m_reg, input, m_reg, length, BlockTransformation::BT_DontIncrementInOutPointers|BlockTransformation::BT_XorInput);
		input += (length - leftOver);
		length = leftOver;
	}

	while (length--)
	{
		m_reg[m_counter++] ^= *input++;
		if (m_counter == blockSize)
			ProcessBuf();
	}
}

void CBC_MAC_Base::TruncatedFinal(byte *mac, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	if (m_counter)
		ProcessBuf();

	memcpy(mac, m_reg, size);
	memset(m_reg, 0, AccessCipher().BlockSize());
}

void CBC_MAC_Base::ProcessBuf()
{
	AccessCipher().ProcessBlock(m_reg);
	m_counter = 0;
}

NAMESPACE_END

#endif
