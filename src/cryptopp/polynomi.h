// polynomi.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile polynomi.h
//! \brief Classes for polynomial basis and operations


#ifndef CRYPTOPP_POLYNOMI_H
#define CRYPTOPP_POLYNOMI_H

/*! \file */

#include "cryptlib.h"
#include "secblock.h"
#include "algebra.h"
#include "misc.h"

#include <iosfwd>
#include <vector>

NAMESPACE_BEGIN(CryptoPP)

//! represents single-variable polynomials over arbitrary rings
/*!	\nosubgrouping */
template <class T> class PolynomialOver
{
public:
	//! \name ENUMS, EXCEPTIONS, and TYPEDEFS
	//@{
		//! division by zero exception
		class DivideByZero : public Exception
		{
		public:
			DivideByZero() : Exception(OTHER_ERROR, "PolynomialOver<T>: division by zero") {}
		};

		//! specify the distribution for randomization functions
		class RandomizationParameter
		{
		public:
			RandomizationParameter(unsigned int coefficientCount, const typename T::RandomizationParameter &coefficientParameter )
				: m_coefficientCount(coefficientCount), m_coefficientParameter(coefficientParameter) {}

		private:
			unsigned int m_coefficientCount;
			typename T::RandomizationParameter m_coefficientParameter;
			friend class PolynomialOver<T>;
		};

		typedef T Ring;
		typedef typename T::Element CoefficientType;
	//@}

	//! \name CREATORS
	//@{
		//! creates the zero polynomial
		PolynomialOver() {}

		//!
		PolynomialOver(const Ring &ring, unsigned int count)
			: m_coefficients((size_t)count, ring.Identity()) {}

		//! copy constructor
		PolynomialOver(const PolynomialOver<Ring> &t)
			: m_coefficients(t.m_coefficients.size()) {*this = t;}

		//! construct constant polynomial
		PolynomialOver(const CoefficientType &element)
			: m_coefficients(1, element) {}

		//! construct polynomial with specified coefficients, starting from coefficient of x^0
		template <typename Iterator> PolynomialOver(Iterator begin, Iterator end)
			: m_coefficients(begin, end) {}

		//! convert from string
		PolynomialOver(const char *str, const Ring &ring) {FromStr(str, ring);}

		//! convert from big-endian byte array
		PolynomialOver(const byte *encodedPolynomialOver, unsigned int byteCount);

		//! convert from Basic Encoding Rules encoded byte array
		explicit PolynomialOver(const byte *BEREncodedPolynomialOver);

		//! convert from BER encoded byte array stored in a BufferedTransformation object
		explicit PolynomialOver(BufferedTransformation &bt);

		//! create a random PolynomialOver<T>
		PolynomialOver(RandomNumberGenerator &rng, const RandomizationParameter &parameter, const Ring &ring)
			{Randomize(rng, parameter, ring);}
	//@}

	//! \name ACCESSORS
	//@{
		//! the zero polynomial will return a degree of -1
		int Degree(const Ring &ring) const {return int(CoefficientCount(ring))-1;}
		//!
		unsigned int CoefficientCount(const Ring &ring) const;
		//! return coefficient for x^i
		CoefficientType GetCoefficient(unsigned int i, const Ring &ring) const;
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		PolynomialOver<Ring>&  operator=(const PolynomialOver<Ring>& t);

		//!
		void Randomize(RandomNumberGenerator &rng, const RandomizationParameter &parameter, const Ring &ring);

		//! set the coefficient for x^i to value
		void SetCoefficient(unsigned int i, const CoefficientType &value, const Ring &ring);

		//!
		void Negate(const Ring &ring);

		//!
		void swap(PolynomialOver<Ring> &t);
	//@}


	//! \name BASIC ARITHMETIC ON POLYNOMIALS
	//@{
		bool Equals(const PolynomialOver<Ring> &t, const Ring &ring) const;
		bool IsZero(const Ring &ring) const {return CoefficientCount(ring)==0;}

		PolynomialOver<Ring> Plus(const PolynomialOver<Ring>& t, const Ring &ring) const;
		PolynomialOver<Ring> Minus(const PolynomialOver<Ring>& t, const Ring &ring) const;
		PolynomialOver<Ring> Inverse(const Ring &ring) const;

		PolynomialOver<Ring> Times(const PolynomialOver<Ring>& t, const Ring &ring) const;
		PolynomialOver<Ring> DividedBy(const PolynomialOver<Ring>& t, const Ring &ring) const;
		PolynomialOver<Ring> Modulo(const PolynomialOver<Ring>& t, const Ring &ring) const;
		PolynomialOver<Ring> MultiplicativeInverse(const Ring &ring) const;
		bool IsUnit(const Ring &ring) const;

		PolynomialOver<Ring>& Accumulate(const PolynomialOver<Ring>& t, const Ring &ring);
		PolynomialOver<Ring>& Reduce(const PolynomialOver<Ring>& t, const Ring &ring);

		//!
		PolynomialOver<Ring> Doubled(const Ring &ring) const {return Plus(*this, ring);}
		//!
		PolynomialOver<Ring> Squared(const Ring &ring) const {return Times(*this, ring);}

		CoefficientType EvaluateAt(const CoefficientType &x, const Ring &ring) const;

		PolynomialOver<Ring>& ShiftLeft(unsigned int n, const Ring &ring);
		PolynomialOver<Ring>& ShiftRight(unsigned int n, const Ring &ring);

		//! calculate r and q such that (a == d*q + r) && (0 <= degree of r < degree of d)
		static void Divide(PolynomialOver<Ring> &r, PolynomialOver<Ring> &q, const PolynomialOver<Ring> &a, const PolynomialOver<Ring> &d, const Ring &ring);
	//@}

	//! \name INPUT/OUTPUT
	//@{
		std::istream& Input(std::istream &in, const Ring &ring);
		std::ostream& Output(std::ostream &out, const Ring &ring) const;
	//@}

private:
	void FromStr(const char *str, const Ring &ring);

	std::vector<CoefficientType> m_coefficients;
};

//! Polynomials over a fixed ring
/*! Having a fixed ring allows overloaded operators */
template <class T, int instance> class PolynomialOverFixedRing : private PolynomialOver<T>
{
	typedef PolynomialOver<T> B;
	typedef PolynomialOverFixedRing<T, instance> ThisType;

public:
	typedef T Ring;
	typedef typename T::Element CoefficientType;
	typedef typename B::DivideByZero DivideByZero;
	typedef typename B::RandomizationParameter RandomizationParameter;

	//! \name CREATORS
	//@{
		//! creates the zero polynomial
		PolynomialOverFixedRing(unsigned int count = 0) : B(ms_fixedRing, count) {}

		//! copy constructor
		PolynomialOverFixedRing(const ThisType &t) : B(t) {}

		explicit PolynomialOverFixedRing(const B &t) : B(t) {}

		//! construct constant polynomial
		PolynomialOverFixedRing(const CoefficientType &element) : B(element) {}

		//! construct polynomial with specified coefficients, starting from coefficient of x^0
		template <typename Iterator> PolynomialOverFixedRing(Iterator first, Iterator last)
			: B(first, last) {}

		//! convert from string
		explicit PolynomialOverFixedRing(const char *str) : B(str, ms_fixedRing) {}

		//! convert from big-endian byte array
		PolynomialOverFixedRing(const byte *encodedPoly, unsigned int byteCount) : B(encodedPoly, byteCount) {}

		//! convert from Basic Encoding Rules encoded byte array
		explicit PolynomialOverFixedRing(const byte *BEREncodedPoly) : B(BEREncodedPoly) {}

		//! convert from BER encoded byte array stored in a BufferedTransformation object
		explicit PolynomialOverFixedRing(BufferedTransformation &bt) : B(bt) {}

		//! create a random PolynomialOverFixedRing
		PolynomialOverFixedRing(RandomNumberGenerator &rng, const RandomizationParameter &parameter) : B(rng, parameter, ms_fixedRing) {}

		static const ThisType &Zero();
		static const ThisType &One();
	//@}

	//! \name ACCESSORS
	//@{
		//! the zero polynomial will return a degree of -1
		int Degree() const {return B::Degree(ms_fixedRing);}
		//! degree + 1
		unsigned int CoefficientCount() const {return B::CoefficientCount(ms_fixedRing);}
		//! return coefficient for x^i
		CoefficientType GetCoefficient(unsigned int i) const {return B::GetCoefficient(i, ms_fixedRing);}
		//! return coefficient for x^i
		CoefficientType operator[](unsigned int i) const {return B::GetCoefficient(i, ms_fixedRing);}
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		ThisType&  operator=(const ThisType& t) {B::operator=(t); return *this;}
		//!
		ThisType&  operator+=(const ThisType& t) {Accumulate(t, ms_fixedRing); return *this;}
		//!
		ThisType&  operator-=(const ThisType& t) {Reduce(t, ms_fixedRing); return *this;}
		//!
		ThisType&  operator*=(const ThisType& t) {return *this = *this*t;}
		//!
		ThisType&  operator/=(const ThisType& t) {return *this = *this/t;}
		//!
		ThisType&  operator%=(const ThisType& t) {return *this = *this%t;}

		//!
		ThisType&  operator<<=(unsigned int n) {ShiftLeft(n, ms_fixedRing); return *this;}
		//!
		ThisType&  operator>>=(unsigned int n) {ShiftRight(n, ms_fixedRing); return *this;}

		//! set the coefficient for x^i to value
		void SetCoefficient(unsigned int i, const CoefficientType &value) {B::SetCoefficient(i, value, ms_fixedRing);}

		//!
		void Randomize(RandomNumberGenerator &rng, const RandomizationParameter &parameter) {B::Randomize(rng, parameter, ms_fixedRing);}

		//!
		void Negate() {B::Negate(ms_fixedRing);}

		void swap(ThisType &t) {B::swap(t);}
	//@}

	//! \name UNARY OPERATORS
	//@{
		//!
		bool operator!() const {return CoefficientCount()==0;}
		//!
		ThisType operator+() const {return *this;}
		//!
		ThisType operator-() const {return ThisType(Inverse(ms_fixedRing));}
	//@}

	//! \name BINARY OPERATORS
	//@{
		//!
		friend ThisType operator>>(ThisType a, unsigned int n)	{return ThisType(a>>=n);}
		//!
		friend ThisType operator<<(ThisType a, unsigned int n)	{return ThisType(a<<=n);}
	//@}

	//! \name OTHER ARITHMETIC FUNCTIONS
	//@{
		//!
		ThisType MultiplicativeInverse() const {return ThisType(B::MultiplicativeInverse(ms_fixedRing));}
		//!
		bool IsUnit() const {return B::IsUnit(ms_fixedRing);}

		//!
		ThisType Doubled() const {return ThisType(B::Doubled(ms_fixedRing));}
		//!
		ThisType Squared() const {return ThisType(B::Squared(ms_fixedRing));}

		CoefficientType EvaluateAt(const CoefficientType &x) const {return B::EvaluateAt(x, ms_fixedRing);}

		//! calculate r and q such that (a == d*q + r) && (0 <= r < abs(d))
		static void Divide(ThisType &r, ThisType &q, const ThisType &a, const ThisType &d)
			{B::Divide(r, q, a, d, ms_fixedRing);}
	//@}

	//! \name INPUT/OUTPUT
	//@{
		//!
		friend std::istream& operator>>(std::istream& in, ThisType &a)
			{return a.Input(in, ms_fixedRing);}
		//!
		friend std::ostream& operator<<(std::ostream& out, const ThisType &a)
			{return a.Output(out, ms_fixedRing);}
	//@}

private:
	struct NewOnePolynomial
	{
		ThisType * operator()() const
		{
			return new ThisType(ms_fixedRing.MultiplicativeIdentity());
		}
	};

	static const Ring ms_fixedRing;
};

//! Ring of polynomials over another ring
template <class T> class RingOfPolynomialsOver : public AbstractEuclideanDomain<PolynomialOver<T> >
{
public:
	typedef T CoefficientRing;
	typedef PolynomialOver<T> Element;
	typedef typename Element::CoefficientType CoefficientType;
	typedef typename Element::RandomizationParameter RandomizationParameter;

	RingOfPolynomialsOver(const CoefficientRing &ring) : m_ring(ring) {}

	Element RandomElement(RandomNumberGenerator &rng, const RandomizationParameter &parameter)
		{return Element(rng, parameter, m_ring);}

	bool Equal(const Element &a, const Element &b) const
		{return a.Equals(b, m_ring);}

	const Element& Identity() const
		{return this->result = m_ring.Identity();}

	const Element& Add(const Element &a, const Element &b) const
		{return this->result = a.Plus(b, m_ring);}

	Element& Accumulate(Element &a, const Element &b) const
		{a.Accumulate(b, m_ring); return a;}

	const Element& Inverse(const Element &a) const
		{return this->result = a.Inverse(m_ring);}

	const Element& Subtract(const Element &a, const Element &b) const
		{return this->result = a.Minus(b, m_ring);}

	Element& Reduce(Element &a, const Element &b) const
		{return a.Reduce(b, m_ring);}

	const Element& Double(const Element &a) const
		{return this->result = a.Doubled(m_ring);}

	const Element& MultiplicativeIdentity() const
		{return this->result = m_ring.MultiplicativeIdentity();}

	const Element& Multiply(const Element &a, const Element &b) const
		{return this->result = a.Times(b, m_ring);}

	const Element& Square(const Element &a) const
		{return this->result = a.Squared(m_ring);}

	bool IsUnit(const Element &a) const
		{return a.IsUnit(m_ring);}

	const Element& MultiplicativeInverse(const Element &a) const
		{return this->result = a.MultiplicativeInverse(m_ring);}

	const Element& Divide(const Element &a, const Element &b) const
		{return this->result = a.DividedBy(b, m_ring);}

	const Element& Mod(const Element &a, const Element &b) const
		{return this->result = a.Modulo(b, m_ring);}

	void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const
		{Element::Divide(r, q, a, d, m_ring);}

	class InterpolationFailed : public Exception
	{
	public:
		InterpolationFailed() : Exception(OTHER_ERROR, "RingOfPolynomialsOver<T>: interpolation failed") {}
	};

	Element Interpolate(const CoefficientType x[], const CoefficientType y[], unsigned int n) const;

	// a faster version of Interpolate(x, y, n).EvaluateAt(position)
	CoefficientType InterpolateAt(const CoefficientType &position, const CoefficientType x[], const CoefficientType y[], unsigned int n) const;
/*
	void PrepareBulkInterpolation(CoefficientType *w, const CoefficientType x[], unsigned int n) const;
	void PrepareBulkInterpolationAt(CoefficientType *v, const CoefficientType &position, const CoefficientType x[], const CoefficientType w[], unsigned int n) const;
	CoefficientType BulkInterpolateAt(const CoefficientType y[], const CoefficientType v[], unsigned int n) const;
*/
protected:
	void CalculateAlpha(std::vector<CoefficientType> &alpha, const CoefficientType x[], const CoefficientType y[], unsigned int n) const;

	CoefficientRing m_ring;
};

template <class Ring, class Element>
void PrepareBulkPolynomialInterpolation(const Ring &ring, Element *w, const Element x[], unsigned int n);
template <class Ring, class Element>
void PrepareBulkPolynomialInterpolationAt(const Ring &ring, Element *v, const Element &position, const Element x[], const Element w[], unsigned int n);
template <class Ring, class Element>
Element BulkPolynomialInterpolateAt(const Ring &ring, const Element y[], const Element v[], unsigned int n);

//!
template <class T, int instance>
inline bool operator==(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return a.Equals(b, a.ms_fixedRing);}
//!
template <class T, int instance>
inline bool operator!=(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return !(a==b);}

//!
template <class T, int instance>
inline bool operator> (const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return a.Degree() > b.Degree();}
//!
template <class T, int instance>
inline bool operator>=(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return a.Degree() >= b.Degree();}
//!
template <class T, int instance>
inline bool operator< (const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return a.Degree() < b.Degree();}
//!
template <class T, int instance>
inline bool operator<=(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return a.Degree() <= b.Degree();}

//!
template <class T, int instance>
inline CryptoPP::PolynomialOverFixedRing<T, instance> operator+(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return CryptoPP::PolynomialOverFixedRing<T, instance>(a.Plus(b, a.ms_fixedRing));}
//!
template <class T, int instance>
inline CryptoPP::PolynomialOverFixedRing<T, instance> operator-(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return CryptoPP::PolynomialOverFixedRing<T, instance>(a.Minus(b, a.ms_fixedRing));}
//!
template <class T, int instance>
inline CryptoPP::PolynomialOverFixedRing<T, instance> operator*(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return CryptoPP::PolynomialOverFixedRing<T, instance>(a.Times(b, a.ms_fixedRing));}
//!
template <class T, int instance>
inline CryptoPP::PolynomialOverFixedRing<T, instance> operator/(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return CryptoPP::PolynomialOverFixedRing<T, instance>(a.DividedBy(b, a.ms_fixedRing));}
//!
template <class T, int instance>
inline CryptoPP::PolynomialOverFixedRing<T, instance> operator%(const CryptoPP::PolynomialOverFixedRing<T, instance> &a, const CryptoPP::PolynomialOverFixedRing<T, instance> &b)
	{return CryptoPP::PolynomialOverFixedRing<T, instance>(a.Modulo(b, a.ms_fixedRing));}

NAMESPACE_END

NAMESPACE_BEGIN(std)
template<class T> inline void swap(CryptoPP::PolynomialOver<T> &a, CryptoPP::PolynomialOver<T> &b)
{
	a.swap(b);
}
template<class T, int i> inline void swap(CryptoPP::PolynomialOverFixedRing<T,i> &a, CryptoPP::PolynomialOverFixedRing<T,i> &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
