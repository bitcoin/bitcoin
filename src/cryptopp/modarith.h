// modarith.h - written and placed in the public domain by Wei Dai

//! \file modarith.h
//! \brief Class file for performing modular arithmetic.

#ifndef CRYPTOPP_MODARITH_H
#define CRYPTOPP_MODARITH_H

// implementations are in integer.cpp

#include "cryptlib.h"
#include "integer.h"
#include "algebra.h"
#include "secblock.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

CRYPTOPP_DLL_TEMPLATE_CLASS AbstractGroup<Integer>;
CRYPTOPP_DLL_TEMPLATE_CLASS AbstractRing<Integer>;
CRYPTOPP_DLL_TEMPLATE_CLASS AbstractEuclideanDomain<Integer>;

//! \class ModularArithmetic
//! \brief Ring of congruence classes modulo n
//! \details This implementation represents each congruence class as the smallest
//!   non-negative integer in that class.
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
class CRYPTOPP_DLL ModularArithmetic : public AbstractRing<Integer>
{
public:

	typedef int RandomizationParameter;
	typedef Integer Element;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ModularArithmetic() {}
#endif

	//! \brief Construct a ModularArithmetic
	//! \param modulus congruence class modulus
	ModularArithmetic(const Integer &modulus = Integer::One())
		: AbstractRing<Integer>(), m_modulus(modulus), m_result((word)0, modulus.reg.size()) {}

	//! \brief Copy construct a ModularArithmetic
	//! \param ma other ModularArithmetic
	ModularArithmetic(const ModularArithmetic &ma)
		: AbstractRing<Integer>(), m_modulus(ma.m_modulus), m_result((word)0, ma.m_modulus.reg.size()) {}

	//! \brief Construct a ModularArithmetic
	//! \param bt BER encoded ModularArithmetic
	ModularArithmetic(BufferedTransformation &bt);	// construct from BER encoded parameters

	//! \brief Clone a ModularArithmetic
	//! \returns pointer to a new ModularArithmetic
	//! \details Clone effectively copy constructs a new ModularArithmetic. The caller is
	//!   responsible for deleting the pointer returned from this method.
	virtual ModularArithmetic * Clone() const {return new ModularArithmetic(*this);}

	//! \brief Encodes in DER format
	//! \param bt BufferedTransformation object
	void DEREncode(BufferedTransformation &bt) const;

	//! \brief Encodes element in DER format
	//! \param out BufferedTransformation object
	//! \param a Element to encode
	void DEREncodeElement(BufferedTransformation &out, const Element &a) const;

	//! \brief Decodes element in DER format
	//! \param in BufferedTransformation object
	//! \param a Element to decode
	void BERDecodeElement(BufferedTransformation &in, Element &a) const;

	//! \brief Retrieves the modulus
	//! \returns the modulus
	const Integer& GetModulus() const {return m_modulus;}

	//! \brief Sets the modulus
	//! \param newModulus the new modulus
	void SetModulus(const Integer &newModulus)
		{m_modulus = newModulus; m_result.reg.resize(m_modulus.reg.size());}

	//! \brief Retrieves the representation
	//! \returns true if the representation is MontgomeryRepresentation, false otherwise
	virtual bool IsMontgomeryRepresentation() const {return false;}

	//! \brief Reduces an element in the congruence class
	//! \param a element to convert
	//! \returns the reduced element
	//! \details ConvertIn is useful for derived classes, like MontgomeryRepresentation, which
	//!   must convert between representations.
	virtual Integer ConvertIn(const Integer &a) const
		{return a%m_modulus;}

	//! \brief Reduces an element in the congruence class
	//! \param a element to convert
	//! \returns the reduced element
	//! \details ConvertOut is useful for derived classes, like MontgomeryRepresentation, which
	//!   must convert between representations.
	virtual Integer ConvertOut(const Integer &a) const
		{return a;}

	//! \brief TODO
	//! \param a element to convert
	const Integer& Half(const Integer &a) const;

	//! \brief Compare two elements for equality
	//! \param a first element
	//! \param b second element
	//! \returns true if the elements are equal, false otherwise
	//! \details Equal() tests the elements for equality using <tt>a==b</tt>
	bool Equal(const Integer &a, const Integer &b) const
		{return a==b;}

	//! \brief Provides the Identity element
	//! \returns the Identity element
	const Integer& Identity() const
		{return Integer::Zero();}

	//! \brief Adds elements in the ring
	//! \param a first element
	//! \param b second element
	//! \returns the sum of <tt>a</tt> and <tt>b</tt>
	const Integer& Add(const Integer &a, const Integer &b) const;

	//! \brief TODO
	//! \param a first element
	//! \param b second element
	//! \returns TODO
	Integer& Accumulate(Integer &a, const Integer &b) const;

	//! \brief Inverts the element in the ring
	//! \param a first element
	//! \returns the inverse of the element
	const Integer& Inverse(const Integer &a) const;

	//! \brief Subtracts elements in the ring
	//! \param a first element
	//! \param b second element
	//! \returns the difference of <tt>a</tt> and <tt>b</tt>. The element <tt>a</tt> must provide a Subtract member function.
	const Integer& Subtract(const Integer &a, const Integer &b) const;

	//! \brief TODO
	//! \param a first element
	//! \param b second element
	//! \returns TODO
	Integer& Reduce(Integer &a, const Integer &b) const;

	//! \brief Doubles an element in the ring
	//! \param a the element
	//! \returns the element doubled
	//! \details Double returns <tt>Add(a, a)</tt>. The element <tt>a</tt> must provide an Add member function.
	const Integer& Double(const Integer &a) const
		{return Add(a, a);}

	//! \brief Retrieves the multiplicative identity
	//! \returns the multiplicative identity
	//! \details the base class implementations returns 1.
	const Integer& MultiplicativeIdentity() const
		{return Integer::One();}

	//! \brief Multiplies elements in the ring
	//! \param a the multiplicand
	//! \param b the multiplier
	//! \returns the product of a and b
	//! \details Multiply returns <tt>a*b\%n</tt>.
	const Integer& Multiply(const Integer &a, const Integer &b) const
		{return m_result1 = a*b%m_modulus;}

	//! \brief Square an element in the ring
	//! \param a the element
	//! \returns the element squared
	//! \details Square returns <tt>a*a\%n</tt>. The element <tt>a</tt> must provide a Square member function.
	const Integer& Square(const Integer &a) const
		{return m_result1 = a.Squared()%m_modulus;}

	//! \brief Determines whether an element is a unit in the ring
	//! \param a the element
	//! \returns true if the element is a unit after reduction, false otherwise.
	bool IsUnit(const Integer &a) const
		{return Integer::Gcd(a, m_modulus).IsUnit();}

	//! \brief Calculate the multiplicative inverse of an element in the ring
	//! \param a the element
	//! \details MultiplicativeInverse returns <tt>a<sup>-1</sup>\%n</tt>. The element <tt>a</tt> must
	//!   provide a InverseMod member function.
	const Integer& MultiplicativeInverse(const Integer &a) const
		{return m_result1 = a.InverseMod(m_modulus);}

	//! \brief Divides elements in the ring
	//! \param a the dividend
	//! \param b the divisor
	//! \returns the quotient
	//! \details Divide returns <tt>a*b<sup>-1</sup>\%n</tt>.
	const Integer& Divide(const Integer &a, const Integer &b) const
		{return Multiply(a, MultiplicativeInverse(b));}

	//! \brief TODO
	//! \param x first element
	//! \param e1 first exponent
	//! \param y second element
	//! \param e2 second exponent
	//! \returns TODO
	Integer CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const;

	//! \brief Exponentiates a base to multiple exponents in the ring
	//! \param results an array of Elements
	//! \param base the base to raise to the exponents
	//! \param exponents an array of exponents
	//! \param exponentsCount the number of exponents in the array
	//! \details SimultaneousExponentiate() raises the base to each exponent in the exponents array and stores the
	//!   result at the respective position in the results array.
	//! \details SimultaneousExponentiate() must be implemented in a derived class.
	//! \pre <tt>COUNTOF(results) == exponentsCount</tt>
	//! \pre <tt>COUNTOF(exponents) == exponentsCount</tt>
	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	//! \brief Provides the maximum bit size of an element in the ring
	//! \returns maximum bit size of an element
	unsigned int MaxElementBitLength() const
		{return (m_modulus-1).BitCount();}

	//! \brief Provides the maximum byte size of an element in the ring
	//! \returns maximum byte size of an element
	unsigned int MaxElementByteLength() const
		{return (m_modulus-1).ByteCount();}

	//! \brief Provides a random element in the ring
	//! \param rng RandomNumberGenerator used to generate material
	//! \param ignore_for_now unused
	//! \returns a random element that is uniformly distributed
	//! \details RandomElement constructs a new element in the range <tt>[0,n-1]</tt>, inclusive.
	//!   The element's class must provide a constructor with the signature <tt>Element(RandomNumberGenerator rng,
	//!   Element min, Element max)</tt>.
	Element RandomElement( RandomNumberGenerator &rng , const RandomizationParameter &ignore_for_now = 0) const
		// left RandomizationParameter arg as ref in case RandomizationParameter becomes a more complicated struct
	{
		CRYPTOPP_UNUSED(ignore_for_now);
		return Element(rng, Integer::Zero(), m_modulus - Integer::One()) ;
	}

	//! \brief Compares two ModularArithmetic for equality
	//! \param rhs other ModularArithmetic
	//! \returns true if this is equal to the other, false otherwise
	//! \details The operator tests for equality using <tt>this.m_modulus == rhs.m_modulus</tt>.
	bool operator==(const ModularArithmetic &rhs) const
		{return m_modulus == rhs.m_modulus;}

	static const RandomizationParameter DefaultRandomizationParameter ;

protected:
	Integer m_modulus;
	mutable Integer m_result, m_result1;
};

// const ModularArithmetic::RandomizationParameter ModularArithmetic::DefaultRandomizationParameter = 0 ;

//! \class MontgomeryRepresentation
//! \brief Performs modular arithmetic in Montgomery representation for increased speed
//! \details The Montgomery representation represents each congruence class <tt>[a]</tt> as
//!   <tt>a*r\%n</tt>, where <tt>r</tt> is a convenient power of 2.
//! \details <tt>const Element&</tt> returned by member functions are references
//!   to internal data members. Since each object may have only
//!   one such data member for holding results, the following code
//!   will produce incorrect results:
//!   <pre>    abcd = group.Add(group.Add(a,b), group.Add(c,d));</pre>
//!   But this should be fine:
//!   <pre>    abcd = group.Add(a, group.Add(b, group.Add(c,d));</pre>
class CRYPTOPP_DLL MontgomeryRepresentation : public ModularArithmetic
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~MontgomeryRepresentation() {}
#endif

	//! \brief Construct a IsMontgomeryRepresentation
	//! \param modulus congruence class modulus
	//! \note The modulus must be odd.
	MontgomeryRepresentation(const Integer &modulus);

	//! \brief Clone a MontgomeryRepresentation
	//! \returns pointer to a new MontgomeryRepresentation
	//! \details Clone effectively copy constructs a new MontgomeryRepresentation. The caller is
	//!   responsible for deleting the pointer returned from this method.
	virtual ModularArithmetic * Clone() const {return new MontgomeryRepresentation(*this);}

	bool IsMontgomeryRepresentation() const {return true;}

	Integer ConvertIn(const Integer &a) const
		{return (a<<(WORD_BITS*m_modulus.reg.size()))%m_modulus;}

	Integer ConvertOut(const Integer &a) const;

	const Integer& MultiplicativeIdentity() const
		{return m_result1 = Integer::Power2(WORD_BITS*m_modulus.reg.size())%m_modulus;}

	const Integer& Multiply(const Integer &a, const Integer &b) const;

	const Integer& Square(const Integer &a) const;

	const Integer& MultiplicativeInverse(const Integer &a) const;

	Integer CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const
		{return AbstractRing<Integer>::CascadeExponentiate(x, e1, y, e2);}

	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
		{AbstractRing<Integer>::SimultaneousExponentiate(results, base, exponents, exponentsCount);}

private:
	Integer m_u;
	mutable IntegerSecBlock m_workspace;
};

NAMESPACE_END

#endif
