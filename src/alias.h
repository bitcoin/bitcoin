#ifndef ALIAS_H
#define ALIAS_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "feedback.h"
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
class CCoinControl;
struct CRecipient;
static const unsigned int MAX_GUID_LENGTH = 71;
static const unsigned int MAX_NAME_LENGTH = 256;
static const unsigned int MAX_VALUE_LENGTH = 1024;
static const unsigned int MAX_ID_LENGTH = 20;
static const unsigned int MAX_ENCRYPTED_VALUE_LENGTH = MAX_VALUE_LENGTH + 85;
static const unsigned int MAX_ENCRYPTED_NAME_LENGTH = MAX_NAME_LENGTH + 85;
static const unsigned int MAX_ENCRYPTED_GUID_LENGTH = MAX_GUID_LENGTH + 85;
static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;

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

    inline void SetNull() { vchGUID.clear(); nExpireTime=0;}
    inline bool IsNull() const { return (vchGUID.empty() && nExpireTime == 0); }
};

class CAliasIndex {
public:
	std::vector<unsigned char> vchAlias;
	std::vector<unsigned char> vchGUID;
    uint256 txHash;
    uint64_t nHeight;
	uint64_t nExpireTime;
	CNameTXIDTuple aliasPegTuple;
	std::vector<unsigned char> vchAddress;
	std::vector<unsigned char> vchEncryptionPublicKey;
	std::vector<unsigned char> vchEncryptionPrivateKey;
	bool acceptCertTransfers;
	// 1 can edit, 2 can edit/transfer
	unsigned char nAccessFlags;

	std::vector<unsigned char> vchPasswordSalt;

	std::vector<unsigned char> vchPublicValue;
	std::vector<unsigned char> vchPrivateValue;
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
		vchAddress.clear();
		vchPasswordSalt.clear();
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
		READWRITE(vchAlias);
		READWRITE(aliasPegTuple);
		READWRITE(vchGUID);
		READWRITE(VARINT(nExpireTime));
		READWRITE(acceptCertTransfers);
		READWRITE(VARINT(nAccessFlags));
		READWRITE(vchPasswordSalt);
		READWRITE(vchAddress);	
	}
    friend bool operator==(const CAliasIndex &a, const CAliasIndex &b) {
		return (a.aliasPegTuple == b.aliasPegTuple && a.nAccessFlags == b.nAccessFlags && a.vchAddress == b.vchAddress && a.vchEncryptionPublicKey == b.vchEncryptionPublicKey && a.vchEncryptionPrivateKey == b.vchEncryptionPrivateKey && a.vchPasswordSalt ==b.vchPasswordSalt && a.acceptCertTransfers == b.acceptCertTransfers && a.nExpireTime == b.nExpireTime && a.vchGUID == b.vchGUID && a.vchAlias == b.vchAlias && a.nHeight == b.nHeight && a.txHash == b.txHash && a.vchPublicValue == b.vchPublicValue && a.vchPrivateValue == b.vchPrivateValue);
    }

    friend bool operator!=(const CAliasIndex &a, const CAliasIndex &b) {
        return !(a == b);
    }
    CAliasIndex operator=(const CAliasIndex &b) {
		vchGUID = b.vchGUID;
		nExpireTime = b.nExpireTime;
		vchAlias = b.vchAlias;
		aliasPegTuple = b.aliasPegTuple;
        txHash = b.txHash;
        nHeight = b.nHeight;
        vchPublicValue = b.vchPublicValue;
        vchPrivateValue = b.vchPrivateValue;
		vchPasswordSalt = b.vchPasswordSalt;
		vchAddress = b.vchAddress;
		acceptCertTransfers = b.acceptCertTransfers;
		vchEncryptionPrivateKey = b.vchEncryptionPrivateKey;
		vchEncryptionPublicKey = b.vchEncryptionPublicKey;
		nAccessFlags = b.nAccessFlags;
        return *this;
    }   
    void SetNull() {nAccessFlags = 2; vchAddress.clear(); vchEncryptionPublicKey.clear();vchEncryptionPrivateKey.clear(); aliasPegTuple.first.clear(); vchPasswordSalt.clear();acceptCertTransfers = true; nExpireTime = 0; vchGUID.clear(); vchAlias.clear();txHash.SetNull(); nHeight = 0; vchPublicValue.clear(); vchPrivateValue.clear();}
    bool IsNull() const { return (nAccessFlags == 2 && vchAddress.empty() && vchEncryptionPublicKey.empty() && vchEncryptionPrivateKey.empty() && aliasPegTuple.first.empty() && vchPasswordSalt.empty() && acceptCertTransfers && nExpireTime == 0 && vchGUID.empty() && vchAlias.empty() && nHeight == 0 && txHash.IsNull() && vchPublicValue.empty() && vchPrivateValue.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class CAliasDB : public CDBWrapper {
public:
    CAliasDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "aliases", nCacheSize, fMemory, fWipe) {
    }
	bool WriteAlias(const CAliasIndex& alias) {
		bool writeState = Write(make_pair(std::string("namei"), CNameTXIDTuple(alias.vchAlias, alias.txHash)), alias);
		WriteAliasIndex(alias);
		return writeState;
	}
	bool WriteAlias(const CAliasUnprunable &aliasUnprunable, const std::vector<unsigned char>& address, const CAliasIndex& alias) {
		if(address.empty())
			return false;	
		bool writeState = WriteAliasLastTXID(alias.vchAlias, alias.txHash) && Write(make_pair(std::string("namei"), CNameTXIDTuple(alias.vchAlias, alias.txHash)), alias) && Write(make_pair(std::string("namea"), address), alias.vchAlias) && Write(make_pair(std::string("nameu"), alias.vchAlias), aliasUnprunable);
		WriteAliasIndex(alias);
		return writeState;
	}


	bool EraseAlias(const CNameTXIDTuple& aliasTuple) {
		bool eraseState = Erase(make_pair(std::string("namei"), CNameTXIDTuple(aliasTuple.first, aliasTuple.second)));
		EraseAliasLastTXID(aliasTuple.first);
		EraseAliasIndex(aliasTuple.first);
		return eraseState;
	}
	bool ReadAlias(const CNameTXIDTuple& aliasTuple, CAliasIndex& alias) {
		return Read(make_pair(std::string("namei"), CNameTXIDTuple(aliasTuple.first, aliasTuple.second)), alias);
	}
	bool ReadAddress(const std::vector<unsigned char>& address, std::vector<unsigned char>& name) {
		return Read(make_pair(std::string("namea"), address), name);
	}
	bool ReadAliasUnprunable(const std::vector<unsigned char>& alias, CAliasUnprunable& aliasUnprunable) {
		return Read(make_pair(std::string("nameu"), alias), aliasUnprunable);
	}
	bool EraseAddress(const std::vector<unsigned char>& address) {
	    return Erase(make_pair(std::string("namea"), address));
	}
	bool ExistsAddress(const std::vector<unsigned char>& address) {
	    return Exists(make_pair(std::string("namea"), address));
	}
	bool WriteAliasLastTXID(const std::vector<unsigned char>& alias, const uint256& txid) {
		return Write(make_pair(std::string("namelt"), alias), txid);
	}
	bool ReadAliasLastTXID(const std::vector<unsigned char>& alias, uint256& txid) {
		return Read(make_pair(std::string("namelt"), alias), txid);
	}
	bool EraseAliasLastTXID(const std::vector<unsigned char>& alias) {
		return Erase(make_pair(std::string("namelt"), alias));
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteAliasIndex(const CAliasIndex& alias);
	void EraseAliasIndex(const std::vector<unsigned char>& vchAlias);
};

class COfferDB;
class CCertDB;
class CEscrowDB;
extern CAliasDB *paliasdb;
extern COfferDB *pofferdb;
extern CCertDB *pcertdb;
extern CEscrowDB *pescrowdb;



std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
int GetSyscoinTxVersion();
const int SYSCOIN_TX_VERSION = 0x7400;
bool IsValidAliasName(const std::vector<unsigned char> &vchAlias);
bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient);
void CreateFeeRecipient(CScript& scriptPubKey, const CNameTXIDTuple& aliasPegTuple, const std::vector<unsigned char>& data, CRecipient& recipient);
void CreateAliasRecipient(const CScript& scriptPubKey, const std::vector<unsigned char>& vchAlias, const CNameTXIDTuple& aliasPegTuple, CRecipient& recipient);
int aliasselectpaymentcoins(const std::vector<unsigned char> &vchAlias, const CAmount &nAmount, std::vector<COutPoint>& outPoints, bool& bIsFunded, CAmount &nRequiredAmount, bool bSelectFeePlacementOnly, bool bSelectAll=false, bool bNoAliasRecipient=false);
CAmount GetDataFee(const CScript& scriptPubKey, const CNameTXIDTuple& aliasPegTuple);
bool IsSyscoinTxMine(const CTransaction& tx,const std::string &type);
bool IsAliasOp(int op, bool ismine=false);
bool GetAlias(const CNameTXIDTuple& aliasTuple, CAliasIndex& alias);
bool GetAlias(const std::vector<unsigned char> &vchAlias, CAliasIndex& alias);
bool CheckParam(const UniValue& params, const unsigned int index);
int IndexOfAliasOutput(const CTransaction& tx);
bool GetAliasOfTx(const CTransaction& tx, std::vector<unsigned char>& name);
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, bool payment=false);
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAliasScript(const CScript& script, int& op,
		std::vector<std::vector<unsigned char> > &vvch);
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, std::vector<unsigned char> &vchPubKey);
bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, std::vector<unsigned char> &vchPubKey);
CAmount convertCurrencyCodeToSyscoin(const CNameTXIDTuple& aliasPegTuple, const std::vector<unsigned char> &vchCurrencyCode, const double &nPrice,int &precision);
int getFeePerByte(const CNameTXIDTuple& aliasPegTuple, const std::vector<unsigned char> &vchCurrencyCode, int &precision);
float getEscrowFee(const CNameTXIDTuple& aliasPegTupleg, const std::vector<unsigned char> &vchCurrencyCode, int &precision);
CAmount convertSyscoinToCurrencyCode(const CNameTXIDTuple& aliasPegTuple, const std::vector<unsigned char> &vchCurrencyCode, const CAmount &nPrice, int &precision);
std::string getCurrencyToSYSFromAlias(const CNameTXIDTuple& aliasPegTuple, const std::vector<unsigned char> &vchCurrency, double &nFee, std::vector<std::string>& rateList, int &precision, int &nFeePerByte, float &fEscrowFee);
std::string aliasFromOp(int op);
std::string GenerateSyscoinGuid();
bool RemoveAliasScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
int GetSyscoinDataOutput(const CTransaction& tx);
bool IsSyscoinDataOutput(const CTxOut& out);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, std::vector<unsigned char> &vchHash, int& nOut);
bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData, std::vector<unsigned char> &vchHash);
bool IsSysServiceExpired(const uint64_t &nTime);
bool GetTimeToPrune(const CScript& scriptPubKey, uint64_t &nTime);
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, const Consensus::Params& consensusParams);
bool GetSyscoinTransaction(int nHeight, const uint256 &hash, CTransaction &txOut, uint256& hashBlock, const Consensus::Params& consensusParams);
bool IsSyscoinScript(const CScript& scriptPubKey, int &op, std::vector<std::vector<unsigned char> > &vvchArgs);
bool RemoveSyscoinScript(const CScript& scriptPubKeyIn, CScript& scriptPubKeyOut);
void PutToAliasList(std::vector<CAliasIndex> &aliasList, CAliasIndex& index);
void SysTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
void AliasTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
bool BuildAliasJson(const CAliasIndex& alias, UniValue& oName);
void CleanupSyscoinServiceDatabases(int &servicesCleaned);
int aliasunspent(const std::vector<unsigned char> &vchAlias, COutPoint& outPoint);
bool IsMyAlias(const CAliasIndex& alias);
void GetAddress(const CAliasIndex &alias, CSyscoinAddress* address, CScript& script, const uint32_t nPaymentOption=1);
void startMongoDB();
void stopMongoDB();
#endif // ALIAS_H
