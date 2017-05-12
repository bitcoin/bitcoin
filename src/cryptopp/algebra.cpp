// algebra.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_ALGEBRA_CPP	// SunCC workaround: compiler could cause this file to be included twice
#define CRYPTOPP_ALGEBRA_CPP

#include "algebra.h"
#include "integer.h"

#include <vector>

NAMESPACE_BEGIN(CryptoPP)

template <class T> const T& AbstractGroup<T>::Double(const Element &a) const
{
	return this->Add(a, a);
}

template <class T> const T& AbstractGroup<T>::Subtract(const Element &a, const Element &b) const
{
	// make copy of a in case Inverse() overwrites it
	Element a1(a);
	return this->Add(a1, Inverse(b));
}

template <class T> T& AbstractGroup<T>::Accumulate(Element &a, const Element &b) const
{
	return a = this->Add(a, b);
}

template <class T> T& AbstractGroup<T>::Reduce(Element &a, const Element &b) const
{
	return a = this->Subtract(a, b);
}

template <class T> const T& AbstractRing<T>::Square(const Element &a) const
{
	return this->Multiply(a, a);
}

template <class T> const T& AbstractRing<T>::Divide(const Element &a, const Element &b) const
{
	// make copy of a in case MultiplicativeInverse() overwrites it
	Element a1(a);
	return this->Multiply(a1, this->MultiplicativeInverse(b));
}

template <class T> const T& AbstractEuclideanDomain<T>::Mod(const Element &a, const Element &b) const
{
	Element q;
	this->DivisionAlgorithm(result, q, a, b);
	return result;
}

template <class T> const T& AbstractEuclideanDomain<T>::Gcd(const Element &a, const Element &b) const
{
	Element g[3]={b, a};
	unsigned int i0=0, i1=1, i2=2;

	while (!this->Equal(g[i1], this->Identity()))
	{
		g[i2] = this->Mod(g[i0], g[i1]);
		unsigned int t = i0; i0 = i1; i1 = i2; i2 = t;
	}

	return result = g[i0];
}

template <class T> const typename QuotientRing<T>::Element& QuotientRing<T>::MultiplicativeInverse(const Element &a) const
{
	Element g[3]={m_modulus, a};
	Element v[3]={m_domain.Identity(), m_domain.MultiplicativeIdentity()};
	Element y;
	unsigned int i0=0, i1=1, i2=2;

	while (!this->Equal(g[i1], this->Identity()))
	{
		// y = g[i0] / g[i1];
		// g[i2] = g[i0] % g[i1];
		m_domain.DivisionAlgorithm(g[i2], y, g[i0], g[i1]);
		// v[i2] = v[i0] - (v[i1] * y);
		v[i2] = m_domain.Subtract(v[i0], m_domain.Multiply(v[i1], y));
		unsigned int t = i0; i0 = i1; i1 = i2; i2 = t;
	}

	return m_domain.IsUnit(g[i0]) ? m_domain.Divide(v[i0], g[i0]) : m_domain.Identity();
}

template <class T> T AbstractGroup<T>::ScalarMultiply(const Element &base, const Integer &exponent) const
{
	Element result;
	this->SimultaneousMultiply(&result, base, &exponent, 1);
	return result;
}

template <class T> T AbstractGroup<T>::CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
{
	const unsigned expLen = STDMAX(e1.BitCount(), e2.BitCount());
	if (expLen==0)
		return this->Identity();

	const unsigned w = (expLen <= 46 ? 1 : (expLen <= 260 ? 2 : 3));
	const unsigned tableSize = 1<<w;
	std::vector<Element> powerTable(tableSize << w);

	powerTable[1] = x;
	powerTable[tableSize] = y;
	if (w==1)
		powerTable[3] = this->Add(x,y);
	else
	{
		powerTable[2] = this->Double(x);
		powerTable[2*tableSize] = this->Double(y);

		unsigned i, j;

		for (i=3; i<tableSize; i+=2)
			powerTable[i] = Add(powerTable[i-2], powerTable[2]);
		for (i=1; i<tableSize; i+=2)
			for (j=i+tableSize; j<(tableSize<<w); j+=tableSize)
				powerTable[j] = Add(powerTable[j-tableSize], y);

		for (i=3*tableSize; i<(tableSize<<w); i+=2*tableSize)
			powerTable[i] = Add(powerTable[i-2*tableSize], powerTable[2*tableSize]);
		for (i=tableSize; i<(tableSize<<w); i+=2*tableSize)
			for (j=i+2; j<i+tableSize; j+=2)
				powerTable[j] = Add(powerTable[j-1], x);
	}

	Element result;
	unsigned power1 = 0, power2 = 0, prevPosition = expLen-1;
	bool firstTime = true;

	for (int i = expLen-1; i>=0; i--)
	{
		power1 = 2*power1 + e1.GetBit(i);
		power2 = 2*power2 + e2.GetBit(i);

		if (i==0 || 2*power1 >= tableSize || 2*power2 >= tableSize)
		{
			unsigned squaresBefore = prevPosition-i;
			unsigned squaresAfter = 0;
			prevPosition = i;
			while ((power1 || power2) && power1%2 == 0 && power2%2==0)
			{
				power1 /= 2;
				power2 /= 2;
				squaresBefore--;
				squaresAfter++;
			}
			if (firstTime)
			{
				result = powerTable[(power2<<w) + power1];
				firstTime = false;
			}
			else
			{
				while (squaresBefore--)
					result = this->Double(result);
				if (power1 || power2)
					Accumulate(result, powerTable[(power2<<w) + power1]);
			}
			while (squaresAfter--)
				result = this->Double(result);
			power1 = power2 = 0;
		}
	}
	return result;
}

template <class Element, class Iterator> Element GeneralCascadeMultiplication(const AbstractGroup<Element> &group, Iterator begin, Iterator end)
{
	if (end-begin == 1)
		return group.ScalarMultiply(begin->base, begin->exponent);
	else if (end-begin == 2)
		return group.CascadeScalarMultiply(begin->base, begin->exponent, (begin+1)->base, (begin+1)->exponent);
	else
	{
		Integer q, t;
		Iterator last = end;
		--last;

		std::make_heap(begin, end);
		std::pop_heap(begin, end);

		while (!!begin->exponent)
		{
			// last->exponent is largest exponent, begin->exponent is next largest
			t = last->exponent;
			Integer::Divide(last->exponent, q, t, begin->exponent);

			if (q == Integer::One())
				group.Accumulate(begin->base, last->base);	// avoid overhead of ScalarMultiply()
			else
				group.Accumulate(begin->base, group.ScalarMultiply(last->base, q));

			std::push_heap(begin, end);
			std::pop_heap(begin, end);
		}

		return group.ScalarMultiply(last->base, last->exponent);
	}
}

struct WindowSlider
{
	WindowSlider(const Integer &expIn, bool fastNegate, unsigned int windowSizeIn=0)
		: exp(expIn), windowModulus(Integer::One()), windowSize(windowSizeIn), windowBegin(0), expWindow(0)
		, fastNegate(fastNegate), negateNext(false), firstTime(true), finished(false)
	{
		if (windowSize == 0)
		{
			unsigned int expLen = exp.BitCount();
			windowSize = expLen <= 17 ? 1 : (expLen <= 24 ? 2 : (expLen <= 70 ? 3 : (expLen <= 197 ? 4 : (expLen <= 539 ? 5 : (expLen <= 1434 ? 6 : 7)))));
		}
		windowModulus <<= windowSize;
	}

	void FindNextWindow()
	{
		unsigned int expLen = exp.WordCount() * WORD_BITS;
		unsigned int skipCount = firstTime ? 0 : windowSize;
		firstTime = false;
		while (!exp.GetBit(skipCount))
		{
			if (skipCount >= expLen)
			{
				finished = true;
				return;
			}
			skipCount++;
		}

		exp >>= skipCount;
		windowBegin += skipCount;
		expWindow = word32(exp % (word(1) << windowSize));

		if (fastNegate && exp.GetBit(windowSize))
		{
			negateNext = true;
			expWindow = (word32(1) << windowSize) - expWindow;
			exp += windowModulus;
		}
		else
			negateNext = false;
	}

	Integer exp, windowModulus;
	unsigned int windowSize, windowBegin;
	word32 expWindow;
	bool fastNegate, negateNext, firstTime, finished;
};

template <class T>
void AbstractGroup<T>::SimultaneousMultiply(T *results, const T &base, const Integer *expBegin, unsigned int expCount) const
{
	std::vector<std::vector<Element> > buckets(expCount);
	std::vector<WindowSlider> exponents;
	exponents.reserve(expCount);
	unsigned int i;

	for (i=0; i<expCount; i++)
	{
		CRYPTOPP_ASSERT(expBegin->NotNegative());
		exponents.push_back(WindowSlider(*expBegin++, InversionIsFast(), 0));
		exponents[i].FindNextWindow();
		buckets[i].resize(((size_t) 1) << (exponents[i].windowSize-1), Identity());
	}

	unsigned int expBitPosition = 0;
	Element g = base;
	bool notDone = true;

	while (notDone)
	{
		notDone = false;
		for (i=0; i<expCount; i++)
		{
			if (!exponents[i].finished && expBitPosition == exponents[i].windowBegin)
			{
				Element &bucket = buckets[i][exponents[i].expWindow/2];
				if (exponents[i].negateNext)
					Accumulate(bucket, Inverse(g));
				else
					Accumulate(bucket, g);
				exponents[i].FindNextWindow();
			}
			notDone = notDone || !exponents[i].finished;
		}

		if (notDone)
		{
			g = Double(g);
			expBitPosition++;
		}
	}

	for (i=0; i<expCount; i++)
	{
		Element &r = *results++;
		r = buckets[i][buckets[i].size()-1];
		if (buckets[i].size() > 1)
		{
			for (int j = (int)buckets[i].size()-2; j >= 1; j--)
			{
				Accumulate(buckets[i][j], buckets[i][j+1]);
				Accumulate(r, buckets[i][j]);
			}
			Accumulate(buckets[i][0], buckets[i][1]);
			r = Add(Double(r), buckets[i][0]);
		}
	}
}

template <class T> T AbstractRing<T>::Exponentiate(const Element &base, const Integer &exponent) const
{
	Element result;
	SimultaneousExponentiate(&result, base, &exponent, 1);
	return result;
}

template <class T> T AbstractRing<T>::CascadeExponentiate(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
{
	return MultiplicativeGroup().AbstractGroup<T>::CascadeScalarMultiply(x, e1, y, e2);
}

template <class Element, class Iterator> Element GeneralCascadeExponentiation(const AbstractRing<Element> &ring, Iterator begin, Iterator end)
{
	return GeneralCascadeMultiplication<Element>(ring.MultiplicativeGroup(), begin, end);
}

template <class T>
void AbstractRing<T>::SimultaneousExponentiate(T *results, const T &base, const Integer *exponents, unsigned int expCount) const
{
	MultiplicativeGroup().AbstractGroup<T>::SimultaneousMultiply(results, base, exponents, expCount);
}

NAMESPACE_END

#endif
