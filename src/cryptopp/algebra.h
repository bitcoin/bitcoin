// algebra.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile algebra.h
//! \brief Classes for performing mathematics over different fields

#ifndef CRYPTOPP_ALGEBRA_H
#define CRYPTOPP_ALGEBRA_H

#include "config.h"
#include "misc.h"
#include "integer.h"

NAMESPACE_BEGIN(CryptoPP)

class Integer;

// "const Element&" returned by member functions are references
// to internal data members. Since each object may have only
// one such data member for holding results, the following code
// will produce incorrect results:
// abcd = group.Add(group.Add(a,b), group.Add(c,d));
// But this should be fine:
// abcd = group.Add(a, group.Add(b, group.Add(c,d));

//! Abstract Group
template <class T> class CRYPTOPP_NO_VTABLE AbstractGroup
{
public:
	typedef T Element;

	virtual ~AbstractGroup() {}

	virtual bool Equal(const Element &a, const Element &b) const =0;
	virtual const Element& Identity() const =0;
	virtual const Element& Add(const Element &a, const Element &b) const =0;
	virtual const Element& Inverse(const Element &a) const =0;
	virtual bool InversionIsFast() const {return false;}

	virtual const Element& Double(const Element &a) const;
	virtual const Element& Subtract(const Element &a, const Element &b) const;
	virtual Element& Accumulate(Element &a, const Element &b) const;
	virtual Element& Reduce(Element &a, const Element &b) const;

	virtual Element ScalarMultiply(const Element &a, const Integer &e) const;
	virtual Element CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	virtual void SimultaneousMultiply(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
};

//! Abstract Ring
template <class T> class CRYPTOPP_NO_VTABLE AbstractRing : public AbstractGroup<T>
{
public:
	typedef T Element;

	AbstractRing() {m_mg.m_pRing = this;}
	AbstractRing(const AbstractRing &source)
		{CRYPTOPP_UNUSED(source); m_mg.m_pRing = this;}
	AbstractRing& operator=(const AbstractRing &source)
		{CRYPTOPP_UNUSED(source); return *this;}

	virtual bool IsUnit(const Element &a) const =0;
	virtual const Element& MultiplicativeIdentity() const =0;
	virtual const Element& Multiply(const Element &a, const Element &b) const =0;
	virtual const Element& MultiplicativeInverse(const Element &a) const =0;

	virtual const Element& Square(const Element &a) const;
	virtual const Element& Divide(const Element &a, const Element &b) const;

	virtual Element Exponentiate(const Element &a, const Integer &e) const;
	virtual Element CascadeExponentiate(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	virtual void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	virtual const AbstractGroup<T>& MultiplicativeGroup() const
		{return m_mg;}

private:
	class MultiplicativeGroupT : public AbstractGroup<T>
	{
	public:
		const AbstractRing<T>& GetRing() const
			{return *m_pRing;}

		bool Equal(const Element &a, const Element &b) const
			{return GetRing().Equal(a, b);}

		const Element& Identity() const
			{return GetRing().MultiplicativeIdentity();}

		const Element& Add(const Element &a, const Element &b) const
			{return GetRing().Multiply(a, b);}

		Element& Accumulate(Element &a, const Element &b) const
			{return a = GetRing().Multiply(a, b);}

		const Element& Inverse(const Element &a) const
			{return GetRing().MultiplicativeInverse(a);}

		const Element& Subtract(const Element &a, const Element &b) const
			{return GetRing().Divide(a, b);}

		Element& Reduce(Element &a, const Element &b) const
			{return a = GetRing().Divide(a, b);}

		const Element& Double(const Element &a) const
			{return GetRing().Square(a);}

		Element ScalarMultiply(const Element &a, const Integer &e) const
			{return GetRing().Exponentiate(a, e);}

		Element CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
			{return GetRing().CascadeExponentiate(x, e1, y, e2);}

		void SimultaneousMultiply(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
			{GetRing().SimultaneousExponentiate(results, base, exponents, exponentsCount);}

		const AbstractRing<T> *m_pRing;
	};

	MultiplicativeGroupT m_mg;
};

// ********************************************************

//! Base and Exponent
template <class T, class E = Integer>
struct BaseAndExponent
{
public:
	BaseAndExponent() {}
	BaseAndExponent(const T &base, const E &exponent) : base(base), exponent(exponent) {}
	bool operator<(const BaseAndExponent<T, E> &rhs) const {return exponent < rhs.exponent;}
	T base;
	E exponent;
};

// VC60 workaround: incomplete member template support
template <class Element, class Iterator>
	Element GeneralCascadeMultiplication(const AbstractGroup<Element> &group, Iterator begin, Iterator end);
template <class Element, class Iterator>
	Element GeneralCascadeExponentiation(const AbstractRing<Element> &ring, Iterator begin, Iterator end);

// ********************************************************

//! Abstract Euclidean Domain
template <class T> class CRYPTOPP_NO_VTABLE AbstractEuclideanDomain : public AbstractRing<T>
{
public:
	typedef T Element;

	virtual void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const =0;

	virtual const Element& Mod(const Element &a, const Element &b) const =0;
	virtual const Element& Gcd(const Element &a, const Element &b) const;

protected:
	mutable Element result;
};

// ********************************************************

//! EuclideanDomainOf
template <class T> class EuclideanDomainOf : public AbstractEuclideanDomain<T>
{
public:
	typedef T Element;

	EuclideanDomainOf() {}

	bool Equal(const Element &a, const Element &b) const
		{return a==b;}

	const Element& Identity() const
		{return Element::Zero();}

	const Element& Add(const Element &a, const Element &b) const
		{return result = a+b;}

	Element& Accumulate(Element &a, const Element &b) const
		{return a+=b;}

	const Element& Inverse(const Element &a) const
		{return result = -a;}

	const Element& Subtract(const Element &a, const Element &b) const
		{return result = a-b;}

	Element& Reduce(Element &a, const Element &b) const
		{return a-=b;}

	const Element& Double(const Element &a) const
		{return result = a.Doubled();}

	const Element& MultiplicativeIdentity() const
		{return Element::One();}

	const Element& Multiply(const Element &a, const Element &b) const
		{return result = a*b;}

	const Element& Square(const Element &a) const
		{return result = a.Squared();}

	bool IsUnit(const Element &a) const
		{return a.IsUnit();}

	const Element& MultiplicativeInverse(const Element &a) const
		{return result = a.MultiplicativeInverse();}

	const Element& Divide(const Element &a, const Element &b) const
		{return result = a/b;}

	const Element& Mod(const Element &a, const Element &b) const
		{return result = a%b;}

	void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const
		{Element::Divide(r, q, a, d);}

	bool operator==(const EuclideanDomainOf<T> &rhs) const
		{CRYPTOPP_UNUSED(rhs); return true;}

private:
	mutable Element result;
};

//! Quotient Ring
template <class T> class QuotientRing : public AbstractRing<typename T::Element>
{
public:
	typedef T EuclideanDomain;
	typedef typename T::Element Element;

	QuotientRing(const EuclideanDomain &domain, const Element &modulus)
		: m_domain(domain), m_modulus(modulus) {}

	const EuclideanDomain & GetDomain() const
		{return m_domain;}

	const Element& GetModulus() const
		{return m_modulus;}

	bool Equal(const Element &a, const Element &b) const
		{return m_domain.Equal(m_domain.Mod(m_domain.Subtract(a, b), m_modulus), m_domain.Identity());}

	const Element& Identity() const
		{return m_domain.Identity();}

	const Element& Add(const Element &a, const Element &b) const
		{return m_domain.Add(a, b);}

	Element& Accumulate(Element &a, const Element &b) const
		{return m_domain.Accumulate(a, b);}

	const Element& Inverse(const Element &a) const
		{return m_domain.Inverse(a);}

	const Element& Subtract(const Element &a, const Element &b) const
		{return m_domain.Subtract(a, b);}

	Element& Reduce(Element &a, const Element &b) const
		{return m_domain.Reduce(a, b);}

	const Element& Double(const Element &a) const
		{return m_domain.Double(a);}

	bool IsUnit(const Element &a) const
		{return m_domain.IsUnit(m_domain.Gcd(a, m_modulus));}

	const Element& MultiplicativeIdentity() const
		{return m_domain.MultiplicativeIdentity();}

	const Element& Multiply(const Element &a, const Element &b) const
		{return m_domain.Mod(m_domain.Multiply(a, b), m_modulus);}

	const Element& Square(const Element &a) const
		{return m_domain.Mod(m_domain.Square(a), m_modulus);}

	const Element& MultiplicativeInverse(const Element &a) const;

	bool operator==(const QuotientRing<T> &rhs) const
		{return m_domain == rhs.m_domain && m_modulus == rhs.m_modulus;}

protected:
	EuclideanDomain m_domain;
	Element m_modulus;
};

NAMESPACE_END

#ifdef CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#include "algebra.cpp"
#endif

#endif
