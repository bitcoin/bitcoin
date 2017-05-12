// adler32.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "adler32.h"

NAMESPACE_BEGIN(CryptoPP)

void Adler32::Update(const byte *input, size_t length)
{
	const unsigned long BASE = 65521;

	unsigned long s1 = m_s1;
	unsigned long s2 = m_s2;

	if (length % 8 != 0)
	{
		do
		{
			s1 += *input++;
			s2 += s1;
			length--;
		} while (length % 8 != 0);

		if (s1 >= BASE)
			s1 -= BASE;
		s2 %= BASE;
	}

	while (length > 0)
	{
		s1 += input[0]; s2 += s1;
		s1 += input[1]; s2 += s1;
		s1 += input[2]; s2 += s1;
		s1 += input[3]; s2 += s1;
		s1 += input[4]; s2 += s1;
		s1 += input[5]; s2 += s1;
		s1 += input[6]; s2 += s1;
		s1 += input[7]; s2 += s1;

		length -= 8;
		input += 8;

		if (s1 >= BASE)
			s1 -= BASE;
		if (length % 0x8000 == 0)
			s2 %= BASE;
	}

	CRYPTOPP_ASSERT(s1 < BASE);
	CRYPTOPP_ASSERT(s2 < BASE);

	m_s1 = (word16)s1;
	m_s2 = (word16)s2;
}

void Adler32::TruncatedFinal(byte *hash, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	switch (size)
	{
	default:
		hash[3] = byte(m_s1);
		// fall through
	case 3:
		hash[2] = byte(m_s1 >> 8);
		// fall through
	case 2:
		hash[1] = byte(m_s2);
		// fall through
	case 1:
		hash[0] = byte(m_s2 >> 8);
		// fall through
	case 0:
		;;
		// fall through
	}

	Reset();
}

NAMESPACE_END
