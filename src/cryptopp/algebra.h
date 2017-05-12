// algebra.h - written and placed in the public domain by Wei Dai

//! \file algebra.h
//! \brief Classes for performing mathematics over different fields

#ifndef CRYPTOPP_ALGEBRA_H
#define CRYPTOPP_ALGEBRA_H

#include "config.h"
#include "misc.h"
#include "integer.h"

NAMESPACE_BEGIN(CryptoPP)

class Integer;

//! \brief Abstract group
//! \tparam T element class or type
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
template <class T> class CRYPTOPP_NO_VTABLE AbstractGroup
{
public:
	typedef T Element;

	virtual ~AbstractGroup() {}

	//! \brief Compare two elements for equality
	//! \param a first element
	//! \param b second element
	//! \returns true if the elements are equal, false otherwise
	//! \details Equal() tests the elements for equality using <tt>a==b</tt>
	virtual bool Equal(const Element &a, const Element &b) const =0;

	//! \brief Provides the Identity element
	//! \returns the Identity element
	virtual const Element& Identity() const =0;

	//! \brief Adds elements in the group
	//! \param a first element
	//! \param b second element
	//! \returns the sum of <tt>a</tt> and <tt>b</tt>
	virtual const Element& Add(const Element &a, const Element &b) const =0;

	//! \brief Inverts the element in the group
	//! \param a first element
	//! \returns the inverse of the element
	virtual const Element& Inverse(const Element &a) const =0;

	//! \brief Determine if inversion is fast
	//! \returns true if inversion is fast, false otherwise
	virtual bool InversionIsFast() const {return false;}

	//! \brief Doubles an element in the group
	//! \param a the element
	//! \returns the element doubled
	virtual const Element& Double(const Element &a) const;

	//! \brief Subtracts elements in the group
	//! \param a first element
	//! \param b second element
	//! \returns the difference of <tt>a</tt> and <tt>b</tt>. The element <tt>a</tt> must provide a Subtract member function.
	virtual const Element& Subtract(const Element &a, const Element &b) const;

	//! \brief TODO
	//! \param a first element
	//! \param b second element
	//! \returns TODO
	virtual Element& Accumulate(Element &a, const Element &b) const;

	//! \brief Reduces an element in the congruence class
	//! \param a element to reduce
	//! \param b the congruence class
	//! \returns the reduced element
	virtual Element& Reduce(Element &a, const Element &b) const;

	//! \brief Performs a scalar multiplication
	//! \param a multiplicand
	//! \param e multiplier
	//! \returns the product
	virtual Element ScalarMultiply(const Element &a, const Integer &e) const;

	//! \brief TODO
	//! \param x first multiplicand
	//! \param e1 the first multiplier
	//! \param y second multiplicand
	//! \param e2 the second multiplier
	//! \returns TODO
	virtual Element CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	//! \brief Multiplies a base to multiple exponents in a group
	//! \param results an array of Elements
	//! \param base the base to raise to the exponents
	//! \param exponents an array of exponents
	//! \param exponentsCount the number of exponents in the array
	//! \details SimultaneousMultiply() multiplies the base to each exponent in the exponents array and stores the
	//!   result at the respective position in the results array.
	//! \details SimultaneousMultiply() must be implemented in a derived class.
	//! \pre <tt>COUNTOF(results) == exponentsCount</tt>
	//! \pre <tt>COUNTOF(exponents) == exponentsCount</tt>
	virtual void SimultaneousMultiply(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
};

//! \brief Abstract ring
//! \tparam T element class or type
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
template <class T> class CRYPTOPP_NO_VTABLE AbstractRing : public AbstractGroup<T>
{
public:
	typedef T Element;

	//! \brief Construct an AbstractRing
	AbstractRing() {m_mg.m_pRing = this;}

	//! \brief Copy construct an AbstractRing
	//! \param source other AbstractRing
	AbstractRing(const AbstractRing &source)
		{CRYPTOPP_UNUSED(source); m_mg.m_pRing = this;}

	//! \brief Assign an AbstractRing
	//! \param source other AbstractRing
	AbstractRing& operator=(const AbstractRing &source)
		{CRYPTOPP_UNUSED(source); return *this;}

	//! \brief Determines whether an element is a unit in the group
	//! \param a the element
	//! \returns true if the element is a unit after reduction, false otherwise.
	virtual bool IsUnit(const Element &a) const =0;

	//! \brief Retrieves the multiplicative identity
	//! \returns the multiplicative identity
	virtual const Element& MultiplicativeIdentity() const =0;

	//! \brief Multiplies elements in the group
	//! \param a the multiplicand
	//! \param b the multiplier
	//! \returns the product of a and b
	virtual const Element& Multiply(const Element &a, const Element &b) const =0;

	//! \brief Calculate the multiplicative inverse of an element in the group
	//! \param a the element
	virtual const Element& MultiplicativeInverse(const Element &a) const =0;

	//! \brief Square an element in the group
	//! \param a the element
	//! \returns the element squared
	virtual const Element& Square(const Element &a) const;

	//! \brief Divides elements in the group
	//! \param a the dividend
	//! \param b the divisor
	//! \returns the quotient
	virtual const Element& Divide(const Element &a, const Element &b) const;

	//! \brief Raises a base to an exponent in the group
	//! \param a the base
	//! \param e the exponent
	//! \returns the exponentiation
	virtual Element Exponentiate(const Element &a, const Integer &e) const;

	//! \brief TODO
	//! \param x first element
	//! \param e1 first exponent
	//! \param y second element
	//! \param e2 second exponent
	//! \returns TODO
	virtual Element CascadeExponentiate(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	//! \brief Exponentiates a base to multiple exponents in the Ring
	//! \param results an array of Elements
	//! \param base the base to raise to the exponents
	//! \param exponents an array of exponents
	//! \param exponentsCount the number of exponents in the array
	//! \details SimultaneousExponentiate() raises the base to each exponent in the exponents array and stores the
	//!   result at the respective position in the results array.
	//! \details SimultaneousExponentiate() must be implemented in a derived class.
	//! \pre <tt>COUNTOF(results) == exponentsCount</tt>
	//! \pre <tt>COUNTOF(exponents) == exponentsCount</tt>
	virtual void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	//! \brief Retrieves the multiplicative group
	//! \returns the multiplicative group
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

//! \brief Base and exponent
//! \tparam T base class or type
//! \tparam T exponent class or type
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

//! \brief Abstract Euclidean domain
//! \tparam T element class or type
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
template <class T> class CRYPTOPP_NO_VTABLE AbstractEuclideanDomain : public AbstractRing<T>
{
public:
	typedef T Element;

	//! \brief Performs the division algorithm on two elements in the ring
	//! \param r the remainder
	//! \param q the quotient
	//! \param a the dividend
	//! \param d the divisor
	virtual void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const =0;

	//! \brief Performs a modular reduction in the ring
	//! \param a the element
	//! \param b the modulus
	//! \returns the result of <tt>a%b</tt>.
	virtual const Element& Mod(const Element &a, const Element &b) const =0;

	//! \brief Calculates the greatest common denominator in the ring
	//! \param a the first element
	//! \param b the second element
	//! \returns the the greatest common denominator of a and b.
	virtual const Element& Gcd(const Element &a, const Element &b) const;

protected:
	mutable Element result;
};

// ********************************************************

//! \brief Euclidean domain
//! \tparam T element class or type
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
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

//! \brief Quotient ring
//! \tparam T element class or type
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
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
