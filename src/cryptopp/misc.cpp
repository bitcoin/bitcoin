// misc.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4189)
# if (CRYPTOPP_MSC_VERSION >= 1400)
#  pragma warning(disable: 6237)
# endif
#endif

#ifndef CRYPTOPP_IMPORTS

#include "misc.h"
#include "words.h"
#include "words.h"
#include "stdcpp.h"
#include "integer.h"

// for memalign
#if defined(CRYPTOPP_MEMALIGN_AVAILABLE) || defined(CRYPTOPP_MM_MALLOC_AVAILABLE) || defined(QNX)
# include <malloc.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

void xorbuf(byte *buf, const byte *mask, size_t count)
{
	CRYPTOPP_ASSERT(buf != NULL);
	CRYPTOPP_ASSERT(mask != NULL);
	CRYPTOPP_ASSERT(count > 0);

	size_t i=0;
	if (IsAligned<word32>(buf) && IsAligned<word32>(mask))
	{
		if (!CRYPTOPP_BOOL_SLOW_WORD64 && IsAligned<word64>(buf) && IsAligned<word64>(mask))
		{
			for (i=0; i<count/8; i++)
				((word64*)(void*)buf)[i] ^= ((word64*)(void*)mask)[i];
			count -= 8*i;
			if (!count)
				return;
			buf += 8*i;
			mask += 8*i;
		}

		for (i=0; i<count/4; i++)
			((word32*)(void*)buf)[i] ^= ((word32*)(void*)mask)[i];
		count -= 4*i;
		if (!count)
			return;
		buf += 4*i;
		mask += 4*i;
	}

	for (i=0; i<count; i++)
		buf[i] ^= mask[i];
}

void xorbuf(byte *output, const byte *input, const byte *mask, size_t count)
{
	CRYPTOPP_ASSERT(output != NULL);
	CRYPTOPP_ASSERT(input != NULL);
	CRYPTOPP_ASSERT(count > 0);

	size_t i=0;
	if (IsAligned<word32>(output) && IsAligned<word32>(input) && IsAligned<word32>(mask))
	{
		if (!CRYPTOPP_BOOL_SLOW_WORD64 && IsAligned<word64>(output) && IsAligned<word64>(input) && IsAligned<word64>(mask))
		{
			for (i=0; i<count/8; i++)
				((word64*)(void*)output)[i] = ((word64*)(void*)input)[i] ^ ((word64*)(void*)mask)[i];
			count -= 8*i;
			if (!count)
				return;
			output += 8*i;
			input += 8*i;
			mask += 8*i;
		}

		for (i=0; i<count/4; i++)
			((word32*)(void*)output)[i] = ((word32*)(void*)input)[i] ^ ((word32*)(void*)mask)[i];
		count -= 4*i;
		if (!count)
			return;
		output += 4*i;
		input += 4*i;
		mask += 4*i;
	}

	for (i=0; i<count; i++)
		output[i] = input[i] ^ mask[i];
}

bool VerifyBufsEqual(const byte *buf, const byte *mask, size_t count)
{
	CRYPTOPP_ASSERT(buf != NULL);
	CRYPTOPP_ASSERT(mask != NULL);
	CRYPTOPP_ASSERT(count > 0);

	size_t i=0;
	byte acc8 = 0;

	if (IsAligned<word32>(buf) && IsAligned<word32>(mask))
	{
		word32 acc32 = 0;
		if (!CRYPTOPP_BOOL_SLOW_WORD64 && IsAligned<word64>(buf) && IsAligned<word64>(mask))
		{
			word64 acc64 = 0;
			for (i=0; i<count/8; i++)
				acc64 |= ((word64*)(void*)buf)[i] ^ ((word64*)(void*)mask)[i];
			count -= 8*i;
			if (!count)
				return acc64 == 0;
			buf += 8*i;
			mask += 8*i;
			acc32 = word32(acc64) | word32(acc64>>32);
		}

		for (i=0; i<count/4; i++)
			acc32 |= ((word32*)(void*)buf)[i] ^ ((word32*)(void*)mask)[i];
		count -= 4*i;
		if (!count)
			return acc32 == 0;
		buf += 4*i;
		mask += 4*i;
		acc8 = byte(acc32) | byte(acc32>>8) | byte(acc32>>16) | byte(acc32>>24);
	}

	for (i=0; i<count; i++)
		acc8 |= buf[i] ^ mask[i];
	return acc8 == 0;
}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
std::string StringNarrow(const wchar_t *str, bool throwOnError)
{
	CRYPTOPP_ASSERT(str);
	std::string result;

	// Safer functions on Windows for C&A, https://github.com/weidai11/cryptopp/issues/55
#if (CRYPTOPP_MSC_VERSION >= 1400)
	size_t len=0, size=0;
	errno_t err = 0;

	//const wchar_t* ptr = str;
	//while (*ptr++) len++;
	len = wcslen(str)+1;

	err = wcstombs_s(&size, NULL, 0, str, len*sizeof(wchar_t));
	CRYPTOPP_ASSERT(err == 0);
	if (err != 0) {goto CONVERSION_ERROR;}

	result.resize(size);
	err = wcstombs_s(&size, &result[0], size, str, len*sizeof(wchar_t));
	CRYPTOPP_ASSERT(err == 0);

	if (err != 0)
	{
CONVERSION_ERROR:
		if (throwOnError)
			throw InvalidArgument("StringNarrow: wcstombs_s() call failed with error " + IntToString(err));
		else
			return std::string();
	}

	// The safe routine's size includes the NULL.
	if (!result.empty() && result[size - 1] == '\0')
		result.erase(size - 1);
#else
	size_t size = wcstombs(NULL, str, 0);
	CRYPTOPP_ASSERT(size != (size_t)-1);
	if (size == (size_t)-1) {goto CONVERSION_ERROR;}

	result.resize(size);
	size = wcstombs(&result[0], str, size);
	CRYPTOPP_ASSERT(size != (size_t)-1);

	if (size == (size_t)-1)
	{
CONVERSION_ERROR:
		if (throwOnError)
			throw InvalidArgument("StringNarrow: wcstombs() call failed");
		else
			return std::string();
	}
#endif

	return result;
}
#endif // StringNarrow and CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562

#if !(defined(_MSC_VER) && (_MSC_VER < 1300))
using std::new_handler;
using std::set_new_handler;
#endif

void CallNewHandler()
{
	new_handler newHandler = set_new_handler(NULL);
	if (newHandler)
		set_new_handler(newHandler);

	if (newHandler)
		newHandler();
	else
		throw std::bad_alloc();
}

#if CRYPTOPP_BOOL_ALIGN16

void * AlignedAllocate(size_t size)
{
	byte *p;
#if defined(CRYPTOPP_APPLE_ALLOC_AVAILABLE)
	while ((p = (byte *)calloc(1, size)) == NULL)
#elif defined(CRYPTOPP_MM_MALLOC_AVAILABLE)
	while ((p = (byte *)_mm_malloc(size, 16)) == NULL)
#elif defined(CRYPTOPP_MEMALIGN_AVAILABLE)
	while ((p = (byte *)memalign(16, size)) == NULL)
#elif defined(CRYPTOPP_MALLOC_ALIGNMENT_IS_16)
	while ((p = (byte *)malloc(size)) == NULL)
#else
	while ((p = (byte *)malloc(size + 16)) == NULL)
#endif
		CallNewHandler();

#ifdef CRYPTOPP_NO_ALIGNED_ALLOC
	size_t adjustment = 16-((size_t)p%16);
	p += adjustment;
	p[-1] = (byte)adjustment;
#endif

	CRYPTOPP_ASSERT(IsAlignedOn(p, 16));
	return p;
}

void AlignedDeallocate(void *p)
{
#ifdef CRYPTOPP_MM_MALLOC_AVAILABLE
	_mm_free(p);
#elif defined(CRYPTOPP_NO_ALIGNED_ALLOC)
	p = (byte *)p - ((byte *)p)[-1];
	free(p);
#else
	free(p);
#endif
}

#endif

void * UnalignedAllocate(size_t size)
{
	void *p;
	while ((p = malloc(size)) == NULL)
		CallNewHandler();
	return p;
}

void UnalignedDeallocate(void *p)
{
	free(p);
}

NAMESPACE_END

#endif
