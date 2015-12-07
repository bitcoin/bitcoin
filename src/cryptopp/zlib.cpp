// zlib.cpp - written and placed in the public domain by Wei Dai

// "zlib" is the name of a well known C language compression library
// (http://www.zlib.org) and also the name of a compression format
// (RFC 1950) that the library implements. This file is part of a
// complete reimplementation of the zlib compression format.

#include "pch.h"
#include "zlib.h"
#include "zdeflate.h"
#include "zinflate.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

static const byte DEFLATE_METHOD = 8;
static const byte FDICT_FLAG = (1 << 5);

// *************************************************************

void ZlibCompressor::WritePrestreamHeader()
{
	m_adler32.Restart();
	assert(((GetLog2WindowSize()-8) << 4) <= 255);
	byte cmf = byte(DEFLATE_METHOD | ((GetLog2WindowSize()-8) << 4));
	assert((GetCompressionLevel() << 6) <= 255);
	byte flags = byte(GetCompressionLevel() << 6);
	AttachedTransformation()->PutWord16(RoundUpToMultipleOf(word16(cmf*256+flags), word16(31)));
}

void ZlibCompressor::ProcessUncompressedData(const byte *inString, size_t length)
{
	m_adler32.Update(inString, length);
}

void ZlibCompressor::WritePoststreamTail()
{
	FixedSizeSecBlock<byte, 4> adler32;
	m_adler32.Final(adler32);
	AttachedTransformation()->Put(adler32, 4);
}

unsigned int ZlibCompressor::GetCompressionLevel() const
{
	static const unsigned int deflateToCompressionLevel[] = {0, 1, 1, 1, 2, 2, 2, 2, 2, 3};
	return deflateToCompressionLevel[GetDeflateLevel()];
}

// *************************************************************

ZlibDecompressor::ZlibDecompressor(BufferedTransformation *attachment, bool repeat, int propagation)
	: Inflator(attachment, repeat, propagation), m_log2WindowSize(0)
{
}

void ZlibDecompressor::ProcessPrestreamHeader()
{
	m_adler32.Restart();

	byte cmf;
	byte flags;

	if (!m_inQueue.Get(cmf) || !m_inQueue.Get(flags))
		throw HeaderErr();

	if ((cmf*256+flags) % 31 != 0)
		throw HeaderErr();	// if you hit this exception, you're probably trying to decompress invalid data

	if ((cmf & 0xf) != DEFLATE_METHOD)
		throw UnsupportedAlgorithm();

	if (flags & FDICT_FLAG)
		throw UnsupportedPresetDictionary();

	m_log2WindowSize = 8 + (cmf >> 4);
}

void ZlibDecompressor::ProcessDecompressedData(const byte *inString, size_t length)
{
	AttachedTransformation()->Put(inString, length);
	m_adler32.Update(inString, length);
}

void ZlibDecompressor::ProcessPoststreamTail()
{
	FixedSizeSecBlock<byte, 4> adler32;
	if (m_inQueue.Get(adler32, 4) != 4)
		throw Adler32Err();
	if (!m_adler32.Verify(adler32))
		throw Adler32Err();
}

NAMESPACE_END
