#ifndef CRYPTOPP_LUC_H
#define CRYPTOPP_LUC_H

/** \file
*/

#include "cryptlib.h"
#include "gfpcrypt.h"
#include "integer.h"
#include "algebra.h"
#include "secblock.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4127 4189)
#endif

#include "pkcspad.h"
#include "integer.h"
#include "oaep.h"
#include "dh.h"

#include <limits.h>

NAMESPACE_BEGIN(CryptoPP)

//! The LUC function.
/*! This class is here for historical and pedagogical interest. It has no
	practical advantages over other trapdoor functions and probably shouldn't
	be used in production software. The discrete log based LUC schemes
	defined later in this .h file may be of more practical interest.
*/
class LUCFunction : public TrapdoorFunction, public PublicKey
{
	typedef LUCFunction ThisClass;

public:
	void Initialize(const Integer &n, const Integer &e)
		{m_n = n; m_e = e;}

	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	Integer ApplyFunction(const Integer &x) const;
	Integer PreimageBound() const {return m_n;}
	Integer ImageBound() const {return m_n;}

	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// non-derived interface
	const Integer & GetModulus() const {return m_n;}
	const Integer & GetPublicExponent() const {return m_e;}

	void SetModulus(const Integer &n) {m_n = n;}
	void SetPublicExponent(const Integer &e) {m_e = e;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~LUCFunction() {}
#endif

protected:
	Integer m_n, m_e;
};

//! _
class InvertibleLUCFunction : public LUCFunction, public TrapdoorFunctionInverse, public PrivateKey
{
	typedef InvertibleLUCFunction ThisClass;

public:
	void Initialize(RandomNumberGenerator &rng, unsigned int modulusBits, const Integer &eStart=17);
	void Initialize(const Integer &n, const Integer &e, const Integer &p, const Integer &q, const Integer &u)
		{m_n = n; m_e = e; m_p = p; m_q = q; m_u = u;}

	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	Integer CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const;

	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);
	/*! parameters: (ModulusSize, PublicExponent (default 17)) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);

	// non-derived interface
	const Integer& GetPrime1() const {return m_p;}
	const Integer& GetPrime2() const {return m_q;}
	const Integer& GetMultiplicativeInverseOfPrime2ModPrime1() const {return m_u;}

	void SetPrime1(const Integer &p) {m_p = p;}
	void SetPrime2(const Integer &q) {m_q = q;}
	void SetMultiplicativeInverseOfPrime2ModPrime1(const Integer &u) {m_u = u;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~InvertibleLUCFunction() {}
#endif

protected:
	Integer m_p, m_q, m_u;
};

struct LUC
{
	static std::string StaticAlgorithmName() {return "LUC";}
	typedef LUCFunction PublicKey;
	typedef InvertibleLUCFunction PrivateKey;
};

//! LUC cryptosystem
template <class STANDARD>
struct LUCES : public TF_ES<STANDARD, LUC>
{
};

//! LUC signature scheme with appendix
template <class STANDARD, class H>
struct LUCSS : public TF_SS<STANDARD, H, LUC>
{
};

// analagous to the RSA schemes defined in PKCS #1 v2.0
typedef LUCES<OAEP<SHA> >::Decryptor LUCES_OAEP_SHA_Decryptor;
typedef LUCES<OAEP<SHA> >::Encryptor LUCES_OAEP_SHA_Encryptor;

typedef LUCSS<PKCS1v15, SHA>::Signer LUCSSA_PKCS1v15_SHA_Signer;
typedef LUCSS<PKCS1v15, SHA>::Verifier LUCSSA_PKCS1v15_SHA_Verifier;

// ********************************************************

// no actual precomputation
class DL_GroupPrecomputation_LUC : public DL_GroupPrecomputation<Integer>
{
public:
	const AbstractGroup<Element> & GetGroup() const {CRYPTOPP_ASSERT(false); throw 0;}
	Element BERDecodeElement(BufferedTransformation &bt) const {return Integer(bt);}
	void DEREncodeElement(BufferedTransformation &bt, const Element &v) const {v.DEREncode(bt);}

	// non-inherited
	void SetModulus(const Integer &v) {m_p = v;}
	const Integer & GetModulus() const {return m_p;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupPrecomputation_LUC() {}
#endif

private:
	Integer m_p;
};

//! _
class DL_BasePrecomputation_LUC : public DL_FixedBasePrecomputation<Integer>
{
public:
	// DL_FixedBasePrecomputation
	bool IsInitialized() const {return m_g.NotZero();}
	void SetBase(const DL_GroupPrecomputation<Element> &group, const Integer &base)
		{CRYPTOPP_UNUSED(group); m_g = base;}
	const Integer & GetBase(const DL_GroupPrecomputation<Element> &group) const
		{CRYPTOPP_UNUSED(group); return m_g;}
	void Precompute(const DL_GroupPrecomputation<Element> &group, unsigned int maxExpBits, unsigned int storage)
		{CRYPTOPP_UNUSED(group); CRYPTOPP_UNUSED(maxExpBits); CRYPTOPP_UNUSED(storage);}
	void Load(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation)
		{CRYPTOPP_UNUSED(group); CRYPTOPP_UNUSED(storedPrecomputation);}
	void Save(const DL_GroupPrecomputation<Element> &group, BufferedTransformation &storedPrecomputation) const
		{CRYPTOPP_UNUSED(group); CRYPTOPP_UNUSED(storedPrecomputation);}
	Integer Exponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent) const;
	Integer CascadeExponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent, const DL_FixedBasePrecomputation<Integer> &pc2, const Integer &exponent2) const
		{
			CRYPTOPP_UNUSED(group); CRYPTOPP_UNUSED(exponent); CRYPTOPP_UNUSED(pc2); CRYPTOPP_UNUSED(exponent2);
			// shouldn't be called
			throw NotImplemented("DL_BasePrecomputation_LUC: CascadeExponentiate not implemented");
		}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_BasePrecomputation_LUC() {}
#endif

private:
	Integer m_g;
};

//! _
class DL_GroupParameters_LUC : public DL_GroupParameters_IntegerBasedImpl<DL_GroupPrecomputation_LUC, DL_BasePrecomputation_LUC>
{
public:
	// DL_GroupParameters
	bool IsIdentity(const Integer &element) const {return element == Integer::Two();}
	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
	Element MultiplyElements(const Element &a, const Element &b) const
	{
		CRYPTOPP_UNUSED(a); CRYPTOPP_UNUSED(b);
		throw NotImplemented("LUC_GroupParameters: MultiplyElements can not be implemented");
	}
	Element CascadeExponentiate(const Element &element1, const Integer &exponent1, const Element &element2, const Integer &exponent2) const
	{
		CRYPTOPP_UNUSED(element1); CRYPTOPP_UNUSED(exponent1); CRYPTOPP_UNUSED(element2); CRYPTOPP_UNUSED(exponent2);
		throw NotImplemented("LUC_GroupParameters: MultiplyElements can not be implemented");
	}

	// NameValuePairs interface
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		return GetValueHelper<DL_GroupParameters_IntegerBased>(this, name, valueType, pValue).Assignable();
	}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupParameters_LUC() {}
#endif

private:
	int GetFieldType() const {return 2;}
};

//! _
class DL_GroupParameters_LUC_DefaultSafePrime : public DL_GroupParameters_LUC
{
public:
	typedef NoCofactorMultiplication DefaultCofactorOption;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupParameters_LUC_DefaultSafePrime() {}
#endif

protected:
	unsigned int GetDefaultSubgroupOrderSize(unsigned int modulusSize) const {return modulusSize-1;}
};

//! _
class DL_Algorithm_LUC_HMP : public DL_ElgamalLikeSignatureAlgorithm<Integer>
{
public:
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "LUC-HMP";}

	void Sign(const DL_GroupParameters<Integer> &params, const Integer &x, const Integer &k, const Integer &e, Integer &r, Integer &s) const;
	bool Verify(const DL_GroupParameters<Integer> &params, const DL_PublicKey<Integer> &publicKey, const Integer &e, const Integer &r, const Integer &s) const;

	size_t RLen(const DL_GroupParameters<Integer> &params) const
		{return params.GetGroupOrder().ByteCount();}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Algorithm_LUC_HMP() {}
#endif
};

//! _
struct DL_SignatureKeys_LUC
{
	typedef DL_GroupParameters_LUC GroupParameters;
	typedef DL_PublicKey_GFP<GroupParameters> PublicKey;
	typedef DL_PrivateKey_GFP<GroupParameters> PrivateKey;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_SignatureKeys_LUC() {}
#endif
};

//! LUC-HMP, based on "Digital signature schemes based on Lucas functions" by Patrick Horster, Markus Michels, Holger Petersen
template <class H>
struct LUC_HMP : public DL_SS<DL_SignatureKeys_LUC, DL_Algorithm_LUC_HMP, DL_SignatureMessageEncodingMethod_DSA, H>
{
};

//! _
struct DL_CryptoKeys_LUC
{
	typedef DL_GroupParameters_LUC_DefaultSafePrime GroupParameters;
	typedef DL_PublicKey_GFP<GroupParameters> PublicKey;
	typedef DL_PrivateKey_GFP<GroupParameters> PrivateKey;

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_CryptoKeys_LUC() {}
#endif
};

//! LUC-IES
template <class COFACTOR_OPTION = NoCofactorMultiplication, bool DHAES_MODE = true>
struct LUC_IES
	: public DL_ES<
		DL_CryptoKeys_LUC,
		DL_KeyAgreementAlgorithm_DH<Integer, COFACTOR_OPTION>,
		DL_KeyDerivationAlgorithm_P1363<Integer, DHAES_MODE, P1363_KDF2<SHA1> >,
		DL_EncryptionAlgorithm_Xor<HMAC<SHA1>, DHAES_MODE>,
		LUC_IES<> >
{
	static std::string StaticAlgorithmName() {return "LUC-IES";}	// non-standard name

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~LUC_IES() {}
#endif
};

// ********************************************************

//! LUC-DH
typedef DH_Domain<DL_GroupParameters_LUC_DefaultSafePrime> LUC_DH;

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif
