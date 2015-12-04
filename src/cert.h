#ifndef CERT_H
#define CERT_H

#include "rpcserver.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CValidationState;
class CCoinsViewCache;
class CCoins;
bool CheckCertInputs(const CTransaction &tx, CValidationState &state, const CCoinsViewCache &inputs, bool fBlock, bool fMiner, bool fJustCheck, int nHeight);
bool IsCertMine(const CTransaction& tx);
bool DecodeCertTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, int nHeight);
bool DecodeCertTxInputs(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, CCoinsViewCache &inputs);
bool DecodeCertScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsCertOp(int op);
int IndexOfCertOutput(const CTransaction& tx);
bool GetValueOfCertTxHash(const uint256 &txHash, std::vector<unsigned char>& vchValue, uint256& hash, int& nHeight);
int64_t GetTxHashHeight(const uint256 txHash);
int64_t GetCertNetworkFee(opcodetype seed, unsigned int nHeight);
int64_t GetCertNetFee(const CTransaction& tx);
bool InsertCertFee(CBlockIndex *pindex, uint256 hash, uint64_t nValue);

bool EncryptMessage(const std::vector<unsigned char> &vchPublicKey, const std::vector<unsigned char> &vchMessage, std::string &strCipherText);
bool DecryptMessage(const std::vector<unsigned char> &vchPublicKey, const std::vector<unsigned char> &vchCipherText, std::string &strMessage);
std::string certFromOp(int op);
bool GetCertAddress(const CTransaction& tx, std::string& strAddress);
int GetCertExpirationDepth();
CScript RemoveCertScriptPrefix(const CScript& scriptIn);
bool DecodeCertScript(const CScript& script, int& op,
        std::vector<std::vector<unsigned char> > &vvch,
        CScript::const_iterator& pc);

class CCert {
public:
    std::vector<unsigned char> vchRand;
	std::vector<unsigned char> vchPubKey;
    std::vector<unsigned char> vchTitle;
    std::vector<unsigned char> vchData;
    uint256 txHash;
    uint64_t nHeight;
	bool bPrivate;
	std::vector<unsigned char> vchOfferLink;
    CCert() {
        SetNull();
    }
    CCert(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchRand);
        READWRITE(vchTitle);
        READWRITE(vchData);
        READWRITE(txHash);
        READWRITE(VARINT(nHeight));
		READWRITE(vchPubKey);
		READWRITE(bPrivate);
		READWRITE(vchOfferLink);
		
		
	}

    friend bool operator==(const CCert &a, const CCert &b) {
        return (
        a.vchRand == b.vchRand
        && a.vchTitle == b.vchTitle
        && a.vchData == b.vchData
        && a.txHash == b.txHash
        && a.nHeight == b.nHeight
		&& a.vchPubKey == b.vchPubKey
		&& a.bPrivate == b.bPrivate
		&& a.vchOfferLink == b.vchOfferLink
        );
    }

    CCert operator=(const CCert &b) {
        vchRand = b.vchRand;
        vchTitle = b.vchTitle;
        vchData = b.vchData;
        txHash = b.txHash;
        nHeight = b.nHeight;
		vchPubKey = b.vchPubKey;
		bPrivate = b.bPrivate;
		vchOfferLink = b.vchOfferLink;
        return *this;
    }

    friend bool operator!=(const CCert &a, const CCert &b) {
        return !(a == b);
    }

    void SetNull() { nHeight = 0; txHash.SetNull();  vchRand.clear(); vchPubKey.clear(); bPrivate = false;vchOfferLink.clear();}
    bool IsNull() const { return (txHash.IsNull() &&  nHeight == 0 && vchRand.size() == 0); }
    bool UnserializeFromTx(const CTransaction &tx);
    std::string SerializeToString();
};


class CCertDB : public CDBWrapper {
public:
    CCertDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "certificates", nCacheSize, fMemory, fWipe) {}

    bool WriteCert(const std::vector<unsigned char>& name, std::vector<CCert>& vtxPos) {
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
            const std::vector<unsigned char>& vchName,
            unsigned int nMax,
            std::vector<std::pair<std::vector<unsigned char>, CCert> >& certScan);

    bool ReconstructCertIndex(CBlockIndex *pindexRescan);
};
bool GetTxOfCert(CCertDB& dbCert, const std::vector<unsigned char> &vchCert,
        CCert& txPos, CTransaction& tx);

#endif // CERT_H
