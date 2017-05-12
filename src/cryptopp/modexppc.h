#ifndef CRYPTOPP_MODEXPPC_H
#define CRYPTOPP_MODEXPPC_H

#include "cryptlib.h"
#include "modarith.h"
#include "integer.h"
#include "algebra.h"
#include "eprecomp.h"
#include "smartptr.h"
#include "pubkey.h"

NAMESPACE_BEGIN(CryptoPP)

CRYPTOPP_DLL_TEMPLATE_CLASS DL_FixedBasePrecomputationImpl<Integer>;

class ModExpPrecomputation : public DL_GroupPrecomputation<Integer>
{
public:
	// DL_GroupPrecomputation
	bool NeedConversions() const {return true;}
	Element ConvertIn(const Element &v) const {return m_mr->ConvertIn(v);}
	virtual Element ConvertOut(const Element &v) const {return m_mr->ConvertOut(v);}
	const AbstractGroup<Element> & GetGroup() const {return m_mr->MultiplicativeGroup();}
	Element BERDecodeElement(BufferedTransformation &bt) const {return Integer(bt);}
	void DEREncodeElement(BufferedTransformation &bt, const Element &v) const {v.DEREncode(bt);}

	// non-inherited
	void SetModulus(const Integer &v) {m_mr.reset(new MontgomeryRepresentation(v));}
	const Integer & GetModulus() const {return m_mr->GetModulus();}

private:
	value_ptr<MontgomeryRepresentation> m_mr;
};

NAMESPACE_END

#endif
