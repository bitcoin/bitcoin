// ccm.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "ccm.h"

NAMESPACE_BEGIN(CryptoPP)

void CCM_Base::SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params)
{
	BlockCipher &blockCipher = AccessBlockCipher();

	blockCipher.SetKey(userKey, keylength, params);

	if (blockCipher.BlockSize() != REQUIRED_BLOCKSIZE)
		throw InvalidArgument(AlgorithmName() + ": block size of underlying block cipher is not 16");

	m_digestSize = params.GetIntValueWithDefault(Name::DigestSize(), DefaultDigestSize());
	if (m_digestSize % 2 > 0 || m_digestSize < 4 || m_digestSize > 16)
		throw InvalidArgument(AlgorithmName() + ": DigestSize must be 4, 6, 8, 10, 12, 14, or 16");

	m_buffer.Grow(2*REQUIRED_BLOCKSIZE);
	m_L = 8;
}

void CCM_Base::Resync(const byte *iv, size_t len)
{
	BlockCipher &cipher = AccessBlockCipher();

	m_L = REQUIRED_BLOCKSIZE-1-(int)len;
	CRYPTOPP_ASSERT(m_L >= 2);
	if (m_L > 8)
		m_L = 8;

	m_buffer[0] = byte(m_L-1);	// flag
	memcpy(m_buffer+1, iv, len);
	memset(m_buffer+1+len, 0, REQUIRED_BLOCKSIZE-1-len);

	if (m_state >= State_IVSet)
		m_ctr.Resynchronize(m_buffer, REQUIRED_BLOCKSIZE);
	else
		m_ctr.SetCipherWithIV(cipher, m_buffer);

	m_ctr.Seek(REQUIRED_BLOCKSIZE);
	m_aadLength = 0;
	m_messageLength = 0;
}

void CCM_Base::UncheckedSpecifyDataLengths(lword headerLength, lword messageLength, lword /*footerLength*/)
{
	if (m_state != State_IVSet)
		throw BadState(AlgorithmName(), "SpecifyDataLengths", "or after State_IVSet");

	m_aadLength = headerLength;
	m_messageLength = messageLength;

	byte *cbcBuffer = CBC_Buffer();
	const BlockCipher &cipher = GetBlockCipher();

	cbcBuffer[0] = byte(64*(headerLength>0) + 8*((m_digestSize-2)/2) + (m_L-1));	// flag
	PutWord<word64>(true, BIG_ENDIAN_ORDER, cbcBuffer+REQUIRED_BLOCKSIZE-8, m_messageLength);
	memcpy(cbcBuffer+1, m_buffer+1, REQUIRED_BLOCKSIZE-1-m_L);
	cipher.ProcessBlock(cbcBuffer);

	if (headerLength>0)
	{
		CRYPTOPP_ASSERT(m_bufferedDataLength == 0);

		if (headerLength < ((1<<16) - (1<<8)))
		{
			PutWord<word16>(true, BIG_ENDIAN_ORDER, m_buffer, (word16)headerLength);
			m_bufferedDataLength = 2;
		}
		else if (headerLength < (W64LIT(1)<<32))
		{
			m_buffer[0] = 0xff;
			m_buffer[1] = 0xfe;
			PutWord<word32>(false, BIG_ENDIAN_ORDER, m_buffer+2, (word32)headerLength);
			m_bufferedDataLength = 6;
		}
		else
		{
			m_buffer[0] = 0xff;
			m_buffer[1] = 0xff;
			PutWord<word64>(false, BIG_ENDIAN_ORDER, m_buffer+2, headerLength);
			m_bufferedDataLength = 10;
		}
	}
}

size_t CCM_Base::AuthenticateBlocks(const byte *data, size_t len)
{
	byte *cbcBuffer = CBC_Buffer();
	const BlockCipher &cipher = GetBlockCipher();
	return cipher.AdvancedProcessBlocks(cbcBuffer, data, cbcBuffer, len, BlockTransformation::BT_DontIncrementInOutPointers|BlockTransformation::BT_XorInput);
}

void CCM_Base::AuthenticateLastHeaderBlock()
{
	byte *cbcBuffer = CBC_Buffer();
	const BlockCipher &cipher = GetBlockCipher();

	if (m_aadLength != m_totalHeaderLength)
		throw InvalidArgument(AlgorithmName() + ": header length doesn't match that given in SpecifyDataLengths");

	if (m_bufferedDataLength > 0)
	{
		xorbuf(cbcBuffer, m_buffer, m_bufferedDataLength);
		cipher.ProcessBlock(cbcBuffer);
		m_bufferedDataLength = 0;
	}
}

void CCM_Base::AuthenticateLastConfidentialBlock()
{
	byte *cbcBuffer = CBC_Buffer();
	const BlockCipher &cipher = GetBlockCipher();

	if (m_messageLength != m_totalMessageLength)
		throw InvalidArgument(AlgorithmName() + ": message length doesn't match that given in SpecifyDataLengths");

	if (m_bufferedDataLength > 0)
	{
		xorbuf(cbcBuffer, m_buffer, m_bufferedDataLength);
		cipher.ProcessBlock(cbcBuffer);
		m_bufferedDataLength = 0;
	}
}

void CCM_Base::AuthenticateLastFooterBlock(byte *mac, size_t macSize)
{
	m_ctr.Seek(0);
	m_ctr.ProcessData(mac, CBC_Buffer(), macSize);
}

NAMESPACE_END

#endif
