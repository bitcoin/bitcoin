#ifndef CRYPTOPP_XTR_H
#define CRYPTOPP_XTR_H

//! \file xtr.h
//! \brief The XTR public key system
//! \details The XTR public key system by Arjen K. Lenstra and Eric R. Verheul

#include "cryptlib.h"
#include "modarith.h"
#include "integer.h"
#include "algebra.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class GFP2Element
//! \brief an element of GF(p^2)
class GFP2Element
{
public:
	GFP2Element() {}
	GFP2Element(const Integer &c1, const Integer &c2) : c1(c1), c2(c2) {}
	GFP2Element(const byte *encodedElement, unsigned int size)
		: c1(encodedElement, size/2), c2(encodedElement+size/2, size/2) {}

	void Encode(byte *encodedElement, unsigned int size)
	{
		c1.Encode(encodedElement, size/2);
		c2.Encode(encodedElement+size/2, size/2);
	}

	bool operator==(const GFP2Element &rhs)	const {return c1 == rhs.c1 && c2 == rhs.c2;}
	bool operator!=(const GFP2Element &rhs) const {return !operator==(rhs);}

	void swap(GFP2Element &a)
	{
		c1.swap(a.c1);
		c2.swap(a.c2);
	}

	static const GFP2Element & Zero();

	Integer c1, c2;
};

//! \class GFP2_ONB
//! \brief GF(p^2), optimal normal basis
template <class F>
class GFP2_ONB : public AbstractRing<GFP2Element>
{
public:
	typedef F BaseField;

	GFP2_ONB(const Integer &p) : modp(p)
	{
		if (p%3 != 2)
			throw InvalidArgument("GFP2_ONB: modulus must be equivalent to 2 mod 3");
	}

	const Integer& GetModulus() const {return modp.GetModulus();}

	GFP2Element ConvertIn(const Integer &a) const
	{
		t = modp.Inverse(modp.ConvertIn(a));
		return GFP2Element(t, t);
	}

	GFP2Element ConvertIn(const GFP2Element &a) const
		{return GFP2Element(modp.ConvertIn(a.c1), modp.ConvertIn(a.c2));}

	GFP2Element ConvertOut(const GFP2Element &a) const
		{return GFP2Element(modp.ConvertOut(a.c1), modp.ConvertOut(a.c2));}

	bool Equal(const GFP2Element &a, const GFP2Element &b) const
	{
		return modp.Equal(a.c1, b.c1) && modp.Equal(a.c2, b.c2);
	}

	const Element& Identity() const
	{
		return GFP2Element::Zero();
	}

	const Element& Add(const Element &a, const Element &b) const
	{
		result.c1 = modp.Add(a.c1, b.c1);
		result.c2 = modp.Add(a.c2, b.c2);
		return result;
	}

	const Element& Inverse(const Element &a) const
	{
		result.c1 = modp.Inverse(a.c1);
		result.c2 = modp.Inverse(a.c2);
		return result;
	}

	const Element& Double(const Element &a) const
	{
		result.c1 = modp.Double(a.c1);
		result.c2 = modp.Double(a.c2);
		return result;
	}

	const Element& Subtract(const Element &a, const Element &b) const
	{
		result.c1 = modp.Subtract(a.c1, b.c1);
		result.c2 = modp.Subtract(a.c2, b.c2);
		return result;
	}

	Element& Accumulate(Element &a, const Element &b) const
	{
		modp.Accumulate(a.c1, b.c1);
		modp.Accumulate(a.c2, b.c2);
		return a;
	}

	Element& Reduce(Element &a, const Element &b) const
	{
		modp.Reduce(a.c1, b.c1);
		modp.Reduce(a.c2, b.c2);
		return a;
	}

	bool IsUnit(const Element &a) const
	{
		return a.c1.NotZero() || a.c2.NotZero();
	}

	const Element& MultiplicativeIdentity() const
	{
		result.c1 = result.c2 = modp.Inverse(modp.MultiplicativeIdentity());
		return result;
	}

	const Element& Multiply(const Element &a, const Element &b) const
	{
		t = modp.Add(a.c1, a.c2);
		t = modp.Multiply(t, modp.Add(b.c1, b.c2));
		result.c1 = modp.Multiply(a.c1, b.c1);
		result.c2 = modp.Multiply(a.c2, b.c2);
		result.c1.swap(result.c2);
		modp.Reduce(t, result.c1);
		modp.Reduce(t, result.c2);
		modp.Reduce(result.c1, t);
		modp.Reduce(result.c2, t);
		return result;
	}

	const Element& MultiplicativeInverse(const Element &a) const
	{
		return result = Exponentiate(a, modp.GetModulus()-2);
	}

	const Element& Square(const Element &a) const
	{
		const Integer &ac1 = (&a == &result) ? (t = a.c1) : a.c1;
		result.c1 = modp.Multiply(modp.Subtract(modp.Subtract(a.c2, a.c1), a.c1), a.c2);
		result.c2 = modp.Multiply(modp.Subtract(modp.Subtract(ac1, a.c2), a.c2), ac1);
		return result;
	}

	Element Exponentiate(const Element &a, const Integer &e) const
	{
		Integer edivp, emodp;
		Integer::Divide(emodp, edivp, e, modp.GetModulus());
		Element b = PthPower(a);
		return AbstractRing<GFP2Element>::CascadeExponentiate(a, emodp, b, edivp);
	}

	const Element & PthPower(const Element &a) const
	{
		result = a;
		result.c1.swap(result.c2);
		return result;
	}

	void RaiseToPthPower(Element &a) const
	{
		a.c1.swap(a.c2);
	}

	// a^2 - 2a^p
	const Element & SpecialOperation1(const Element &a) const
	{
		CRYPTOPP_ASSERT(&a != &result);
		result = Square(a);
		modp.Reduce(result.c1, a.c2);
		modp.Reduce(result.c1, a.c2);
		modp.Reduce(result.c2, a.c1);
		modp.Reduce(result.c2, a.c1);
		return result;
	}

	// x * z - y * z^p
	const Element & SpecialOperation2(const Element &x, const Element &y, const Element &z) const
	{
		CRYPTOPP_ASSERT(&x != &result && &y != &result && &z != &result);
		t = modp.Add(x.c2, y.c2);
		result.c1 = modp.Multiply(z.c1, modp.Subtract(y.c1, t));
		modp.Accumulate(result.c1, modp.Multiply(z.c2, modp.Subtract(t, x.c1)));
		t = modp.Add(x.c1, y.c1);
		result.c2 = modp.Multiply(z.c2, modp.Subtract(y.c2, t));
		modp.Accumulate(result.c2, modp.Multiply(z.c1, modp.Subtract(t, x.c2)));
		return result;
	}

protected:
	BaseField modp;
	mutable GFP2Element result;
	mutable Integer t;
};

//! \brief Creates primes p,q and generator g for XTR
void XTR_FindPrimesAndGenerator(RandomNumberGenerator &rng, Integer &p, Integer &q, GFP2Element &g, unsigned int pbits, unsigned int qbits);

GFP2Element XTR_Exponentiate(const GFP2Element &b, const Integer &e, const Integer &p);

NAMESPACE_END

#endif
