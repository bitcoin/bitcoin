#ifndef CERT_H
#define CERT_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CAliasIndex;
bool CheckCertInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeCertTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseCertTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeCertScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsCertOp(int op);
int IndexOfCertOutput(const CTransaction& tx);
bool EncryptMessage(const std::vector<unsigned char> &strPublicKey, const std::string &strMessage, std::string &strCipherText);
bool DecryptPrivateKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCipherText, std::string &strMessage);
bool DecryptPrivateKey(const CAliasIndex& alias, std::string &strKey);
bool DecryptMessage(const CAliasIndex& alias, const std::vector<unsigned char> &vchCipherText, std::string &strMessage);
void CertTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string certFromOp(int op);
bool RemoveCertScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
class CCert {
public:
	std::vector<unsigned char> vchCert;
	std::vector<unsigned char> vchAlias;
	// to modify vchAlias in certtransfer
	std::vector<unsigned char> vchLinkAlias;
	std::vector<unsigned char> vchTitle;
    std::vector<unsigned char> vchData;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
	// cert data encrypted to this public key linked to alias owner's key through encryption
	std::vector<unsigned char> vchEncryptionPublicKey;
	// secret key encrypted to vchAlias(owner) private key
	std::vector<unsigned char> vchEncryptionPrivateKey;
    uint256 txHash;
    uint64_t nHeight;
	unsigned char safetyLevel;
	bool safeSearch;
	// 1 can edit, 2 can edit/transfer
	unsigned char nAccessFlags;
    CCert() {
        SetNull();
    }
    CCert(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	void ClearCert()
	{
		vchData.clear();
		vchPubData.clear();
		vchTitle.clear();
		sCategory.clear();
		vchEncryptionPublicKey.clear();
		vchEncryptionPrivateKey.clear();

	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {		
		READWRITE(vchData);
		READWRITE(vchTitle);
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchLinkAlias);
		READWRITE(nAccessFlags);
		READWRITE(vchCert);
		READWRITE(VARINT(safetyLevel));
		READWRITE(safeSearch);
		READWRITE(sCategory);
		READWRITE(vchAlias);
		READWRITE(vchEncryptionPublicKey);
		READWRITE(vchEncryptionPrivateKey);
	}
    friend bool operator==(const CCert &a, const CCert &b) {
        return (
        a.vchData == b.vchData
		&& a.vchTitle == b.vchTitle
		&& a.vchPubData == b.vchPubData
        && a.txHash == b.txHash
        && a.nHeight == b.nHeight
		&& a.vchAlias == b.vchAlias
		&& a.vchLinkAlias == b.vchLinkAlias
		&& a.nAccessFlags == b.nAccessFlags
		&& a.safetyLevel == b.safetyLevel
		&& a.safeSearch == b.safeSearch
		&& a.vchCert == b.vchCert
		&& a.sCategory == b.sCategory
		&& a.vchEncryptionPublicKey == b.vchEncryptionPublicKey
		&& a.vchEncryptionPrivateKey == b.vchEncryptionPrivateKey
        );
    }

    CCert operator=(const CCert &b) {
		vchTitle = b.vchTitle;
        vchData = b.vchData;
		vchPubData = b.vchPubData;
        txHash = b.txHash;
        nHeight = b.nHeight;
		vchAlias = b.vchAlias;
		vchLinkAlias = b.vchLinkAlias;
		nAccessFlags = b.nAccessFlags;
		safetyLevel = b.safetyLevel;
		safeSearch = b.safeSearch;
		vchCert = b.vchCert;
		sCategory = b.sCategory;
		vchEncryptionPublicKey = b.vchEncryptionPublicKey;
		vchEncryptionPrivateKey = b.vchEncryptionPrivateKey;
        return *this;
    }

    friend bool operator!=(const CCert &a, const CCert &b) {
        return !(a == b);
    }
    bool GetCertFromList(std::vector<CCert> &certList) {
        if(certList.size() == 0) return false;
		CCert myCert = certList.front();
		if(nHeight <= 0)
		{
			*this = myCert;
			return true;
		}
			
		// find the closest cert without going over in height, assuming certList orders entries by nHeight ascending
        for(std::vector<CCert>::reverse_iterator it = certList.rbegin(); it != certList.rend(); ++it) {
            const CCert &c = *it;
			// skip if this height is greater than our cert height
			if(c.nHeight > nHeight)
				continue;
            myCert = c;
			break;
        }
        *this = myCert;
        return true;
    }
    void SetNull() { sCategory.clear(); vchTitle.clear(); safeSearch = false; vchEncryptionPublicKey.clear(); vchEncryptionPrivateKey.clear(); nAccessFlags = 2; vchLinkAlias.clear(); vchCert.clear(); safetyLevel = 0; nHeight = 0; txHash.SetNull(); vchAlias.clear(); vchData.clear(); vchPubData.clear();}
    bool IsNull() const { return (sCategory.empty() && vchTitle.empty() && !safeSearch && vchEncryptionPublicKey.empty() && vchEncryptionPrivateKey.empty() && nAccessFlags == 2 && vchLinkAlias.empty() && vchCert.empty() && safetyLevel == 0 && txHash.IsNull() &&  nHeight == 0 && vchData.empty() && vchPubData.empty() && vchAlias.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CCertDB : public CDBWrapper {
public:
    CCertDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "certificates", nCacheSize, fMemory, fWipe) {}

    bool WriteCert(const std::vector<unsigned char>& name, const std::vector<CCert>& vtxPos) {
        return Write(make_pair(std::string("certi"), name), vtxPos);
    }

    bool EraseCert(const std::vector<unsigned char>& name) {
        return Erase(make_pair(std::string("certi"), name));
    }

    bool ReadCert(const std::vector<unsigned char>& name, std::vector<CCert>& vtxPos) {
        return Read(make_pair(std::string("certi"), name), vtxPos);
    }

    bool ExistsCert(const std::vector<unsigned char>& name) {
        return Exists(make_pair(std::string("certi"), name));
    }
	bool ScanCerts(
		const std::vector<unsigned char>& vchCert, const std::string &strRegExp, const std::vector<std::string>& aliasArray, bool safeSearch, const std::string& strCategory,
            unsigned int nMax,
            std::vector<CCert>& certScan);
	bool GetDBCerts(std::vector<CCert>& certs, const uint64_t& nExpireFilter, const std::vector<std::string>& aliasArray);
	bool CleanupDatabase(int &servicesCleaned);

};
bool GetTxOfCert(const std::vector<unsigned char> &vchCert,
        CCert& txPos, CTransaction& tx, bool skipExpiresCheck=false);
bool GetTxAndVtxOfCert(const std::vector<unsigned char> &vchCert,
					   CCert& txPos, CTransaction& tx, std::vector<CCert> &vtxPos, bool skipExpiresCheck=false);
bool GetVtxOfCert(const std::vector<unsigned char> &vchCert,
					   CCert& txPos, std::vector<CCert> &vtxPos, bool skipExpiresCheck=false);
void PutToCertList(std::vector<CCert> &certList, CCert& index);
bool BuildCertJson(const CCert& cert, const CAliasIndex& alias, UniValue& oName, const std::string &strWalletless="");
bool BuildCertStatsJson(const std::vector<CCert> &certs, UniValue& oCertStats);
uint64_t GetCertExpiration(const CCert& cert);
#endif // CERT_H
