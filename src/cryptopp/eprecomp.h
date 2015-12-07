// eprecomp.h - written and placed in the public domain by Wei Dai

//! \file eprecomp.h
//! \brief Classes for precomputation in a group

#ifndef CRYPTOPP_EPRECOMP_H
#define CRYPTOPP_EPRECOMP_H

#include "cryptlib.h"
#include "integer.h"
#include "algebra.h"
#include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

template <class T>
class DL_GroupPrecomputation
{
public:
	typedef T Element;

	virtual bool NeedConversions() const {return false;}
	virtual Element ConvertIn(const Element &v) const {return v;}
	virtual Element ConvertOut(const Element &v) const {return v;}
	virtual const AbstractGroup<Element> & GetGroup() const =0;
	virtual Element BERDecodeElement(BufferedTransformation &bt) const =0;
	virtual void DEREncodeElement(BufferedTransformation &bt, const Element &P) const =0;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupPrecomputation() {}
#endif
};

template <class T>
class DL_FixedBasePrecomputation
{
public:
	typedef T Element;

	virtual bool IsInitialized() const =0;
	virtual void SetBase(const DL_GroupPrecomputation<Element> &group, const Element &base) =0;
	virtual const Element & GetBase(const DL_GroupPrecomputation<Element> &group) const =0;
	virtual void Precompute(const DL_GroupPrecomputation<Element> &group, unsigned int maxExpBits, unsigned int storage) =0;
	virtual void Load(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation) =0;
	virtual void Save(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation) const =0;
	virtual Element Exponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent) const =0;
	virtual Element CascadeExponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent, const DL_FixedBasePrecomputation<Element> &pc2, const Integer &exponent2) const =0;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_FixedBasePrecomputation() {}
#endif
};

template <class T>
class DL_FixedBasePrecomputationImpl : public DL_FixedBasePrecomputation<T>
{
public:
	typedef T Element;

	DL_FixedBasePrecomputationImpl() : m_windowSize(0) {}

	// DL_FixedBasePrecomputation
	bool IsInitialized() const
		{return !m_bases.empty();}
	void SetBase(const DL_GroupPrecomputation<Element> &group, const Element &base);
	const Element & GetBase(const DL_GroupPrecomputation<Element> &group) const
		{return group.NeedConversions() ? m_base : m_bases[0];}
	void Precompute(const DL_GroupPrecomputation<Element> &group, unsigned int maxExpBits, unsigned int storage);
	void Load(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation);
	void Save(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation) const;
	Element Exponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent) const;
	Element CascadeExponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent, const DL_FixedBasePrecomputation<Element> &pc2, const Integer &exponent2) const;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_FixedBasePrecomputationImpl() {}
#endif

private:
	void PrepareCascade(const DL_GroupPrecomputation<Element> &group, std::vector<BaseAndExponent<Element> > &eb, const Integer &exponent) const;

	Element m_base;
	unsigned int m_windowSize;
	Integer m_exponentBase;			// what base to represent the exponent in
	std::vector<Element> m_bases;	// precalculated bases
};

NAMESPACE_END

#ifdef CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#include "eprecomp.cpp"
#endif

#endif
