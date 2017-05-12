// rng.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "rng.h"
#include "fips140.h"

#include <time.h>
#include <math.h>

NAMESPACE_BEGIN(CryptoPP)

// linear congruential generator
// originally by William S. England

// do not use for cryptographic purposes

/*
** Original_numbers are the original published m and q in the
** ACM article above.  John Burton has furnished numbers for
** a reportedly better generator.  The new numbers are now
** used in this program by default.
*/

#ifndef LCRNG_ORIGINAL_NUMBERS
const word32 LC_RNG::m=2147483647L;
const word32 LC_RNG::q=44488L;

const word16 LC_RNG::a=(unsigned int)48271L;
const word16 LC_RNG::r=3399;
#else
const word32 LC_RNG::m=2147483647L;
const word32 LC_RNG::q=127773L;

const word16 LC_RNG::a=16807;
const word16 LC_RNG::r=2836;
#endif

void LC_RNG::GenerateBlock(byte *output, size_t size)
{
	while (size--)
	{
		word32 hi = seed/q;
		word32 lo = seed%q;

		long test = a*lo - r*hi;

		if (test > 0)
			seed = test;
		else
			seed = test+ m;

		*output++ = byte((GETBYTE(seed, 0) ^ GETBYTE(seed, 1) ^ GETBYTE(seed, 2) ^ GETBYTE(seed, 3)));
	}
}

// ********************************************************

#ifndef CRYPTOPP_IMPORTS

X917RNG::X917RNG(BlockTransformation *c, const byte *seed, const byte *deterministicTimeVector)
	: m_cipher(c),
	  m_size(m_cipher->BlockSize()),
	  m_datetime(m_size),
	  m_randseed(seed, m_size),
	  m_lastBlock(m_size),
	  m_deterministicTimeVector(deterministicTimeVector, deterministicTimeVector ? m_size : 0)
{
	// Valgrind finding, http://github.com/weidai11/cryptopp/issues/105
	// Garbage in the tail creates a non-conforming X9.17 or X9.31 generator.
	if (m_size > 8)
	{
		memset(m_datetime, 0x00, m_size);
		memset(m_lastBlock, 0x00, m_size);
	}

	if (!deterministicTimeVector)
	{
		time_t tstamp1 = time(0);
		xorbuf(m_datetime, (byte *)&tstamp1, UnsignedMin(sizeof(tstamp1), m_size));
		m_cipher->ProcessBlock(m_datetime);
		clock_t tstamp2 = clock();
		xorbuf(m_datetime, (byte *)&tstamp2, UnsignedMin(sizeof(tstamp2), m_size));
		m_cipher->ProcessBlock(m_datetime);
	}

	// for FIPS 140-2
	GenerateBlock(m_lastBlock, m_size);
}

void X917RNG::GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword size)
{
	while (size > 0)
	{
		// calculate new enciphered timestamp
		if (m_deterministicTimeVector.size())
		{
			m_cipher->ProcessBlock(m_deterministicTimeVector, m_datetime);
			IncrementCounterByOne(m_deterministicTimeVector, m_size);
		}
		else
		{
			clock_t c = clock();
			xorbuf(m_datetime, (byte *)&c, UnsignedMin(sizeof(c), m_size));
			time_t t = time(NULL);
			xorbuf(m_datetime+m_size-UnsignedMin(sizeof(t), m_size), (byte *)&t, UnsignedMin(sizeof(t), m_size));
			m_cipher->ProcessBlock(m_datetime);
		}

		// combine enciphered timestamp with seed
		xorbuf(m_randseed, m_datetime, m_size);

		// generate a new block of random bytes
		m_cipher->ProcessBlock(m_randseed);
		if (memcmp(m_lastBlock, m_randseed, m_size) == 0)
			throw SelfTestFailure("X917RNG: Continuous random number generator test failed.");

		// output random bytes
		size_t len = UnsignedMin(m_size, size);
		target.ChannelPut(channel, m_randseed, len);
		size -= len;

		// compute new seed vector
		memcpy(m_lastBlock, m_randseed, m_size);
		xorbuf(m_randseed, m_datetime, m_size);
		m_cipher->ProcessBlock(m_randseed);
	}
}

#endif

MaurerRandomnessTest::MaurerRandomnessTest()
	: sum(0.0), n(0)
{
	for (unsigned i=0; i<V; i++)
		tab[i] = 0;
}

size_t MaurerRandomnessTest::Put2(const byte *inString, size_t length, int /*messageEnd*/, bool /*blocking*/)
{
	while (length--)
	{
		byte inByte = *inString++;
		if (n >= Q)
			sum += log(double(n - tab[inByte]));
		tab[inByte] = n;
		n++;
	}
	return 0;
}

double MaurerRandomnessTest::GetTestValue() const
{
	if (BytesNeeded() > 0)
		throw Exception(Exception::OTHER_ERROR, "MaurerRandomnessTest: " + IntToString(BytesNeeded()) + " more bytes of input needed");

	double fTu = (sum/(n-Q))/log(2.0);	// this is the test value defined by Maurer

	double value = fTu * 0.1392;		// arbitrarily normalize it to
	return value > 1.0 ? 1.0 : value;	// a number between 0 and 1
}

NAMESPACE_END
