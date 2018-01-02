// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
static const unsigned int MAX_VALUE_LENGTH = 512;
static const unsigned int MAX_ID_LENGTH = 20;
static const unsigned int MAX_ENCRYPTED_GUID_LENGTH = MAX_GUID_LENGTH + 85;
static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;
enum {
	ALIAS=0,
	OFFER, 
	CERT,
	ESCROW,
	ASSET,
	ASSETALLOCATION
};
enum {
	ACCEPT_TRANSFER_NONE=0,
	ACCEPT_TRANSFER_CERTIFICATES,
	ACCEPT_TRANSFER_ASSETS,
	ACCEPT_TRANSFER_ALL,
};
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

	inline void SetNull() { vchGUID.clear(); nExpireTime = 0; }
    inline bool IsNull() const { return (vchGUID.empty()); }
};
class COfferLinkWhitelistEntry {
public:
	std::vector<unsigned char> aliasLinkVchRand;
	unsigned char nDiscountPct;
	COfferLinkWhitelistEntry() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(aliasLinkVchRand);
		READWRITE(VARINT(nDiscountPct));
	}

	inline friend bool operator==(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
		return (
			a.aliasLinkVchRand == b.aliasLinkVchRand
			&& a.nDiscountPct == b.nDiscountPct
			);
	}

	inline COfferLinkWhitelistEntry operator=(const COfferLinkWhitelistEntry &b) {
		aliasLinkVchRand = b.aliasLinkVchRand;
		nDiscountPct = b.nDiscountPct;
		return *this;
	}

	inline friend bool operator!=(const COfferLinkWhitelistEntry &a, const COfferLinkWhitelistEntry &b) {
		return !(a == b);
	}

	inline void SetNull() {
		aliasLinkVchRand.clear(); 
		nDiscountPct = 0;
	}
	inline bool IsNull() const { return (aliasLinkVchRand.empty()); }

};
typedef std::map<std::vector<unsigned char>, COfferLinkWhitelistEntry> whitelistMap_t;
class COfferLinkWhitelist {
public:
	whitelistMap_t entries;
	COfferLinkWhitelist() {
		SetNull();
	}

	ADD_SERIALIZE_METHODS;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(entries);
	}
	bool GetLinkEntryByHash(const std::vector<unsigned char> &ahash, COfferLinkWhitelistEntry &entry) const;

	inline void RemoveWhitelistEntry(const std::vector<unsigned char> &ahash) {
		entries.erase(ahash);
	}
	inline void PutWhitelistEntry(const COfferLinkWhitelistEntry &theEntry) {
		if (entries.count(theEntry.aliasLinkVchRand) > 0) {
			entries[theEntry.aliasLinkVchRand] = theEntry;
			return;
		}
		entries[theEntry.aliasLinkVchRand] = theEntry;
	}
	inline friend bool operator==(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
		return (
			a.entries == b.entries
			);
	}

	inline COfferLinkWhitelist operator=(const COfferLinkWhitelist &b) {
		entries = b.entries;
		return *this;
	}

	inline friend bool operator!=(const COfferLinkWhitelist &a, const COfferLinkWhitelist &b) {
		return !(a == b);
	}

	inline void SetNull() { entries.clear(); }
	inline bool IsNull() const { return (entries.empty()); }

};
class CAliasIndex {
public:
	std::vector<unsigned char> vchAlias;
	std::vector<unsigned char> vchGUID;
    uint256 txHash;
    uint64_t nHeight;
	uint64_t nExpireTime;
	std::vector<unsigned char> vchAddress;
	std::vector<unsigned char> vchEncryptionPublicKey;
	std::vector<unsigned char> vchEncryptionPrivateKey;
	// 1 accepts certs, 2 accepts assets, 3 accepts both (default)
	unsigned char nAcceptTransferFlags;
	// 1 can edit, 2 can edit/transfer
	unsigned char nAccessFlags;
	std::vector<unsigned char> vchPublicValue;
	// to control reseller access + wholesaler discounts
	COfferLinkWhitelist offerWhitelist;
    CAliasIndex() { 
        SetNull();
    }
    CAliasIndex(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	inline void ClearAlias()
	{
		vchEncryptionPublicKey.clear();
		vchEncryptionPrivateKey.clear();
		vchPublicValue.clear();
		vchGUID.clear();
		vchAddress.clear();
		offerWhitelist.SetNull();
	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {        
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchPublicValue);
		READWRITE(vchEncryptionPublicKey);
		READWRITE(vchEncryptionPrivateKey);
		READWRITE(vchAlias);
		READWRITE(vchGUID);
		READWRITE(VARINT(nExpireTime));
		READWRITE(VARINT(nAcceptTransferFlags));
		READWRITE(VARINT(nAccessFlags));
		READWRITE(vchAddress);	
		READWRITE(offerWhitelist);
	}
    inline friend bool operator==(const CAliasIndex &a, const CAliasIndex &b) {
		return (a.vchGUID == b.vchGUID && a.vchAlias == b.vchAlias);
    }

    inline friend bool operator!=(const CAliasIndex &a, const CAliasIndex &b) {
        return !(a == b);
    }
    inline CAliasIndex operator=(const CAliasIndex &b) {
		vchGUID = b.vchGUID;
		nExpireTime = b.nExpireTime;
		vchAlias = b.vchAlias;
        txHash = b.txHash;
        nHeight = b.nHeight;
        vchPublicValue = b.vchPublicValue;
		vchAddress = b.vchAddress;
		nAcceptTransferFlags = b.nAcceptTransferFlags;
		vchEncryptionPrivateKey = b.vchEncryptionPrivateKey;
		vchEncryptionPublicKey = b.vchEncryptionPublicKey;
		nAccessFlags = b.nAccessFlags;
		offerWhitelist = b.offerWhitelist;
        return *this;
    }   
	inline void SetNull() { offerWhitelist.SetNull(); nAccessFlags = 2; vchAddress.clear(); vchEncryptionPublicKey.clear(); vchEncryptionPrivateKey.clear(); nAcceptTransferFlags = 3; nExpireTime = 0; vchGUID.clear(); vchAlias.clear(); txHash.SetNull(); nHeight = 0; vchPublicValue.clear(); }
    inline bool IsNull() const { return (vchAlias.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};

class CAliasDB : public CDBWrapper {
public:
    CAliasDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "aliases", nCacheSize, fMemory, fWipe) {
    }
	bool WriteAlias(const CAliasUnprunable &aliasUnprunable, const std::vector<unsigned char>& address, const CAliasIndex& alias, const CAliasIndex& prevAlias, const int &op, const bool &fJustCheck) {
		if(address.empty())
			return false;	
		bool writeState = Write(make_pair(std::string("namei"), alias.vchAlias), alias) && Write(make_pair(std::string("namea"), address), alias.vchAlias) && Write(make_pair(std::string("nameu"), alias.vchAlias), aliasUnprunable);
		if (!fJustCheck && !prevAlias.IsNull())
			writeState = writeState && Write(make_pair(std::string("namep"), alias.vchAlias), prevAlias);
		else if(fJustCheck)
			writeState = writeState && Write(make_pair(std::string("namel"), alias.vchAlias), fJustCheck);
		WriteAliasIndex(alias, op);
		return writeState;
	}

	bool EraseAlias(const std::vector<unsigned char>& vchAlias, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("namei"), vchAlias));
		Erase(make_pair(std::string("namep"), vchAlias));
		EraseISLock(vchAlias);
		EraseAliasIndex(vchAlias, cleanup);
		return eraseState;
	}
	bool ReadAlias(const std::vector<unsigned char>& vchAlias, CAliasIndex& alias) {
		return Read(make_pair(std::string("namei"), vchAlias), alias);
	}
	bool ReadLastAlias(const std::vector<unsigned char>& vchGuid, CAliasIndex& alias) {
		return Read(make_pair(std::string("namep"), vchGuid), alias);
	}
	bool ReadISLock(const std::vector<unsigned char>& vchGuid, bool& lock) {
		return Read(make_pair(std::string("namel"), vchGuid), lock);
	}
	bool EraseISLock(const std::vector<unsigned char>& vchGuid) {
		return Erase(make_pair(std::string("namel"), vchGuid));
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
	bool CleanupDatabase(int &servicesCleaned);
	void WriteAliasIndex(const CAliasIndex& alias, const int &op);
	void EraseAliasIndex(const std::vector<unsigned char>& vchAlias, bool cleanup);
	void WriteAliasIndexHistory(const CAliasIndex& alias, const int &op);
	void EraseAliasIndexHistory(const std::vector<unsigned char>& vchAlias, bool cleanup);
	void EraseAliasIndexHistory(const std::string& id);
	void WriteAliasIndexTxHistory(const std::string &user1, const std::string &user2, const std::string &user3, const uint256 &txHash, const uint64_t& nHeight, const std::string &type, const std::string &guid);
	void EraseAliasIndexTxHistory(const std::vector<unsigned char>& vchAlias, bool cleanup);
	void EraseAliasIndexTxHistory(const std::string& id);
};

class COfferDB;
class CCertDB;
class CEscrowDB;
class CAssetDB;
extern CAliasDB *paliasdb;
extern COfferDB *pofferdb;
extern CCertDB *pcertdb;
extern CEscrowDB *pescrowdb;
extern CAssetDB *passetdb;
extern CAssetDB *passetallocationdb;

std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
int GetSyscoinTxVersion();
const int SYSCOIN_TX_VERSION = 0x7400;
bool IsValidAliasName(const std::vector<unsigned char> &vchAlias);
bool CheckAliasInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool & bDestCheckFailed,bool dontaddtodb=false);
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient);
void CreateFeeRecipient(CScript& scriptPubKey, const std::vector<unsigned char>& data, CRecipient& recipient);
void CreateAliasRecipient(const CScript& scriptPubKey, CRecipient& recipient);
int aliasselectpaymentcoins(const std::vector<unsigned char> &vchAlias, const CAmount &nAmount, std::vector<COutPoint>& outPoints, bool& bIsFunded, CAmount &nRequiredAmount, bool bSelectFeePlacement, bool bSelectAll=false, bool bNoAliasRecipient=false);
CAmount GetDataFee(const CScript& scriptPubKey);
bool IsAliasOp(int op);
bool GetAlias(const std::vector<unsigned char> &vchAlias, CAliasIndex& alias);
bool CheckParam(const UniValue& params, const unsigned int index);
bool GetAliasOfTx(const CTransaction& tx, std::vector<unsigned char>& name);
bool DecodeAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAliasTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char &type);
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char &type);
bool DecodeAliasScript(const CScript& script, int& op,
		std::vector<std::vector<unsigned char> > &vvch);
bool GetAddressFromAlias(const std::string& strAlias, std::string& strAddress, std::vector<unsigned char> &vchPubKey);
bool GetAliasFromAddress(const std::string& strAddress, std::string& strAlias, std::vector<unsigned char> &vchPubKey);
int getFeePerByte(const uint64_t &paymentOptionMask);
float getEscrowFee();
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
void SysTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry, const char& type);
void AliasTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
bool BuildAliasJson(const CAliasIndex& alias, UniValue& oName);
void CleanupSyscoinServiceDatabases(int &servicesCleaned);
int aliasunspent(const std::vector<unsigned char> &vchAlias, COutPoint& outPoint);
bool IsMyAlias(const CAliasIndex& alias);
void GetAddress(const CAliasIndex &alias, CSyscoinAddress* address, CScript& script, const uint32_t nPaymentOption=1);
void startMongoDB();
void stopMongoDB();
std::string GetSyscoinTransactionDescription(const int op, const std::vector<std::vector<unsigned char> > &vvchArgs, std::string& responseEnglish, std::string& responseGUID, const char &type);
bool BuildAliasIndexerHistoryJson(const CAliasIndex& alias, UniValue& oName);
#endif // ALIAS_H
