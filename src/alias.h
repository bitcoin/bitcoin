#ifndef ALIAS_H
#define ALIAS_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
#include "consensus/params.h"
#include "sync.h" 
class CWalletTx;
class CTransaction;
class CTxOut;
class COutPoint;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CSyscoinAddress;
class COutPoint;
struct CRecipient;
static const unsigned int MAX_GUID_LENGTH = 71;
static const unsigned int MAX_NAME_LENGTH = 256;
static const unsigned int MAX_VALUE_LENGTH = 1024;
static const unsigned int MAX_ID_LENGTH = 20;
static const unsigned int MAX_ENCRYPTED_VALUE_LENGTH = MAX_VALUE_LENGTH + 85;
static const unsigned int MAX_ENCRYPTED_NAME_LENGTH = MAX_NAME_LENGTH + 85;
static const unsigned int MAX_ALIAS_UPDATES_PER_BLOCK = 5;

static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;
static const unsigned int SAFETY_LEVEL1 = 1;
static const unsigned int SAFETY_LEVEL2 = 2;

class CAliasUnprunable
{
	public:
	std::vector<unsigned char> vchGUID;
    uint64_t nExpireTime;
	CAliasUnprunable() {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchGUID);
		READWRITE(VARINT(nExpireTime));
	}

    inline friend bool operator==(const CAliasUnprunable &a, const CAliasUnprunable &b) {
        return (
		a.vchGUID == b.vchGUID
		&& a.nExpireTime == b.nExpireTime
        );
    }

    inline CAliasUnprunable operator=(const CAliasUnprunable &b) {
		vchGUID = b.vchGUID;
		nExpireTime = b.nExpireTime;
        return *this;
    }

    inline friend bool operator!=(const CAliasUnprunable &a, const CAliasUnprunable &b) {
        return !(a == b);
    }

    inline void SetNull() { vchGUID.clear(); nExpireTime = 0;}
    inline bool IsNull() const { return (vchGUID.empty() && nExpireTime == 0 ); }
};
class CAliasPayment {
public:
	
	unsigned char nOut;
	uint256 txHash;
	CAliasPayment() {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(txHash);
		READWRITE(VARINT(nOut));
	}

    inline friend bool operator==(const CAliasPayment &a, const CAliasPayment &b) {
        return (
		a.txHash == b.txHash
        && a.nOut == b.nOut
        );
    }

    inline CAliasPayment operator=(const CAliasPayment &b) {
		txHash = b.txHash;
        nOut = b.nOut;
        return *this;
    }

    inline friend bool operator!=(const CAliasPayment &a, const CAliasPayment &b) {
        return !(a == b);
    }

    inline void SetNull() { txHash.SetNull(); nOut = 0;}
    inline bool IsNull() const { return (txHash.IsNull() && nOut == 0); }

};
class CMultiSigAliasInfo {
public:
	std::vector<std::string> vchAliases;
	unsigned char nRequiredSigs;
	std::vector<unsigned char> vchRedeemScript;
	std::vector<std::string> vchEncryptionPrivateKeys;
	CMultiSigAliasInfo() {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchAliases);
		READWRITE(VARINT(nRequiredSigs));
		READWRITE(vchRedeemScript);
		READWRITE(vchEncryptionPrivateKeys);
	}

    inline friend bool operator==(const CMultiSigAliasInfo &a, const CMultiSigAliasInfo &b) {
        return (
		a.vchAliases == b.vchAliases
        && a.nRequiredSigs == b.nRequiredSigs
		&& a.vchRedeemScript == b.vchRedeemScript
		&& a.vchEncryptionPrivateKeys == b.vchEncryptionPrivateKeys
        );
    }

    inline CMultiSigAliasInfo operator=(const CMultiSigAliasInfo &b) {
		vchAliases = b.vchAliases;
        nRequiredSigs = b.nRequiredSigs;
		vchRedeemScript = b.vchRedeemScript;
		vchEncryptionPrivateKeys = b.vchEncryptionPrivateKeys;
        return *this;
    }

    inline friend bool operator!=(const CMultiSigAliasInfo &a, const CMultiSigAliasInfo &b) {
        return !(a == b);
    }

    inline void SetNull() { vchEncryptionPrivateKeys.clear(); vchRedeemScript.clear(); vchAliases.clear(); nRequiredSigs = 0;}
    inline bool IsNull() const { return (vchEncryptionPrivateKeys.empty() && vchRedeemScript.empty() && vchAliases.empty() && nRequiredSigs == 0); }

};
class CAliasIndex {
public:
	std::vector<unsigned char> vchAlias;
	std::vector<unsigned char> vchGUID;
    uint256 txHash;
    uint64_t nHeight;
	uint64_t nExpireTime;
	std::vector<unsigned char> vchAliasPeg;
    std::vector<unsigned char> vchPublicValue;
	std::vector<unsigned char> vchPrivateValue;
	std::vector<unsigned char> vchPubKey;
	std::vector<unsigned char> vchEncryptionPublicKey;
	std::vector<unsigned char> vchEncryptionPrivateKey;
	std::vector<unsigned char> vchPassword;
	CMultiSigAliasInfo multiSigInfo;
	unsigned char safetyLevel;
	unsigned int nRatingAsBuyer;
	unsigned int nRatingCountAsBuyer;
	unsigned int nRatingAsSeller;
	unsigned int nRatingCountAsSeller;
	unsigned int nRatingAsArbiter;
	unsigned int nRatingCountAsArbiter;
	bool safeSearch;
	bool acceptCertTransfers;
    CAliasIndex() { 
        SetNull();
    }
    CAliasIndex(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	void ClearAlias()
	{
		vchEncryptionPublicKey.clear();
		vchEncryptionPrivateKey.clear();
		vchPublicValue.clear();
		vchPrivateValue.clear();
		vchGUID.clear();
		multiSigInfo.SetNull();
		vchPassword.clear();
	}
    bool GetAliasFromList(std::vector<CAliasIndex> &aliasList) {
        if(aliasList.size() == 0) return false;
		CAliasIndex myAlias = aliasList.front();
		if(nHeight <= 0)
		{
			*this = myAlias;
			return true;
		}
			
		// find the closest alias without going over in height, assuming aliasList orders entries by nHeight ascending
        for(std::vector<CAliasIndex>::reverse_iterator it = aliasList.rbegin(); it != aliasList.rend(); ++it) {
            const CAliasIndex &a = *it;
			// skip if this height is greater than our alias height
			if(a.nHeight > nHeight)
				continue;
            myAlias = a;
			break;
        }
        *this = myAlias;
        return true;
    }
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {        
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchPublicValue);
		READWRITE(vchPrivateValue);
		READWRITE(vchEncryptionPublicKey);
		READWRITE(vchEncryptionPrivateKey);
		READWRITE(vchPubKey);
		READWRITE(vchPassword);
		READWRITE(vchAlias);
		READWRITE(vchAliasPeg);
		READWRITE(vchGUID);
		READWRITE(VARINT(safetyLevel));
		READWRITE(VARINT(nExpireTime));
		READWRITE(safeSearch);
		READWRITE(acceptCertTransfers);
		READWRITE(multiSigInfo);
		READWRITE(VARINT(nRatingAsBuyer));
		READWRITE(VARINT(nRatingCountAsBuyer));
		READWRITE(VARINT(nRatingAsSeller));
		READWRITE(VARINT(nRatingCountAsSeller));
		READWRITE(VARINT(nRatingAsArbiter));
		READWRITE(VARINT(nRatingCountAsArbiter));
	}
    friend bool operator==(const CAliasIndex &a, const CAliasIndex &b) {
		return (a.vchEncryptionPublicKey == b.vchEncryptionPublicKey && a.vchEncryptionPrivateKey == b.vchEncryptionPrivateKey && a.vchPassword ==b.vchPassword && a.acceptCertTransfers == b.acceptCertTransfers && a.multiSigInfo == b.multiSigInfo && a.nExpireTime == b.nExpireTime && a.vchGUID == b.vchGUID && a.vchAlias == b.vchAlias && a.nRatingCountAsArbiter == b.nRatingCountAsArbiter && a.nRatingAsArbiter == b.nRatingAsArbiter && a.nRatingCountAsSeller == b.nRatingCountAsSeller && a.nRatingAsSeller == b.nRatingAsSeller && a.nRatingCountAsBuyer == b.nRatingCountAsBuyer && a.nRatingAsBuyer == b.nRatingAsBuyer && a.safetyLevel == b.safetyLevel && a.safeSearch == b.safeSearch && a.nHeight == b.nHeight && a.txHash == b.txHash && a.vchPublicValue == b.vchPublicValue && a.vchPrivateValue == b.vchPrivateValue && a.vchPubKey == b.vchPubKey);
    }

    friend bool operator!=(const CAliasIndex &a, const CAliasIndex &b) {
        return !(a == b);
    }
    CAliasIndex operator=(const CAliasIndex &b) {
		vchGUID = b.vchGUID;
		nExpireTime = b.nExpireTime;
		vchAlias = b.vchAlias;
		vchAliasPeg = b.vchAliasPeg;
        txHash = b.txHash;
        nHeight = b.nHeight;
        vchPublicValue = b.vchPublicValue;
        vchPrivateValue = b.vchPrivateValue;
        vchPubKey = b.vchPubKey;
		vchPassword = b.vchPassword;
		safetyLevel = b.safetyLevel;
		safeSearch = b.safeSearch;
		acceptCertTransfers = b.acceptCertTransfers;
		multiSigInfo = b.multiSigInfo;
		nRatingAsBuyer = b.nRatingAsBuyer;
		nRatingCountAsBuyer = b.nRatingCountAsBuyer;
		nRatingAsSeller = b.nRatingAsSeller;
		nRatingCountAsSeller = b.nRatingCountAsSeller;
		nRatingAsArbiter = b.nRatingAsArbiter;
		nRatingCountAsArbiter = b.nRatingCountAsArbiter;
		vchEncryptionPrivateKey = b.vchEncryptionPrivateKey;
		vchEncryptionPublicKey = b.vchEncryptionPublicKey;
        return *this;
    }   
    void SetNull() {vchEncryptionPublicKey.clear();vchEncryptionPrivateKey.clear();vchAliasPeg.clear();vchPassword.clear(); acceptCertTransfers = true; multiSigInfo.SetNull(); nExpireTime = 0; vchGUID.clear(); vchAlias.clear(); nRatingCountAsBuyer = 0; nRatingAsBuyer = 0; nRatingCountAsSeller = 0; nRatingAsSeller = 0; nRatingCountAsArbiter = 0; nRatingAsArbiter = 0; safetyLevel = 0; safeSearch = true; txHash.SetNull(); nHeight = 0; vchPublicValue.clear(); vchPrivateValue.clear(); vchPubKey.clear(); }
    bool IsNull() const { return (vchEncryptionPrivateKey.empty() && vchEncryptionPrivateKey.empty() && vchAliasPeg.empty() && vchPassword.empty() && acceptCertTransfers && multiSigInfo.IsNull() && nExpireTime == 0 && vchGUID.empty() && vchAlias.empty() && nRatingCountAsBuyer == 0 && nRatingAsBuyer == 0 && nRatingCountAsArbiter == 0 && nRatingAsArbiter == 0 && nRatingCountAsSeller == 0 && nRatingAsSeller == 0 && safetyLevel == 0 && safeSearch && nHeight == 0 && txHash.IsNull() && vchPublicValue.empty() && vchPrivateValue.empty() && vchPubKey.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class CAliasDB : public CDBWrapper {
public:
    CAliasDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "aliases", nCacheSize, fMemory, fWipe) {
    }
	bool WriteAlias(const std::vector<unsigned char>& name, const std::vector<CAliasIndex>& vtxPos) {	
		return Write(make_pair(std::string("namei"), name), vtxPos);
	}
	bool WriteAlias(const std::vector<unsigned char>& name, const CAliasUnprunable &aliasUnprunable, const std::vector<unsigned char>& address, const std::vector<CAliasIndex>& vtxPos) {
		if(address.empty())
			return false;		
		return Write(make_pair(std::string("namei"), name), vtxPos) && Write(make_pair(std::string("namea"), address), name) && Write(make_pair(std::string("nameu"), name), aliasUnprunable);
	}
	bool WriteAliasPayment(const std::vector<unsigned char>& name, const std::vector<CAliasPayment>& vtxPaymentPos)
	{
		return Write(make_pair(std::string("namep"), name), vtxPaymentPos);
	}

	bool EraseAlias(const std::vector<unsigned char>& name) {
	    bool eraseAlias =  Erase(make_pair(std::string("namei"), name)) ;
		bool eraseAliasPayment = Erase(make_pair(std::string("namep"), name));
		return eraseAlias && eraseAliasPayment;
	}
	bool EraseAliasPayment(const std::vector<unsigned char>& name) {
	    return Erase(make_pair(std::string("namep"), name));
	}
	bool ReadAlias(const std::vector<unsigned char>& name, std::vector<CAliasIndex>& vtxPos) {
		return Read(make_pair(std::string("namei"), name), vtxPos);
	}
	bool ReadAddress(const std::vector<unsigned char>& address, std::vector<unsigned char>& name) {
		return Read(make_pair(std::string("namea"), address), name);
	}
	bool ReadAliasPayment(const std::vector<unsigned char>& name, std::vector<CAliasPayment>& vtxPaymentPos) {
		return Read(make_pair(std::string("namep"), name), vtxPaymentPos);
	}
	bool ReadAliasUnprunable(const std::vector<unsigned char>& name, CAliasUnprunable& aliasUnprunable) {
		return Read(make_pair(std::string("nameu"), name), aliasUnprunable);
	}
	bool ExistsAlias(const std::vector<unsigned char>& name) {
	    return Exists(make_pair(std::string("namei"), name));
	}
	bool ExistsAliasPayment(const std::vector<unsigned char>& name) {
	    return Exists(make_pair(std::string("namep"), name));
	}
	bool ExistsAliasUnprunable(const std::vector<unsigned char>& name) {
	    return Exists(make_pair(std::string("nameu"), name));
	}
	bool ExistsAddress(const std::vector<unsigned char>& address) {
	    return Exists(make_pair(std::string("namea"), address));
	}
    bool ScanNames(
		const std::vector<unsigned char>& vchAlias, const std::string& strRegExp, bool safeSearch,
            unsigned int nMax,
            std::vector<CAliasIndex>& nameScan);
	bool CleanupDatabase(int &servicesCleaned);

};

class COfferDB;
class CCertDB;
class CEscrowDB;
class CMessageDB;
extern CAliasDB *paliasdb;
extern COfferDB *pofferdb;
extern CCertDB *pcertdb;
extern CEscrowDB *pescrowdb;
extern CMessageDB *pmessagedb;



std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
int GetSyscoinTxVersion();
const int SYSCOIN_TX_VERSION = 0x7400;
bool IsValidAliasName(const std::vector<unsigned char> &vchAlias);
bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient);
void CreateFeeRecipient(CScript& scriptPubKey, const std::vector<unsigned char>& vchAliasPeg, const uint64_t& nHeight, const std::vector<unsigned char>& data, CRecipient& recipient);
CAmount GetDataFee(const CScript& scriptPubKey, const std::vector<unsigned char>& vchAliasPeg, const uint64_t& nHeight);
bool IsSyscoinTxMine(const CTransaction& tx,const std::string &type);
bool IsAliasOp(int op);
bool getCategoryList(std::vector<std::string>& categoryList);
bool getBanList(const std::vector<unsigned char> &banData, std::map<std::string, unsigned char> &banAliasList,  std::map<std::string, unsigned char>& banCertList,  std::map<std::string, unsigned char>& banOfferList);
bool GetTxOfAlias(const std::vector<unsigned char> &vchAlias, CAliasIndex& alias, CTransaction& tx, bool skipExpiresCheck=false);
bool GetTxAndVtxOfAlias(const std::vector<unsigned char> &vchAlias, CAliasIndex& alias, CTransaction& tx, std::vector<CAliasIndex> &vtxPos, bool &isExpired, bool skipExpiresCheck=false);
bool GetVtxOfAlias(const std::vector<unsigned char> &vchAlias, CAliasIndex& alias, std::vector<CAliasIndex> &vtxPos, bool &isExpired, bool skipExpiresCheck=false);

int IndexOfAliasOutput(const CTransaction& tx);
bool GetAliasOfTx(const CTransaction& tx, std::vector<unsigned char>& name);
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, bool payment=false);
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAliasScript(const CScript& script, int& op,
		std::vector<std::vector<unsigned char> > &vvch);
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, unsigned char& safetyLevel, bool& safeSearch, std::vector<unsigned char> &vchRedeemScript, std::vector<unsigned char> &vchPubKey);
bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, unsigned char& safetyLevel, bool& safeSearch, std::vector<unsigned char> &vchRedeemScript, std::vector<unsigned char> &vchPubKey);
CAmount convertCurrencyCodeToSyscoin(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const double &nPrice, const unsigned int &nHeight, int &precision);
int getFeePerByte(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const unsigned int &nHeight, int &precision);
float getEscrowFee(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const unsigned int &nHeight, int &precision);
CAmount convertSyscoinToCurrencyCode(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrencyCode, const CAmount &nPrice, const unsigned int &nHeight, int &precision);
std::string getCurrencyToSYSFromAlias(const std::vector<unsigned char> &vchAliasPeg, const std::vector<unsigned char> &vchCurrency, double &nFee, const unsigned int &nHeightToFind, std::vector<std::string>& rateList, int &precision, int &nFeePerByte, float &fEscrowFee);
std::string aliasFromOp(int op);
std::string GenerateSyscoinGuid();
bool IsAliasOp(int op);
bool RemoveAliasScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
int GetSyscoinDataOutput(const CTransaction& tx);
bool IsSyscoinDataOutput(const CTxOut& out);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, std::vector<unsigned char> &vchHash, int& nOut);
bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData, std::vector<unsigned char> &vchHash);
bool IsSysServiceExpired(const uint64_t &nTime);
bool GetTimeToPrune(const CScript& scriptPubKey, uint64_t &nTime);
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, const Consensus::Params& consensusParams);
bool IsSyscoinScript(const CScript& scriptPubKey, int &op, std::vector<std::vector<unsigned char> > &vvchArgs);
void RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut);
void PutToAliasList(std::vector<CAliasIndex> &aliasList, CAliasIndex& index);
void SysTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
void AliasTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
bool BuildAliasJson(const CAliasIndex& alias, const int pending, UniValue& oName);
void CleanupSyscoinServiceDatabases(int &servicesCleaned);
int aliasunspent(const std::vector<unsigned char> &vchAlias, COutPoint& outpoint);
bool IsMyAlias(const CAliasIndex& alias);
void GetAddress(const CAliasIndex &alias, CSyscoinAddress* address, const uint32_t nPaymentOption=1);
void GetAddress(const CAliasIndex &alias, CSyscoinAddress* address, CScript& script, const uint32_t nPaymentOption=1);
void GetPrivateKeysFromScript(const CScript& script, std::vector<std::string> &strKeys);
bool CheckParam(const UniValue& params, const unsigned int index);
#endif // ALIAS_H
