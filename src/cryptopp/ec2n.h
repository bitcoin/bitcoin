// ec2n.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile ec2n.h
//! \brief Classes for Elliptic Curves over binary fields


#ifndef CRYPTOPP_EC2N_H
#define CRYPTOPP_EC2N_H

#include "cryptlib.h"
#include "gf2n.h"
#include "integer.h"
#include "algebra.h"
#include "eprecomp.h"
#include "smartptr.h"
#include "pubkey.h"

NAMESPACE_BEGIN(CryptoPP)

//! Elliptic Curve Point
struct CRYPTOPP_DLL EC2NPoint
{
	EC2NPoint() : identity(true) {}
	EC2NPoint(const PolynomialMod2 &x, const PolynomialMod2 &y)
		: identity(false), x(x), y(y) {}

	bool operator==(const EC2NPoint &t) const
		{return (identity && t.identity) || (!identity && !t.identity && x==t.x && y==t.y);}
	bool operator< (const EC2NPoint &t) const
		{return identity ? !t.identity : (!t.identity && (x<t.x || (x==t.x && y<t.y)));}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~EC2NPoint() {}
#endif

	bool identity;
	PolynomialMod2 x, y;
};

CRYPTOPP_DLL_TEMPLATE_CLASS AbstractGroup<EC2NPoint>;

//! Elliptic Curve over GF(2^n)
class CRYPTOPP_DLL EC2N : public AbstractGroup<EC2NPoint>
{
public:
	typedef GF2NP Field;
	typedef Field::Element FieldElement;
	typedef EC2NPoint Point;

	EC2N() {}
	EC2N(const Field &field, const Field::Element &a, const Field::Element &b)
		: m_field(field), m_a(a), m_b(b) {}
	// construct from BER encoded parameters
	// this constructor will decode and extract the the fields fieldID and curve of the sequence ECParameters
	EC2N(BufferedTransformation &bt);

	// encode the fields fieldID and curve of the sequence ECParameters
	void DEREncode(BufferedTransformation &bt) const;

	bool Equal(const Point &P, const Point &Q) const;
	const Point& Identity() const;
	const Point& Inverse(const Point &P) const;
	bool InversionIsFast() const {return true;}
	const Point& Add(const Point &P, const Point &Q) const;
	const Point& Double(const Point &P) const;

	Point Multiply(const Integer &k, const Point &P) const
		{return ScalarMultiply(P, k);}
	Point CascadeMultiply(const Integer &k1, const Point &P, const Integer &k2, const Point &Q) const
		{return CascadeScalarMultiply(P, k1, Q, k2);}

	bool ValidateParameters(RandomNumberGenerator &rng, unsigned int level=3) const;
	bool VerifyPoint(const Point &P) const;

	unsigned int EncodedPointSize(bool compressed = false) const
		{return 1 + (compressed?1:2)*m_field->MaxElementByteLength();}
	// returns false if point is compressed and not valid (doesn't check if uncompressed)
	bool DecodePoint(Point &P, BufferedTransformation &bt, size_t len) const;
	bool DecodePoint(Point &P, const byte *encodedPoint, size_t len) const;
	void EncodePoint(byte *encodedPoint, const Point &P, bool compressed) const;
	void EncodePoint(BufferedTransformation &bt, const Point &P, bool compressed) const;

	Point BERDecodePoint(BufferedTransformation &bt) const;
	void DEREncodePoint(BufferedTransformation &bt, const Point &P, bool compressed) const;

	Integer FieldSize() const {return Integer::Power2(m_field->MaxElementBitLength());}
	const Field & GetField() const {return *m_field;}
	const FieldElement & GetA() const {return m_a;}
	const FieldElement & GetB() const {return m_b;}

	bool operator==(const EC2N &rhs) const
		{return GetField() == rhs.GetField() && m_a == rhs.m_a && m_b == rhs.m_b;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~EC2N() {}
#endif

private:
	clonable_ptr<Field> m_field;
	FieldElement m_a, m_b;
	mutable Point m_R;
};

CRYPTOPP_DLL_TEMPLATE_CLASS DL_FixedBasePrecomputationImpl<EC2N::Point>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupPrecomputation<EC2N::Point>;

template <class T> class EcPrecomputation;

//! EC2N precomputation
template<> class EcPrecomputation<EC2N> : public DL_GroupPrecomputation<EC2N::Point>
{
public:
	typedef EC2N EllipticCurve;

	// DL_GroupPrecomputation
	const AbstractGroup<Element> & GetGroup() const {return m_ec;}
	Element BERDecodeElement(BufferedTransformation &bt) const {return m_ec.BERDecodePoint(bt);}
	void DEREncodeElement(BufferedTransformation &bt, const Element &v) const {m_ec.DEREncodePoint(bt, v, false);}

	// non-inherited
	void SetCurve(const EC2N &ec) {m_ec = ec;}
	const EC2N & GetCurve() const {return m_ec;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~EcPrecomputation() {}
#endif

private:
	EC2N m_ec;
};

NAMESPACE_END

#endif
