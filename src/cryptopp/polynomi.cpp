// polynomi.cpp - written and placed in the public domain by Wei Dai

// Part of the code for polynomial evaluation and interpolation
// originally came from Hal Finney's public domain secsplit.c.

#include "pch.h"
#include "polynomi.h"
#include "secblock.h"

#include <sstream>
#include <iostream>

NAMESPACE_BEGIN(CryptoPP)

template <class T>
void PolynomialOver<T>::Randomize(RandomNumberGenerator &rng, const RandomizationParameter &parameter, const Ring &ring)
{
	m_coefficients.resize(parameter.m_coefficientCount);
	for (unsigned int i=0; i<m_coefficients.size(); ++i)
		m_coefficients[i] = ring.RandomElement(rng, parameter.m_coefficientParameter);
}

template <class T>
void PolynomialOver<T>::FromStr(const char *str, const Ring &ring)
{
	std::istringstream in((char *)str);
	bool positive = true;
	CoefficientType coef;
	unsigned int power;

	while (in)
	{
		std::ws(in);
		if (in.peek() == 'x')
			coef = ring.MultiplicativeIdentity();
		else
			in >> coef;

		std::ws(in);
		if (in.peek() == 'x')
		{
			in.get();
			std::ws(in);
			if (in.peek() == '^')
			{
				in.get();
				in >> power;
			}
			else
				power = 1;
		}
		else
			power = 0;

		if (!positive)
			coef = ring.Inverse(coef);

		SetCoefficient(power, coef, ring);

		std::ws(in);
		switch (in.get())
		{
		case '+':
			positive = true;
			break;
		case '-':
			positive = false;
			break;
		default:
			return;		// something's wrong with the input string
		}
	}
}

template <class T>
unsigned int PolynomialOver<T>::CoefficientCount(const Ring &ring) const
{
	unsigned count = m_coefficients.size();
	while (count && ring.Equal(m_coefficients[count-1], ring.Identity()))
		count--;
	const_cast<std::vector<CoefficientType> &>(m_coefficients).resize(count);
	return count;
}

template <class T>
typename PolynomialOver<T>::CoefficientType PolynomialOver<T>::GetCoefficient(unsigned int i, const Ring &ring) const 
{
	return (i < m_coefficients.size()) ? m_coefficients[i] : ring.Identity();
}

template <class T>
PolynomialOver<T>&  PolynomialOver<T>::operator=(const PolynomialOver<T>& t)
{
	if (this != &t)
	{
		m_coefficients.resize(t.m_coefficients.size());
		for (unsigned int i=0; i<m_coefficients.size(); i++)
			m_coefficients[i] = t.m_coefficients[i];
	}
	return *this;
}

template <class T>
PolynomialOver<T>& PolynomialOver<T>::Accumulate(const PolynomialOver<T>& t, const Ring &ring)
{
	unsigned int count = t.CoefficientCount(ring);

	if (count > CoefficientCount(ring))
		m_coefficients.resize(count, ring.Identity());

	for (unsigned int i=0; i<count; i++)
		ring.Accumulate(m_coefficients[i], t.GetCoefficient(i, ring));

	return *this;
}

template <class T>
PolynomialOver<T>& PolynomialOver<T>::Reduce(const PolynomialOver<T>& t, const Ring &ring)
{
	unsigned int count = t.CoefficientCount(ring);

	if (count > CoefficientCount(ring))
		m_coefficients.resize(count, ring.Identity());

	for (unsigned int i=0; i<count; i++)
		ring.Reduce(m_coefficients[i], t.GetCoefficient(i, ring));

	return *this;
}

template <class T>
typename PolynomialOver<T>::CoefficientType PolynomialOver<T>::EvaluateAt(const CoefficientType &x, const Ring &ring) const
{
	int degree = Degree(ring);

	if (degree < 0)
		return ring.Identity();

	CoefficientType result = m_coefficients[degree];
	for (int j=degree-1; j>=0; j--)
	{
		result = ring.Multiply(result, x);
		ring.Accumulate(result, m_coefficients[j]);
	}
	return result;
}

template <class T>
PolynomialOver<T>& PolynomialOver<T>::ShiftLeft(unsigned int n, const Ring &ring)
{
	unsigned int i = CoefficientCount(ring) + n;
	m_coefficients.resize(i, ring.Identity());
	while (i > n)
	{
		i--;
		m_coefficients[i] = m_coefficients[i-n];
	}
	while (i)
	{
		i--;
		m_coefficients[i] = ring.Identity();
	}
	return *this;
}

template <class T>
PolynomialOver<T>& PolynomialOver<T>::ShiftRight(unsigned int n, const Ring &ring)
{
	unsigned int count = CoefficientCount(ring);
	if (count > n)
	{
		for (unsigned int i=0; i<count-n; i++)
			m_coefficients[i] = m_coefficients[i+n];
		m_coefficients.resize(count-n, ring.Identity());
	}
	else
		m_coefficients.resize(0, ring.Identity());
	return *this;
}

template <class T>
void PolynomialOver<T>::SetCoefficient(unsigned int i, const CoefficientType &value, const Ring &ring)
{
	if (i >= m_coefficients.size())
		m_coefficients.resize(i+1, ring.Identity());
	m_coefficients[i] = value;
}

template <class T>
void PolynomialOver<T>::Negate(const Ring &ring)
{
	unsigned int count = CoefficientCount(ring);
	for (unsigned int i=0; i<count; i++)
		m_coefficients[i] = ring.Inverse(m_coefficients[i]);
}

template <class T>
void PolynomialOver<T>::swap(PolynomialOver<T> &t)
{
	m_coefficients.swap(t.m_coefficients);
}

template <class T>
bool PolynomialOver<T>::Equals(const PolynomialOver<T>& t, const Ring &ring) const
{
	unsigned int count = CoefficientCount(ring);

	if (count != t.CoefficientCount(ring))
		return false;

	for (unsigned int i=0; i<count; i++)
		if (!ring.Equal(m_coefficients[i], t.m_coefficients[i]))
			return false;

	return true;
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::Plus(const PolynomialOver<T>& t, const Ring &ring) const
{
	unsigned int i;
	unsigned int count = CoefficientCount(ring);
	unsigned int tCount = t.CoefficientCount(ring);

	if (count > tCount)
	{
		PolynomialOver<T> result(ring, count);

		for (i=0; i<tCount; i++)
			result.m_coefficients[i] = ring.Add(m_coefficients[i], t.m_coefficients[i]);
		for (; i<count; i++)
			result.m_coefficients[i] = m_coefficients[i];

		return result;
	}
	else
	{
		PolynomialOver<T> result(ring, tCount);

		for (i=0; i<count; i++)
			result.m_coefficients[i] = ring.Add(m_coefficients[i], t.m_coefficients[i]);
		for (; i<tCount; i++)
			result.m_coefficients[i] = t.m_coefficients[i];

		return result;
	}
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::Minus(const PolynomialOver<T>& t, const Ring &ring) const
{
	unsigned int i;
	unsigned int count = CoefficientCount(ring);
	unsigned int tCount = t.CoefficientCount(ring);

	if (count > tCount)
	{
		PolynomialOver<T> result(ring, count);

		for (i=0; i<tCount; i++)
			result.m_coefficients[i] = ring.Subtract(m_coefficients[i], t.m_coefficients[i]);
		for (; i<count; i++)
			result.m_coefficients[i] = m_coefficients[i];

		return result;
	}
	else
	{
		PolynomialOver<T> result(ring, tCount);

		for (i=0; i<count; i++)
			result.m_coefficients[i] = ring.Subtract(m_coefficients[i], t.m_coefficients[i]);
		for (; i<tCount; i++)
			result.m_coefficients[i] = ring.Inverse(t.m_coefficients[i]);

		return result;
	}
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::Inverse(const Ring &ring) const
{
	unsigned int count = CoefficientCount(ring);
	PolynomialOver<T> result(ring, count);

	for (unsigned int i=0; i<count; i++)
		result.m_coefficients[i] = ring.Inverse(m_coefficients[i]);

	return result;
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::Times(const PolynomialOver<T>& t, const Ring &ring) const
{
	if (IsZero(ring) || t.IsZero(ring))
		return PolynomialOver<T>();

	unsigned int count1 = CoefficientCount(ring), count2 = t.CoefficientCount(ring);
	PolynomialOver<T> result(ring, count1 + count2 - 1);

	for (unsigned int i=0; i<count1; i++)
		for (unsigned int j=0; j<count2; j++)
			ring.Accumulate(result.m_coefficients[i+j], ring.Multiply(m_coefficients[i], t.m_coefficients[j]));

	return result;
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::DividedBy(const PolynomialOver<T>& t, const Ring &ring) const
{
	PolynomialOver<T> remainder, quotient;
	Divide(remainder, quotient, *this, t, ring);
	return quotient;
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::Modulo(const PolynomialOver<T>& t, const Ring &ring) const
{
	PolynomialOver<T> remainder, quotient;
	Divide(remainder, quotient, *this, t, ring);
	return remainder;
}

template <class T>
PolynomialOver<T> PolynomialOver<T>::MultiplicativeInverse(const Ring &ring) const
{
	return Degree(ring)==0 ? ring.MultiplicativeInverse(m_coefficients[0]) : ring.Identity();
}

template <class T>
bool PolynomialOver<T>::IsUnit(const Ring &ring) const
{
	return Degree(ring)==0 && ring.IsUnit(m_coefficients[0]);
}

template <class T>
std::istream& PolynomialOver<T>::Input(std::istream &in, const Ring &ring)
{
	char c;
	unsigned int length = 0;
	SecBlock<char> str(length + 16);
	bool paren = false;

	std::ws(in);

	if (in.peek() == '(')
	{
		paren = true;
		in.get();
	}

	do
	{
		in.read(&c, 1);
		str[length++] = c;
		if (length >= str.size())
			str.Grow(length + 16);
	}
	// if we started with a left paren, then read until we find a right paren,
	// otherwise read until the end of the line
	while (in && ((paren && c != ')') || (!paren && c != '\n')));

	str[length-1] = '\0';
	*this = PolynomialOver<T>(str, ring);

	return in;
}

template <class T>
std::ostream& PolynomialOver<T>::Output(std::ostream &out, const Ring &ring) const
{
	unsigned int i = CoefficientCount(ring);
	if (i)
	{
		bool firstTerm = true;

		while (i--)
		{
			if (m_coefficients[i] != ring.Identity())
			{
				if (firstTerm)
				{
					firstTerm = false;
					if (!i || !ring.Equal(m_coefficients[i], ring.MultiplicativeIdentity()))
						out << m_coefficients[i];
				}
				else
				{
					CoefficientType inverse = ring.Inverse(m_coefficients[i]);
					std::ostringstream pstr, nstr;

					pstr << m_coefficients[i];
					nstr << inverse;

					if (pstr.str().size() <= nstr.str().size())
					{
						out << " + "; 
						if (!i || !ring.Equal(m_coefficients[i], ring.MultiplicativeIdentity()))
							out << m_coefficients[i];
					}
					else
					{
						out << " - "; 
						if (!i || !ring.Equal(inverse, ring.MultiplicativeIdentity()))
							out << inverse;
					}
				}

				switch (i)
				{
				case 0:
					break;
				case 1:
					out << "x";
					break;
				default:
					out << "x^" << i;
				}
			}
		}
	}
	else
	{
		out << ring.Identity();
	}
	return out;
}

template <class T>
void PolynomialOver<T>::Divide(PolynomialOver<T> &r, PolynomialOver<T> &q, const PolynomialOver<T> &a, const PolynomialOver<T> &d, const Ring &ring)
{
	unsigned int i = a.CoefficientCount(ring);
	const int dDegree = d.Degree(ring);

	if (dDegree < 0)
		throw DivideByZero();

	r = a;
	q.m_coefficients.resize(STDMAX(0, int(i - dDegree)));

	while (i > (unsigned int)dDegree)
	{
		--i;
		q.m_coefficients[i-dDegree] = ring.Divide(r.m_coefficients[i], d.m_coefficients[dDegree]);
		for (int j=0; j<=dDegree; j++)
			ring.Reduce(r.m_coefficients[i-dDegree+j], ring.Multiply(q.m_coefficients[i-dDegree], d.m_coefficients[j]));
	}

	r.CoefficientCount(ring);	// resize r.m_coefficients
}

// ********************************************************

// helper function for Interpolate() and InterpolateAt()
template <class T>
void RingOfPolynomialsOver<T>::CalculateAlpha(std::vector<CoefficientType> &alpha, const CoefficientType x[], const CoefficientType y[], unsigned int n) const
{
	for (unsigned int j=0; j<n; ++j)
		alpha[j] = y[j];

	for (unsigned int k=1; k<n; ++k)
	{
		for (unsigned int j=n-1; j>=k; --j)
		{
			m_ring.Reduce(alpha[j], alpha[j-1]);

			CoefficientType d = m_ring.Subtract(x[j], x[j-k]);
			if (!m_ring.IsUnit(d))
				throw InterpolationFailed();
			alpha[j] = m_ring.Divide(alpha[j], d);
		}
	}
}

template <class T>
typename RingOfPolynomialsOver<T>::Element RingOfPolynomialsOver<T>::Interpolate(const CoefficientType x[], const CoefficientType y[], unsigned int n) const
{
	assert(n > 0);

	std::vector<CoefficientType> alpha(n);
	CalculateAlpha(alpha, x, y, n);

	std::vector<CoefficientType> coefficients((size_t)n, m_ring.Identity());
	coefficients[0] = alpha[n-1];

	for (int j=n-2; j>=0; --j)
	{
		for (unsigned int i=n-j-1; i>0; i--)
			coefficients[i] = m_ring.Subtract(coefficients[i-1], m_ring.Multiply(coefficients[i], x[j]));

		coefficients[0] = m_ring.Subtract(alpha[j], m_ring.Multiply(coefficients[0], x[j]));
	}

	return PolynomialOver<T>(coefficients.begin(), coefficients.end());
}

template <class T>
typename RingOfPolynomialsOver<T>::CoefficientType RingOfPolynomialsOver<T>::InterpolateAt(const CoefficientType &position, const CoefficientType x[], const CoefficientType y[], unsigned int n) const
{
	assert(n > 0);

	std::vector<CoefficientType> alpha(n);
	CalculateAlpha(alpha, x, y, n);

	CoefficientType result = alpha[n-1];
	for (int j=n-2; j>=0; --j)
	{
		result = m_ring.Multiply(result, m_ring.Subtract(position, x[j]));
		m_ring.Accumulate(result, alpha[j]);
	}
	return result;
}

template <class Ring, class Element>
void PrepareBulkPolynomialInterpolation(const Ring &ring, Element *w, const Element x[], unsigned int n)
{
	for (unsigned int i=0; i<n; i++)
	{
		Element t = ring.MultiplicativeIdentity();
		for (unsigned int j=0; j<n; j++)
			if (i != j)
				t = ring.Multiply(t, ring.Subtract(x[i], x[j]));
		w[i] = ring.MultiplicativeInverse(t);
	}
}

template <class Ring, class Element>
void PrepareBulkPolynomialInterpolationAt(const Ring &ring, Element *v, const Element &position, const Element x[], const Element w[], unsigned int n)
{
	assert(n > 0);

	std::vector<Element> a(2*n-1);
	unsigned int i;

	for (i=0; i<n; i++)
		a[n-1+i] = ring.Subtract(position, x[i]);

	for (i=n-1; i>1; i--)
		a[i-1] = ring.Multiply(a[2*i], a[2*i-1]);

	a[0] = ring.MultiplicativeIdentity();

	for (i=0; i<n-1; i++)
	{
		std::swap(a[2*i+1], a[2*i+2]);
		a[2*i+1] = ring.Multiply(a[i], a[2*i+1]);
		a[2*i+2] = ring.Multiply(a[i], a[2*i+2]);
	}

	for (i=0; i<n; i++)
		v[i] = ring.Multiply(a[n-1+i], w[i]);
}

template <class Ring, class Element>
Element BulkPolynomialInterpolateAt(const Ring &ring, const Element y[], const Element v[], unsigned int n)
{
	Element result = ring.Identity();
	for (unsigned int i=0; i<n; i++)
		ring.Accumulate(result, ring.Multiply(y[i], v[i]));
	return result;
}

// ********************************************************

template <class T, int instance>
const PolynomialOverFixedRing<T, instance> &PolynomialOverFixedRing<T, instance>::Zero()
{
	return Singleton<ThisType>().Ref();
}

template <class T, int instance>
const PolynomialOverFixedRing<T, instance> &PolynomialOverFixedRing<T, instance>::One()
{
	return Singleton<ThisType, NewOnePolynomial>().Ref();
}

NAMESPACE_END
