// eprecomp.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "eprecomp.h"
#include "integer.h"
#include "algebra.h"
#include "asn.h"

NAMESPACE_BEGIN(CryptoPP)

template <class T> void DL_FixedBasePrecomputationImpl<T>::SetBase(const DL_GroupPrecomputation<Element> &group, const Element &i_base)
{
	m_base = group.NeedConversions() ? group.ConvertIn(i_base) : i_base;

	if (m_bases.empty() || !(m_base == m_bases[0]))
	{
		m_bases.resize(1);
		m_bases[0] = m_base;
	}

	if (group.NeedConversions())
		m_base = i_base;
}

template <class T> void DL_FixedBasePrecomputationImpl<T>::Precompute(const DL_GroupPrecomputation<Element> &group, unsigned int maxExpBits, unsigned int storage)
{
	CRYPTOPP_ASSERT(m_bases.size() > 0);
	CRYPTOPP_ASSERT(storage <= maxExpBits);

	if (storage > 1)
	{
		m_windowSize = (maxExpBits+storage-1)/storage;
		m_exponentBase = Integer::Power2(m_windowSize);
	}

	m_bases.resize(storage);
	for (unsigned i=1; i<storage; i++)
		m_bases[i] = group.GetGroup().ScalarMultiply(m_bases[i-1], m_exponentBase);
}

template <class T> void DL_FixedBasePrecomputationImpl<T>::Load(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	word32 version;
	BERDecodeUnsigned<word32>(seq, version, INTEGER, 1, 1);
	m_exponentBase.BERDecode(seq);
	m_windowSize = m_exponentBase.BitCount() - 1;
	m_bases.clear();
	while (!seq.EndReached())
		m_bases.push_back(group.BERDecodeElement(seq));
	if (!m_bases.empty() && group.NeedConversions())
		m_base = group.ConvertOut(m_bases[0]);
	seq.MessageEnd();
}

template <class T> void DL_FixedBasePrecomputationImpl<T>::Save(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	DEREncodeUnsigned<word32>(seq, 1);	// version
	m_exponentBase.DEREncode(seq);
	for (unsigned i=0; i<m_bases.size(); i++)
		group.DEREncodeElement(seq, m_bases[i]);
	seq.MessageEnd();
}

template <class T> void DL_FixedBasePrecomputationImpl<T>::PrepareCascade(const DL_GroupPrecomputation<Element> &i_group, std::vector<BaseAndExponent<Element> > &eb, const Integer &exponent) const
{
	const AbstractGroup<T> &group = i_group.GetGroup();

	Integer r, q, e = exponent;
	bool fastNegate = group.InversionIsFast() && m_windowSize > 1;
	unsigned int i;

	for (i=0; i+1<m_bases.size(); i++)
	{
		Integer::DivideByPowerOf2(r, q, e, m_windowSize);
		std::swap(q, e);
		if (fastNegate && r.GetBit(m_windowSize-1))
		{
			++e;
			eb.push_back(BaseAndExponent<Element>(group.Inverse(m_bases[i]), m_exponentBase - r));
		}
		else
			eb.push_back(BaseAndExponent<Element>(m_bases[i], r));
	}
	eb.push_back(BaseAndExponent<Element>(m_bases[i], e));
}

template <class T> T DL_FixedBasePrecomputationImpl<T>::Exponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent) const
{
	std::vector<BaseAndExponent<Element> > eb;	// array of segments of the exponent and precalculated bases
	eb.reserve(m_bases.size());
	PrepareCascade(group, eb, exponent);
	return group.ConvertOut(GeneralCascadeMultiplication<Element>(group.GetGroup(), eb.begin(), eb.end()));
}

template <class T> T
	DL_FixedBasePrecomputationImpl<T>::CascadeExponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent,
		const DL_FixedBasePrecomputation<T> &i_pc2, const Integer &exponent2) const
{
	std::vector<BaseAndExponent<Element> > eb;	// array of segments of the exponent and precalculated bases
	const DL_FixedBasePrecomputationImpl<T> &pc2 = static_cast<const DL_FixedBasePrecomputationImpl<T> &>(i_pc2);
	eb.reserve(m_bases.size() + pc2.m_bases.size());
	PrepareCascade(group, eb, exponent);
	pc2.PrepareCascade(group, eb, exponent2);
	return group.ConvertOut(GeneralCascadeMultiplication<Element>(group.GetGroup(), eb.begin(), eb.end()));
}

NAMESPACE_END

#endif
