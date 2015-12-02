// ecp.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "ecp.h"
#include "asn.h"
#include "integer.h"
#include "nbtheory.h"
#include "modarith.h"
#include "filters.h"
#include "algebra.cpp"

NAMESPACE_BEGIN(CryptoPP)

ANONYMOUS_NAMESPACE_BEGIN
static inline ECP::Point ToMontgomery(const ModularArithmetic &mr, const ECP::Point &P)
{
	return P.identity ? P : ECP::Point(mr.ConvertIn(P.x), mr.ConvertIn(P.y));
}

static inline ECP::Point FromMontgomery(const ModularArithmetic &mr, const ECP::Point &P)
{
	return P.identity ? P : ECP::Point(mr.ConvertOut(P.x), mr.ConvertOut(P.y));
}
NAMESPACE_END

ECP::ECP(const ECP &ecp, bool convertToMontgomeryRepresentation)
{
	if (convertToMontgomeryRepresentation && !ecp.GetField().IsMontgomeryRepresentation())
	{
		m_fieldPtr.reset(new MontgomeryRepresentation(ecp.GetField().GetModulus()));
		m_a = GetField().ConvertIn(ecp.m_a);
		m_b = GetField().ConvertIn(ecp.m_b);
	}
	else
		operator=(ecp);
}

ECP::ECP(BufferedTransformation &bt)
	: m_fieldPtr(new Field(bt))
{
	BERSequenceDecoder seq(bt);
	GetField().BERDecodeElement(seq, m_a);
	GetField().BERDecodeElement(seq, m_b);
	// skip optional seed
	if (!seq.EndReached())
	{
		SecByteBlock seed;
		unsigned int unused;
		BERDecodeBitString(seq, seed, unused);
	}
	seq.MessageEnd();
}

void ECP::DEREncode(BufferedTransformation &bt) const
{
	GetField().DEREncode(bt);
	DERSequenceEncoder seq(bt);
	GetField().DEREncodeElement(seq, m_a);
	GetField().DEREncodeElement(seq, m_b);
	seq.MessageEnd();
}

bool ECP::DecodePoint(ECP::Point &P, const byte *encodedPoint, size_t encodedPointLen) const
{
	StringStore store(encodedPoint, encodedPointLen);
	return DecodePoint(P, store, encodedPointLen);
}

bool ECP::DecodePoint(ECP::Point &P, BufferedTransformation &bt, size_t encodedPointLen) const
{
	byte type;
	if (encodedPointLen < 1 || !bt.Get(type))
		return false;

	switch (type)
	{
	case 0:
		P.identity = true;
		return true;
	case 2:
	case 3:
	{
		if (encodedPointLen != EncodedPointSize(true))
			return false;

		Integer p = FieldSize();

		P.identity = false;
		P.x.Decode(bt, GetField().MaxElementByteLength()); 
		P.y = ((P.x*P.x+m_a)*P.x+m_b) % p;

		if (Jacobi(P.y, p) !=1)
			return false;

		P.y = ModularSquareRoot(P.y, p);

		if ((type & 1) != P.y.GetBit(0))
			P.y = p-P.y;

		return true;
	}
	case 4:
	{
		if (encodedPointLen != EncodedPointSize(false))
			return false;

		unsigned int len = GetField().MaxElementByteLength();
		P.identity = false;
		P.x.Decode(bt, len);
		P.y.Decode(bt, len);
		return true;
	}
	default:
		return false;
	}
}

void ECP::EncodePoint(BufferedTransformation &bt, const Point &P, bool compressed) const
{
	if (P.identity)
		NullStore().TransferTo(bt, EncodedPointSize(compressed));
	else if (compressed)
	{
		bt.Put(2 + P.y.GetBit(0));
		P.x.Encode(bt, GetField().MaxElementByteLength());
	}
	else
	{
		unsigned int len = GetField().MaxElementByteLength();
		bt.Put(4);	// uncompressed
		P.x.Encode(bt, len);
		P.y.Encode(bt, len);
	}
}

void ECP::EncodePoint(byte *encodedPoint, const Point &P, bool compressed) const
{
	ArraySink sink(encodedPoint, EncodedPointSize(compressed));
	EncodePoint(sink, P, compressed);
	assert(sink.TotalPutLength() == EncodedPointSize(compressed));
}

ECP::Point ECP::BERDecodePoint(BufferedTransformation &bt) const
{
	SecByteBlock str;
	BERDecodeOctetString(bt, str);
	Point P;
	if (!DecodePoint(P, str, str.size()))
		BERDecodeError();
	return P;
}

void ECP::DEREncodePoint(BufferedTransformation &bt, const Point &P, bool compressed) const
{
	SecByteBlock str(EncodedPointSize(compressed));
	EncodePoint(str, P, compressed);
	DEREncodeOctetString(bt, str);
}

bool ECP::ValidateParameters(RandomNumberGenerator &rng, unsigned int level) const
{
	Integer p = FieldSize();

	bool pass = p.IsOdd();
	pass = pass && !m_a.IsNegative() && m_a<p && !m_b.IsNegative() && m_b<p;

	if (level >= 1)
		pass = pass && ((4*m_a*m_a*m_a+27*m_b*m_b)%p).IsPositive();

	if (level >= 2)
		pass = pass && VerifyPrime(rng, p);

	return pass;
}

bool ECP::VerifyPoint(const Point &P) const
{
	const FieldElement &x = P.x, &y = P.y;
	Integer p = FieldSize();
	return P.identity ||
		(!x.IsNegative() && x<p && !y.IsNegative() && y<p
		&& !(((x*x+m_a)*x+m_b-y*y)%p));
}

bool ECP::Equal(const Point &P, const Point &Q) const
{
	if (P.identity && Q.identity)
		return true;

	if (P.identity && !Q.identity)
		return false;

	if (!P.identity && Q.identity)
		return false;

	return (GetField().Equal(P.x,Q.x) && GetField().Equal(P.y,Q.y));
}

const ECP::Point& ECP::Identity() const
{
	return Singleton<Point>().Ref();
}

const ECP::Point& ECP::Inverse(const Point &P) const
{
	if (P.identity)
		return P;
	else
	{
		m_R.identity = false;
		m_R.x = P.x;
		m_R.y = GetField().Inverse(P.y);
		return m_R;
	}
}

const ECP::Point& ECP::Add(const Point &P, const Point &Q) const
{
	if (P.identity) return Q;
	if (Q.identity) return P;
	if (GetField().Equal(P.x, Q.x))
		return GetField().Equal(P.y, Q.y) ? Double(P) : Identity();

	FieldElement t = GetField().Subtract(Q.y, P.y);
	t = GetField().Divide(t, GetField().Subtract(Q.x, P.x));
	FieldElement x = GetField().Subtract(GetField().Subtract(GetField().Square(t), P.x), Q.x);
	m_R.y = GetField().Subtract(GetField().Multiply(t, GetField().Subtract(P.x, x)), P.y);

	m_R.x.swap(x);
	m_R.identity = false;
	return m_R;
}

const ECP::Point& ECP::Double(const Point &P) const
{
	if (P.identity || P.y==GetField().Identity()) return Identity();

	FieldElement t = GetField().Square(P.x);
	t = GetField().Add(GetField().Add(GetField().Double(t), t), m_a);
	t = GetField().Divide(t, GetField().Double(P.y));
	FieldElement x = GetField().Subtract(GetField().Subtract(GetField().Square(t), P.x), P.x);
	m_R.y = GetField().Subtract(GetField().Multiply(t, GetField().Subtract(P.x, x)), P.y);

	m_R.x.swap(x);
	m_R.identity = false;
	return m_R;
}

template <class T, class Iterator> void ParallelInvert(const AbstractRing<T> &ring, Iterator begin, Iterator end)
{
	size_t n = end-begin;
	if (n == 1)
		*begin = ring.MultiplicativeInverse(*begin);
	else if (n > 1)
	{
		std::vector<T> vec((n+1)/2);
		unsigned int i;
		Iterator it;

		for (i=0, it=begin; i<n/2; i++, it+=2)
			vec[i] = ring.Multiply(*it, *(it+1));
		if (n%2 == 1)
			vec[n/2] = *it;

		ParallelInvert(ring, vec.begin(), vec.end());

		for (i=0, it=begin; i<n/2; i++, it+=2)
		{
			if (!vec[i])
			{
				*it = ring.MultiplicativeInverse(*it);
				*(it+1) = ring.MultiplicativeInverse(*(it+1));
			}
			else
			{
				std::swap(*it, *(it+1));
				*it = ring.Multiply(*it, vec[i]);
				*(it+1) = ring.Multiply(*(it+1), vec[i]);
			}
		}
		if (n%2 == 1)
			*it = vec[n/2];
	}
}

struct ProjectivePoint
{
	ProjectivePoint() {}
	ProjectivePoint(const Integer &x, const Integer &y, const Integer &z)
		: x(x), y(y), z(z)	{}

	Integer x,y,z;
};

class ProjectiveDoubling
{
public:
	ProjectiveDoubling(const ModularArithmetic &mr, const Integer &m_a, const Integer &m_b, const ECPPoint &Q)
		: mr(mr), firstDoubling(true), negated(false)
	{
		CRYPTOPP_UNUSED(m_b);
		if (Q.identity)
		{
			sixteenY4 = P.x = P.y = mr.MultiplicativeIdentity();
			aZ4 = P.z = mr.Identity();
		}
		else
		{
			P.x = Q.x;
			P.y = Q.y;
			sixteenY4 = P.z = mr.MultiplicativeIdentity();
			aZ4 = m_a;
		}
	}

	void Double()
	{
		twoY = mr.Double(P.y);
		P.z = mr.Multiply(P.z, twoY);
		fourY2 = mr.Square(twoY);
		S = mr.Multiply(fourY2, P.x);
		aZ4 = mr.Multiply(aZ4, sixteenY4);
		M = mr.Square(P.x);
		M = mr.Add(mr.Add(mr.Double(M), M), aZ4);
		P.x = mr.Square(M);
		mr.Reduce(P.x, S);
		mr.Reduce(P.x, S);
		mr.Reduce(S, P.x);
		P.y = mr.Multiply(M, S);
		sixteenY4 = mr.Square(fourY2);
		mr.Reduce(P.y, mr.Half(sixteenY4));
	}

	const ModularArithmetic &mr;
	ProjectivePoint P;
	bool firstDoubling, negated;
	Integer sixteenY4, aZ4, twoY, fourY2, S, M;
};

struct ZIterator
{
	ZIterator() {}
	ZIterator(std::vector<ProjectivePoint>::iterator it) : it(it) {}
	Integer& operator*() {return it->z;}
	int operator-(ZIterator it2) {return int(it-it2.it);}
	ZIterator operator+(int i) {return ZIterator(it+i);}
	ZIterator& operator+=(int i) {it+=i; return *this;}
	std::vector<ProjectivePoint>::iterator it;
};

ECP::Point ECP::ScalarMultiply(const Point &P, const Integer &k) const
{
	Element result;
	if (k.BitCount() <= 5)
		AbstractGroup<ECPPoint>::SimultaneousMultiply(&result, P, &k, 1);
	else
		ECP::SimultaneousMultiply(&result, P, &k, 1);
	return result;
}

void ECP::SimultaneousMultiply(ECP::Point *results, const ECP::Point &P, const Integer *expBegin, unsigned int expCount) const
{
	if (!GetField().IsMontgomeryRepresentation())
	{
		ECP ecpmr(*this, true);
		const ModularArithmetic &mr = ecpmr.GetField();
		ecpmr.SimultaneousMultiply(results, ToMontgomery(mr, P), expBegin, expCount);
		for (unsigned int i=0; i<expCount; i++)
			results[i] = FromMontgomery(mr, results[i]);
		return;
	}

	ProjectiveDoubling rd(GetField(), m_a, m_b, P);
	std::vector<ProjectivePoint> bases;
	std::vector<WindowSlider> exponents;
	exponents.reserve(expCount);
	std::vector<std::vector<word32> > baseIndices(expCount);
	std::vector<std::vector<bool> > negateBase(expCount);
	std::vector<std::vector<word32> > exponentWindows(expCount);
	unsigned int i;

	for (i=0; i<expCount; i++)
	{
		assert(expBegin->NotNegative());
		exponents.push_back(WindowSlider(*expBegin++, InversionIsFast(), 5));
		exponents[i].FindNextWindow();
	}

	unsigned int expBitPosition = 0;
	bool notDone = true;

	while (notDone)
	{
		notDone = false;
		bool baseAdded = false;
		for (i=0; i<expCount; i++)
		{
			if (!exponents[i].finished && expBitPosition == exponents[i].windowBegin)
			{
				if (!baseAdded)
				{
					bases.push_back(rd.P);
					baseAdded =true;
				}

				exponentWindows[i].push_back(exponents[i].expWindow);
				baseIndices[i].push_back((word32)bases.size()-1);
				negateBase[i].push_back(exponents[i].negateNext);

				exponents[i].FindNextWindow();
			}
			notDone = notDone || !exponents[i].finished;
		}

		if (notDone)
		{
			rd.Double();
			expBitPosition++;
		}
	}

	// convert from projective to affine coordinates
	ParallelInvert(GetField(), ZIterator(bases.begin()), ZIterator(bases.end()));
	for (i=0; i<bases.size(); i++)
	{
		if (bases[i].z.NotZero())
		{
			bases[i].y = GetField().Multiply(bases[i].y, bases[i].z);
			bases[i].z = GetField().Square(bases[i].z);
			bases[i].x = GetField().Multiply(bases[i].x, bases[i].z);
			bases[i].y = GetField().Multiply(bases[i].y, bases[i].z);
		}
	}

	std::vector<BaseAndExponent<Point, Integer> > finalCascade;
	for (i=0; i<expCount; i++)
	{
		finalCascade.resize(baseIndices[i].size());
		for (unsigned int j=0; j<baseIndices[i].size(); j++)
		{
			ProjectivePoint &base = bases[baseIndices[i][j]];
			if (base.z.IsZero())
				finalCascade[j].base.identity = true;
			else
			{
				finalCascade[j].base.identity = false;
				finalCascade[j].base.x = base.x;
				if (negateBase[i][j])
					finalCascade[j].base.y = GetField().Inverse(base.y);
				else
					finalCascade[j].base.y = base.y;
			}
			finalCascade[j].exponent = Integer(Integer::POSITIVE, 0, exponentWindows[i][j]);
		}
		results[i] = GeneralCascadeMultiplication(*this, finalCascade.begin(), finalCascade.end());
	}
}

ECP::Point ECP::CascadeScalarMultiply(const Point &P, const Integer &k1, const Point &Q, const Integer &k2) const
{
	if (!GetField().IsMontgomeryRepresentation())
	{
		ECP ecpmr(*this, true);
		const ModularArithmetic &mr = ecpmr.GetField();
		return FromMontgomery(mr, ecpmr.CascadeScalarMultiply(ToMontgomery(mr, P), k1, ToMontgomery(mr, Q), k2));
	}
	else
		return AbstractGroup<Point>::CascadeScalarMultiply(P, k1, Q, k2);
}

NAMESPACE_END

#endif
