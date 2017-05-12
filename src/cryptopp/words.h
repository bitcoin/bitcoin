#ifndef CRYPTOPP_WORDS_H
#define CRYPTOPP_WORDS_H

#include "config.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

inline size_t CountWords(const word *X, size_t N)
{
	while (N && X[N-1]==0)
		N--;
	return N;
}

inline void SetWords(word *r, word a, size_t n)
{
	for (size_t i=0; i<n; i++)
		r[i] = a;
}

inline void CopyWords(word *r, const word *a, size_t n)
{
	if (r != a)
#if CRYPTOPP_MSC_VERSION
		memcpy_s(r, n*WORD_SIZE, a, n*WORD_SIZE);
#else
		memcpy(r, a, n*WORD_SIZE);
#endif
}

inline void XorWords(word *r, const word *a, const word *b, size_t n)
{
	for (size_t i=0; i<n; i++)
		r[i] = a[i] ^ b[i];
}

inline void XorWords(word *r, const word *a, size_t n)
{
	for (size_t i=0; i<n; i++)
		r[i] ^= a[i];
}

inline void AndWords(word *r, const word *a, const word *b, size_t n)
{
	for (size_t i=0; i<n; i++)
		r[i] = a[i] & b[i];
}

inline void AndWords(word *r, const word *a, size_t n)
{
	for (size_t i=0; i<n; i++)
		r[i] &= a[i];
}

inline word ShiftWordsLeftByBits(word *r, size_t n, unsigned int shiftBits)
{
	CRYPTOPP_ASSERT (shiftBits<WORD_BITS);
	word u, carry=0;
	if (shiftBits)
		for (size_t i=0; i<n; i++)
		{
			u = r[i];
			r[i] = (u << shiftBits) | carry;
			carry = u >> (WORD_BITS-shiftBits);
		}
	return carry;
}

inline word ShiftWordsRightByBits(word *r, size_t n, unsigned int shiftBits)
{
	CRYPTOPP_ASSERT (shiftBits<WORD_BITS);
	word u, carry=0;
	if (shiftBits)
		for (size_t i=n; i>0; i--)
		{
			u = r[i-1];
			r[i-1] = (u >> shiftBits) | carry;
			carry = u << (WORD_BITS-shiftBits);
		}
	return carry;
}

inline void ShiftWordsLeftByWords(word *r, size_t n, size_t shiftWords)
{
	shiftWords = STDMIN(shiftWords, n);
	if (shiftWords)
	{
		for (size_t i=n-1; i>=shiftWords; i--)
			r[i] = r[i-shiftWords];
		SetWords(r, 0, shiftWords);
	}
}

inline void ShiftWordsRightByWords(word *r, size_t n, size_t shiftWords)
{
	shiftWords = STDMIN(shiftWords, n);
	if (shiftWords)
	{
		for (size_t i=0; i+shiftWords<n; i++)
			r[i] = r[i+shiftWords];
		SetWords(r+n-shiftWords, 0, shiftWords);
	}
}

NAMESPACE_END

#endif
