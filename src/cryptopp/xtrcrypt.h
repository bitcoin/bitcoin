#ifndef CRYPTOPP_XTRCRYPT_H
#define CRYPTOPP_XTRCRYPT_H

/** \file
	"The XTR public key system" by Arjen K. Lenstra and Eric R. Verheul
*/

#include "cryptlib.h"
#include "xtr.h"
#include "integer.h"

NAMESPACE_BEGIN(CryptoPP)

//! XTR-DH with key validation

class XTR_DH : public SimpleKeyAgreementDomain, public CryptoParameters
{
	typedef XTR_DH ThisClass;
	
public:
	XTR_DH(const Integer &p, const Integer &q, const GFP2Element &g);
	XTR_DH(RandomNumberGenerator &rng, unsigned int pbits, unsigned int qbits);
	XTR_DH(BufferedTransformation &domainParams);

	void DEREncode(BufferedTransformation &domainParams) const;

	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);
	CryptoParameters & AccessCryptoParameters() {return *this;}
	unsigned int AgreedValueLength() const {return 2*m_p.ByteCount();}
	unsigned int PrivateKeyLength() const {return m_q.ByteCount();}
	unsigned int PublicKeyLength() const {return 2*m_p.ByteCount();}

	void GeneratePrivateKey(RandomNumberGenerator &rng, byte *privateKey) const;
	void GeneratePublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const;
	bool Agree(byte *agreedValue, const byte *privateKey, const byte *otherPublicKey, bool validateOtherPublicKey=true) const;

	const Integer &GetModulus() const {return m_p;}
	const Integer &GetSubgroupOrder() const {return m_q;}
	const GFP2Element &GetSubgroupGenerator() const {return m_g;}

	void SetModulus(const Integer &p) {m_p = p;}
	void SetSubgroupOrder(const Integer &q) {m_q = q;}
	void SetSubgroupGenerator(const GFP2Element &g) {m_g = g;}

private:
	unsigned int ExponentBitLength() const;

	Integer m_p, m_q;
	GFP2Element m_g;
};

NAMESPACE_END

#endif
