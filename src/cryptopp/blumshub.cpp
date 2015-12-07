// blumshub.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "blumshub.h"
#include "integer.h"

NAMESPACE_BEGIN(CryptoPP)

PublicBlumBlumShub::PublicBlumBlumShub(const Integer &n, const Integer &seed)
	: modn(n),
	  current(modn.Square(modn.Square(seed))),
	  maxBits(BitPrecision(n.BitCount())-1),
	  bitsLeft(maxBits)
{
}

unsigned int PublicBlumBlumShub::GenerateBit()
{
	if (bitsLeft==0)
	{
		current = modn.Square(current);
		bitsLeft = maxBits;
	}

	return current.GetBit(--bitsLeft);
}

byte PublicBlumBlumShub::GenerateByte()
{
	byte b=0;
	for (int i=0; i<8; i++)
		b = byte((b << 1) | PublicBlumBlumShub::GenerateBit());
	return b;
}

void PublicBlumBlumShub::GenerateBlock(byte *output, size_t size)
{
	while (size--)
		*output++ = PublicBlumBlumShub::GenerateByte();
}

void PublicBlumBlumShub::ProcessData(byte *outString, const byte *inString, size_t length)
{
	while (length--)
		*outString++ = *inString++ ^ PublicBlumBlumShub::GenerateByte();
}

BlumBlumShub::BlumBlumShub(const Integer &p, const Integer &q, const Integer &seed)
	: PublicBlumBlumShub(p*q, seed),
	  p(p), q(q),
	  x0(modn.Square(seed))
{
}

void BlumBlumShub::Seek(lword index)
{
	Integer i(Integer::POSITIVE, index);
	i *= 8;
	Integer e = a_exp_b_mod_c (2, i / maxBits + 1, (p-1)*(q-1));
	current = modn.Exponentiate(x0, e);
	bitsLeft = maxBits - i % maxBits;
}

NAMESPACE_END
