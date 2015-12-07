// gf2n.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#ifndef CRYPTOPP_IMPORTS

#include "cryptlib.h"
#include "algebra.h"
#include "randpool.h"
#include "filters.h"
#include "smartptr.h"
#include "words.h"
#include "misc.h"
#include "gf2n.h"
#include "asn.h"
#include "oids.h"

#include <iostream>

NAMESPACE_BEGIN(CryptoPP)

PolynomialMod2::PolynomialMod2()
{
}

PolynomialMod2::PolynomialMod2(word value, size_t bitLength)
	: reg(BitsToWords(bitLength))
{
	assert(value==0 || reg.size()>0);

	if (reg.size() > 0)
	{
		reg[0] = value;
		SetWords(reg+1, 0, reg.size()-1);
	}
}

PolynomialMod2::PolynomialMod2(const PolynomialMod2& t)
	: reg(t.reg.size())
{
	CopyWords(reg, t.reg, reg.size());
}

void PolynomialMod2::Randomize(RandomNumberGenerator &rng, size_t nbits)
{
	const size_t nbytes = nbits/8 + 1;
	SecByteBlock buf(nbytes);
	rng.GenerateBlock(buf, nbytes);
	buf[0] = (byte)Crop(buf[0], nbits % 8);
	Decode(buf, nbytes);
}

PolynomialMod2 PolynomialMod2::AllOnes(size_t bitLength)
{
	PolynomialMod2 result((word)0, bitLength);
	SetWords(result.reg, word(SIZE_MAX), result.reg.size());
	if (bitLength%WORD_BITS)
		result.reg[result.reg.size()-1] = (word)Crop(result.reg[result.reg.size()-1], bitLength%WORD_BITS);
	return result;
}

void PolynomialMod2::SetBit(size_t n, int value)
{
	if (value)
	{
		reg.CleanGrow(n/WORD_BITS + 1);
		reg[n/WORD_BITS] |= (word(1) << (n%WORD_BITS));
	}
	else
	{
		if (n/WORD_BITS < reg.size())
			reg[n/WORD_BITS] &= ~(word(1) << (n%WORD_BITS));
	}
}

byte PolynomialMod2::GetByte(size_t n) const
{
	if (n/WORD_SIZE >= reg.size())
		return 0;
	else
		return byte(reg[n/WORD_SIZE] >> ((n%WORD_SIZE)*8));
}

void PolynomialMod2::SetByte(size_t n, byte value)
{
	reg.CleanGrow(BytesToWords(n+1));
	reg[n/WORD_SIZE] &= ~(word(0xff) << 8*(n%WORD_SIZE));
	reg[n/WORD_SIZE] |= (word(value) << 8*(n%WORD_SIZE));
}

PolynomialMod2 PolynomialMod2::Monomial(size_t i) 
{
	PolynomialMod2 r((word)0, i+1); 
	r.SetBit(i); 
	return r;
}

PolynomialMod2 PolynomialMod2::Trinomial(size_t t0, size_t t1, size_t t2) 
{
	PolynomialMod2 r((word)0, t0+1);
	r.SetBit(t0);
	r.SetBit(t1);
	r.SetBit(t2);
	return r;
}

PolynomialMod2 PolynomialMod2::Pentanomial(size_t t0, size_t t1, size_t t2, size_t t3, size_t t4)
{
	PolynomialMod2 r((word)0, t0+1);
	r.SetBit(t0);
	r.SetBit(t1);
	r.SetBit(t2);
	r.SetBit(t3);
	r.SetBit(t4);
	return r;
}

template <word i>
struct NewPolynomialMod2
{
	PolynomialMod2 * operator()() const
	{
		return new PolynomialMod2(i);
	}
};

const PolynomialMod2 &PolynomialMod2::Zero()
{
	return Singleton<PolynomialMod2>().Ref();
}

const PolynomialMod2 &PolynomialMod2::One()
{
	return Singleton<PolynomialMod2, NewPolynomialMod2<1> >().Ref();
}

void PolynomialMod2::Decode(const byte *input, size_t inputLen)
{
	StringStore store(input, inputLen);
	Decode(store, inputLen);
}

void PolynomialMod2::Encode(byte *output, size_t outputLen) const
{
	ArraySink sink(output, outputLen);
	Encode(sink, outputLen);
}

void PolynomialMod2::Decode(BufferedTransformation &bt, size_t inputLen)
{
	reg.CleanNew(BytesToWords(inputLen));

	for (size_t i=inputLen; i > 0; i--)
	{
		byte b;
		bt.Get(b);
		reg[(i-1)/WORD_SIZE] |= word(b) << ((i-1)%WORD_SIZE)*8;
	}
}

void PolynomialMod2::Encode(BufferedTransformation &bt, size_t outputLen) const
{
	for (size_t i=outputLen; i > 0; i--)
		bt.Put(GetByte(i-1));
}

void PolynomialMod2::DEREncodeAsOctetString(BufferedTransformation &bt, size_t length) const
{
	DERGeneralEncoder enc(bt, OCTET_STRING);
	Encode(enc, length);
	enc.MessageEnd();
}

void PolynomialMod2::BERDecodeAsOctetString(BufferedTransformation &bt, size_t length)
{
	BERGeneralDecoder dec(bt, OCTET_STRING);
	if (!dec.IsDefiniteLength() || dec.RemainingLength() != length)
		BERDecodeError();
	Decode(dec, length);
	dec.MessageEnd();
}

unsigned int PolynomialMod2::WordCount() const
{
	return (unsigned int)CountWords(reg, reg.size());
}

unsigned int PolynomialMod2::ByteCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_SIZE + BytePrecision(reg[wordCount-1]);
	else
		return 0;
}

unsigned int PolynomialMod2::BitCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_BITS + BitPrecision(reg[wordCount-1]);
	else
		return 0;
}

unsigned int PolynomialMod2::Parity() const
{
	unsigned i;
	word temp=0;
	for (i=0; i<reg.size(); i++)
		temp ^= reg[i];
	return CryptoPP::Parity(temp);
}

PolynomialMod2& PolynomialMod2::operator=(const PolynomialMod2& t)
{
	reg.Assign(t.reg);
	return *this;
}

PolynomialMod2& PolynomialMod2::operator^=(const PolynomialMod2& t)
{
	reg.CleanGrow(t.reg.size());
	XorWords(reg, t.reg, t.reg.size());
	return *this;
}

PolynomialMod2 PolynomialMod2::Xor(const PolynomialMod2 &b) const
{
	if (b.reg.size() >= reg.size())
	{
		PolynomialMod2 result((word)0, b.reg.size()*WORD_BITS);
		XorWords(result.reg, reg, b.reg, reg.size());
		CopyWords(result.reg+reg.size(), b.reg+reg.size(), b.reg.size()-reg.size());
		return result;
	}
	else
	{
		PolynomialMod2 result((word)0, reg.size()*WORD_BITS);
		XorWords(result.reg, reg, b.reg, b.reg.size());
		CopyWords(result.reg+b.reg.size(), reg+b.reg.size(), reg.size()-b.reg.size());
		return result;
	}
}

PolynomialMod2 PolynomialMod2::And(const PolynomialMod2 &b) const
{
	PolynomialMod2 result((word)0, WORD_BITS*STDMIN(reg.size(), b.reg.size()));
	AndWords(result.reg, reg, b.reg, result.reg.size());
	return result;
}

PolynomialMod2 PolynomialMod2::Times(const PolynomialMod2 &b) const
{
	PolynomialMod2 result((word)0, BitCount() + b.BitCount());

	for (int i=b.Degree(); i>=0; i--)
	{
		result <<= 1;
		if (b[i])
			XorWords(result.reg, reg, reg.size());
	}
	return result;
}

PolynomialMod2 PolynomialMod2::Squared() const
{
	static const word map[16] = {0, 1, 4, 5, 16, 17, 20, 21, 64, 65, 68, 69, 80, 81, 84, 85};

	PolynomialMod2 result((word)0, 2*reg.size()*WORD_BITS);

	for (unsigned i=0; i<reg.size(); i++)
	{
		unsigned j;

		for (j=0; j<WORD_BITS; j+=8)
			result.reg[2*i] |= map[(reg[i] >> (j/2)) % 16] << j;

		for (j=0; j<WORD_BITS; j+=8)
			result.reg[2*i+1] |= map[(reg[i] >> (j/2 + WORD_BITS/2)) % 16] << j;
	}

	return result;
}

void PolynomialMod2::Divide(PolynomialMod2 &remainder, PolynomialMod2 &quotient,
				   const PolynomialMod2 &dividend, const PolynomialMod2 &divisor)
{
	if (!divisor)
		throw PolynomialMod2::DivideByZero();

	int degree = divisor.Degree();
	remainder.reg.CleanNew(BitsToWords(degree+1));
	if (dividend.BitCount() >= divisor.BitCount())
		quotient.reg.CleanNew(BitsToWords(dividend.BitCount() - divisor.BitCount() + 1));
	else
		quotient.reg.CleanNew(0);

	for (int i=dividend.Degree(); i>=0; i--)
	{
		remainder <<= 1;
		remainder.reg[0] |= dividend[i];
		if (remainder[degree])
		{
			remainder -= divisor;
			quotient.SetBit(i);
		}
	}
}

PolynomialMod2 PolynomialMod2::DividedBy(const PolynomialMod2 &b) const
{
	PolynomialMod2 remainder, quotient;
	PolynomialMod2::Divide(remainder, quotient, *this, b);
	return quotient;
}

PolynomialMod2 PolynomialMod2::Modulo(const PolynomialMod2 &b) const
{
	PolynomialMod2 remainder, quotient;
	PolynomialMod2::Divide(remainder, quotient, *this, b);
	return remainder;
}

PolynomialMod2& PolynomialMod2::operator<<=(unsigned int n)
{
#if !defined(NDEBUG)
	int x; CRYPTOPP_UNUSED(x);
	assert(SafeConvert(n,x));
#endif

	if (!reg.size())
		return *this;

	int i;
	word u;
	word carry=0;
	word *r=reg;

	if (n==1)	// special case code for most frequent case
	{
		i = (int)reg.size();
		while (i--)
		{
			u = *r;
			*r = (u << 1) | carry;
			carry = u >> (WORD_BITS-1);
			r++;
		}

		if (carry)
		{
			reg.Grow(reg.size()+1);
			reg[reg.size()-1] = carry;
		}

		return *this;
	}

	const int shiftWords = n / WORD_BITS;
	const int shiftBits = n % WORD_BITS;

	if (shiftBits)
	{
		i = (int)reg.size();
		while (i--)
		{
			u = *r;
			*r = (u << shiftBits) | carry;
			carry = u >> (WORD_BITS-shiftBits);
			r++;
		}
	}

	if (carry)
	{
		// Thanks to Apatryda, http://github.com/weidai11/cryptopp/issues/64
		const size_t carryIndex = reg.size();
		reg.Grow(reg.size()+shiftWords+!!shiftBits);
		reg[carryIndex] = carry;
	}
	else
		reg.Grow(reg.size()+shiftWords);

	if (shiftWords)
	{
		for (i = (int)reg.size()-1; i>=shiftWords; i--)
			reg[i] = reg[i-shiftWords];
		for (; i>=0; i--)
			reg[i] = 0;
	}

	return *this;
}

PolynomialMod2& PolynomialMod2::operator>>=(unsigned int n)
{
	if (!reg.size())
		return *this;

	int shiftWords = n / WORD_BITS;
	int shiftBits = n % WORD_BITS;

	size_t i;
	word u;
	word carry=0;
	word *r=reg+reg.size()-1;

	if (shiftBits)
	{
		i = reg.size();
		while (i--)
		{
			u = *r;
			*r = (u >> shiftBits) | carry;
			carry = u << (WORD_BITS-shiftBits);
			r--;
		}
	}

	if (shiftWords)
	{
		for (i=0; i<reg.size()-shiftWords; i++)
			reg[i] = reg[i+shiftWords];
		for (; i<reg.size(); i++)
			reg[i] = 0;
	}

	return *this;
}

PolynomialMod2 PolynomialMod2::operator<<(unsigned int n) const
{
	PolynomialMod2 result(*this);
	return result<<=n;
}

PolynomialMod2 PolynomialMod2::operator>>(unsigned int n) const
{
	PolynomialMod2 result(*this);
	return result>>=n;
}

bool PolynomialMod2::operator!() const
{
	for (unsigned i=0; i<reg.size(); i++)
		if (reg[i]) return false;
	return true;
}

bool PolynomialMod2::Equals(const PolynomialMod2 &rhs) const
{
	size_t i, smallerSize = STDMIN(reg.size(), rhs.reg.size());

	for (i=0; i<smallerSize; i++)
		if (reg[i] != rhs.reg[i]) return false;

	for (i=smallerSize; i<reg.size(); i++)
		if (reg[i] != 0) return false;

	for (i=smallerSize; i<rhs.reg.size(); i++)
		if (rhs.reg[i] != 0) return false;

	return true;
}

std::ostream& operator<<(std::ostream& out, const PolynomialMod2 &a)
{
	// Get relevant conversion specifications from ostream.
	long f = out.flags() & std::ios::basefield;	// Get base digits.
	int bits, block;
	char suffix;
	switch(f)
	{
	case std::ios::oct :
		bits = 3;
		block = 4;
		suffix = 'o';
		break;
	case std::ios::hex :
		bits = 4;
		block = 2;
		suffix = 'h';
		break;
	default :
		bits = 1;
		block = 8;
		suffix = 'b';
	}

	if (!a)
		return out << '0' << suffix;

	SecBlock<char> s(a.BitCount()/bits+1);
	unsigned i;

	static const char upper[]="0123456789ABCDEF";
	static const char lower[]="0123456789abcdef";
	const char* vec = (out.flags() & std::ios::uppercase) ? upper : lower;

	for (i=0; i*bits < a.BitCount(); i++)
	{
		int digit=0;
		for (int j=0; j<bits; j++)
			digit |= a[i*bits+j] << j;
		s[i]=vec[digit];
	}

	while (i--)
	{
		out << s[i];
		if (i && (i%block)==0)
			out << ',';
	}

	return out << suffix;
}

PolynomialMod2 PolynomialMod2::Gcd(const PolynomialMod2 &a, const PolynomialMod2 &b)
{
	return EuclideanDomainOf<PolynomialMod2>().Gcd(a, b);
}

PolynomialMod2 PolynomialMod2::InverseMod(const PolynomialMod2 &modulus) const
{
	typedef EuclideanDomainOf<PolynomialMod2> Domain;
	return QuotientRing<Domain>(Domain(), modulus).MultiplicativeInverse(*this);
}

bool PolynomialMod2::IsIrreducible() const
{
	signed int d = Degree();
	if (d <= 0)
		return false;

	PolynomialMod2 t(2), u(t);
	for (int i=1; i<=d/2; i++)
	{
		u = u.Squared()%(*this);
		if (!Gcd(u+t, *this).IsUnit())
			return false;
	}
	return true;
}

// ********************************************************

GF2NP::GF2NP(const PolynomialMod2 &modulus)
	: QuotientRing<EuclideanDomainOf<PolynomialMod2> >(EuclideanDomainOf<PolynomialMod2>(), modulus), m(modulus.Degree()) 
{
}

GF2NP::Element GF2NP::SquareRoot(const Element &a) const
{
	Element r = a;
	for (unsigned int i=1; i<m; i++)
		r = Square(r);
	return r;
}

GF2NP::Element GF2NP::HalfTrace(const Element &a) const
{
	assert(m%2 == 1);
	Element h = a;
	for (unsigned int i=1; i<=(m-1)/2; i++)
		h = Add(Square(Square(h)), a);
	return h;
}

GF2NP::Element GF2NP::SolveQuadraticEquation(const Element &a) const
{
	if (m%2 == 0)
	{
		Element z, w;
		RandomPool rng;
		do
		{
			Element p((RandomNumberGenerator &)rng, m);
			z = PolynomialMod2::Zero();
			w = p;
			for (unsigned int i=1; i<=m-1; i++)
			{
				w = Square(w);
				z = Square(z);
				Accumulate(z, Multiply(w, a));
				Accumulate(w, p);
			}
		} while (w.IsZero());
		return z;
	}
	else
		return HalfTrace(a);
}

// ********************************************************

GF2NT::GF2NT(unsigned int t0, unsigned int t1, unsigned int t2)
	: GF2NP(PolynomialMod2::Trinomial(t0, t1, t2))
	, t0(t0), t1(t1)
	, result((word)0, m)
{
	assert(t0 > t1 && t1 > t2 && t2==0);
}

const GF2NT::Element& GF2NT::MultiplicativeInverse(const Element &a) const
{
	if (t0-t1 < WORD_BITS)
		return GF2NP::MultiplicativeInverse(a);

	SecWordBlock T(m_modulus.reg.size() * 4);
	word *b = T;
	word *c = T+m_modulus.reg.size();
	word *f = T+2*m_modulus.reg.size();
	word *g = T+3*m_modulus.reg.size();
	size_t bcLen=1, fgLen=m_modulus.reg.size();
	unsigned int k=0;

	SetWords(T, 0, 3*m_modulus.reg.size());
	b[0]=1;
	assert(a.reg.size() <= m_modulus.reg.size());
	CopyWords(f, a.reg, a.reg.size());
	CopyWords(g, m_modulus.reg, m_modulus.reg.size());

	while (1)
	{
		word t=f[0];
		while (!t)
		{
			ShiftWordsRightByWords(f, fgLen, 1);
			if (c[bcLen-1])
				bcLen++;
			assert(bcLen <= m_modulus.reg.size());
			ShiftWordsLeftByWords(c, bcLen, 1);
			k+=WORD_BITS;
			t=f[0];
		}

		unsigned int i=0;
		while (t%2 == 0)
		{
			t>>=1;
			i++;
		}
		k+=i;

		if (t==1 && CountWords(f, fgLen)==1)
			break;

		if (i==1)
		{
			ShiftWordsRightByBits(f, fgLen, 1);
			t=ShiftWordsLeftByBits(c, bcLen, 1);
		}
		else
		{
			ShiftWordsRightByBits(f, fgLen, i);
			t=ShiftWordsLeftByBits(c, bcLen, i);
		}
		if (t)
		{
			c[bcLen] = t;
			bcLen++;
			assert(bcLen <= m_modulus.reg.size());
		}

		if (f[fgLen-1]==0 && g[fgLen-1]==0)
			fgLen--;

		if (f[fgLen-1] < g[fgLen-1])
		{
			std::swap(f, g);
			std::swap(b, c);
		}

		XorWords(f, g, fgLen);
		XorWords(b, c, bcLen);
	}

	while (k >= WORD_BITS)
	{
		word temp = b[0];
		// right shift b
		for (unsigned i=0; i+1<BitsToWords(m); i++)
			b[i] = b[i+1];
		b[BitsToWords(m)-1] = 0;

		// TODO: the shift by "t1+j" (64-bits) is being flagged as potential UB
		//   temp ^= ((temp >> j) & 1) << ((t1 + j) & (sizeof(temp)*8-1));
		if (t1 < WORD_BITS)
			for (unsigned int j=0; j<WORD_BITS-t1; j++)
				temp ^= ((temp >> j) & 1) << (t1 + j);
		else
			b[t1/WORD_BITS-1] ^= temp << t1%WORD_BITS;

		if (t1 % WORD_BITS)
			b[t1/WORD_BITS] ^= temp >> (WORD_BITS - t1%WORD_BITS);

		if (t0%WORD_BITS)
		{
			b[t0/WORD_BITS-1] ^= temp << t0%WORD_BITS;
			b[t0/WORD_BITS] ^= temp >> (WORD_BITS - t0%WORD_BITS);
		}
		else
			b[t0/WORD_BITS-1] ^= temp;

		k -= WORD_BITS;
	}

	if (k)
	{
		word temp = b[0] << (WORD_BITS - k);
		ShiftWordsRightByBits(b, BitsToWords(m), k);

		if (t1 < WORD_BITS)
		{
			for (unsigned int j=0; j<WORD_BITS-t1; j++)
			{
				// Coverity finding on shift amount of 'word x << (t1+j)'.
				assert(t1+j < WORD_BITS);
				temp ^= ((temp >> j) & 1) << (t1 + j);
			}
		}
		else
		{
			b[t1/WORD_BITS-1] ^= temp << t1%WORD_BITS;
		}

		if (t1 % WORD_BITS)
			b[t1/WORD_BITS] ^= temp >> (WORD_BITS - t1%WORD_BITS);

		if (t0%WORD_BITS)
		{
			b[t0/WORD_BITS-1] ^= temp << t0%WORD_BITS;
			b[t0/WORD_BITS] ^= temp >> (WORD_BITS - t0%WORD_BITS);
		}
		else
			b[t0/WORD_BITS-1] ^= temp;
	}

	CopyWords(result.reg.begin(), b, result.reg.size());
	return result;
}

const GF2NT::Element& GF2NT::Multiply(const Element &a, const Element &b) const
{
	size_t aSize = STDMIN(a.reg.size(), result.reg.size());
	Element r((word)0, m);

	for (int i=m-1; i>=0; i--)
	{
		if (r[m-1])
		{
			ShiftWordsLeftByBits(r.reg.begin(), r.reg.size(), 1);
			XorWords(r.reg.begin(), m_modulus.reg, r.reg.size());
		}
		else
			ShiftWordsLeftByBits(r.reg.begin(), r.reg.size(), 1);

		if (b[i])
			XorWords(r.reg.begin(), a.reg, aSize);
	}

	if (m%WORD_BITS)
		r.reg.begin()[r.reg.size()-1] = (word)Crop(r.reg[r.reg.size()-1], m%WORD_BITS);

	CopyWords(result.reg.begin(), r.reg.begin(), result.reg.size());
	return result;
}

const GF2NT::Element& GF2NT::Reduced(const Element &a) const
{
	if (t0-t1 < WORD_BITS)
		return m_domain.Mod(a, m_modulus);

	SecWordBlock b(a.reg);

	size_t i;
	for (i=b.size()-1; i>=BitsToWords(t0); i--)
	{
		word temp = b[i];

		if (t0%WORD_BITS)
		{
			b[i-t0/WORD_BITS] ^= temp >> t0%WORD_BITS;
			b[i-t0/WORD_BITS-1] ^= temp << (WORD_BITS - t0%WORD_BITS);
		}
		else
			b[i-t0/WORD_BITS] ^= temp;

		if ((t0-t1)%WORD_BITS)
		{
			b[i-(t0-t1)/WORD_BITS] ^= temp >> (t0-t1)%WORD_BITS;
			b[i-(t0-t1)/WORD_BITS-1] ^= temp << (WORD_BITS - (t0-t1)%WORD_BITS);
		}
		else
			b[i-(t0-t1)/WORD_BITS] ^= temp;
	}

	if (i==BitsToWords(t0)-1 && t0%WORD_BITS)
	{
		word mask = ((word)1<<(t0%WORD_BITS))-1;
		word temp = b[i] & ~mask;
		b[i] &= mask;

		b[i-t0/WORD_BITS] ^= temp >> t0%WORD_BITS;

		if ((t0-t1)%WORD_BITS)
		{
			b[i-(t0-t1)/WORD_BITS] ^= temp >> (t0-t1)%WORD_BITS;
			if ((t0-t1)%WORD_BITS > t0%WORD_BITS)
				b[i-(t0-t1)/WORD_BITS-1] ^= temp << (WORD_BITS - (t0-t1)%WORD_BITS);
			else
				assert(temp << (WORD_BITS - (t0-t1)%WORD_BITS) == 0);
		}
		else
			b[i-(t0-t1)/WORD_BITS] ^= temp;
	}

	SetWords(result.reg.begin(), 0, result.reg.size());
	CopyWords(result.reg.begin(), b, STDMIN(b.size(), result.reg.size()));
	return result;
}

void GF2NP::DEREncodeElement(BufferedTransformation &out, const Element &a) const
{
	a.DEREncodeAsOctetString(out, MaxElementByteLength());
}

void GF2NP::BERDecodeElement(BufferedTransformation &in, Element &a) const
{
	a.BERDecodeAsOctetString(in, MaxElementByteLength());
}

void GF2NT::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
		ASN1::characteristic_two_field().DEREncode(seq);
		DERSequenceEncoder parameters(seq);
			DEREncodeUnsigned(parameters, m);
			ASN1::tpBasis().DEREncode(parameters);
			DEREncodeUnsigned(parameters, t1);
		parameters.MessageEnd();
	seq.MessageEnd();
}

void GF2NPP::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
		ASN1::characteristic_two_field().DEREncode(seq);
		DERSequenceEncoder parameters(seq);
			DEREncodeUnsigned(parameters, m);
			ASN1::ppBasis().DEREncode(parameters);
			DERSequenceEncoder pentanomial(parameters);
				DEREncodeUnsigned(pentanomial, t3);
				DEREncodeUnsigned(pentanomial, t2);
				DEREncodeUnsigned(pentanomial, t1);
			pentanomial.MessageEnd();
		parameters.MessageEnd();
	seq.MessageEnd();
}

GF2NP * BERDecodeGF2NP(BufferedTransformation &bt)
{
	member_ptr<GF2NP> result;

	BERSequenceDecoder seq(bt);
		if (OID(seq) != ASN1::characteristic_two_field())
			BERDecodeError();
		BERSequenceDecoder parameters(seq);
			unsigned int m;
			BERDecodeUnsigned(parameters, m);
			OID oid(parameters);
			if (oid == ASN1::tpBasis())
			{
				unsigned int t1;
				BERDecodeUnsigned(parameters, t1);
				result.reset(new GF2NT(m, t1, 0));
			}
			else if (oid == ASN1::ppBasis())
			{
				unsigned int t1, t2, t3;
				BERSequenceDecoder pentanomial(parameters);
				BERDecodeUnsigned(pentanomial, t3);
				BERDecodeUnsigned(pentanomial, t2);
				BERDecodeUnsigned(pentanomial, t1);
				pentanomial.MessageEnd();
				result.reset(new GF2NPP(m, t3, t2, t1, 0));
			}
			else
			{
				BERDecodeError();
				return NULL;
			}
		parameters.MessageEnd();
	seq.MessageEnd();

	return result.release();
}

NAMESPACE_END

#endif
