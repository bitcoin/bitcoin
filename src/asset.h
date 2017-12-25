// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSET_H
#define ASSET_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "feedback.h"
#include "primitives/transaction.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CAliasIndex;

bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool bInstantSend = false,bool dontaddtodb=false);
bool DecodeAssetTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char& type);
bool DecodeAssetScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsAssetOp(int op);
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string assetFromOp(int op);
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
/*class CAssetAllocation {
public:
	std::vector<unsigned char> vchAssetAllocation;
	// current owner
	CNameTXIDTuple aliasTuple;
	// previous owner
	CNameTXIDTuple linkAliasTuple;
	uint256 txHash;
	uint64_t nHeight;
	std::vector<std::string> listReceivers;
	CAmount nAmount;
	bool bInstantSendLocked;
	CAsset() {
		SetNull();
	}
	CAsset(const CTransaction &tx) {
		SetNull();
		UnserializeFromTx(tx);
	}
	ADD_SERIALIZE_METHODS;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchName);
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(linkAliasTuple);
		READWRITE(vchAssetAllocation);
		READWRITE(sCategory);
		READWRITE(aliasTuple);
		READWRITE(nAmount);
		READWRITE(bInstantSendLocked);
	}
	inline friend bool operator==(const CAsset &a, const CAsset &b) {
		return (a.vchAssetAllocation == b.vchAssetAllocation
			);
	}

	inline CAsset operator=(const CAsset &b) {
		vchName = b.vchName;
		vchPubData = b.vchPubData;
		txHash = b.txHash;
		nHeight = b.nHeight;
		aliasTuple = b.aliasTuple;
		linkAliasTuple = b.linkAliasTuple;
		vchAssetAllocation = b.vchAssetAllocation;
		sCategory = b.sCategory;
		nAmount = b.nAmount;
		bInstantSendLocked = b.bInstantSendLocked;
		return *this;
	}

	inline friend bool operator!=(const CAsset &a, const CAsset &b) {
		return !(a == b);
	}
	inline void SetNull() { bInstantSendLocked = false; nAmount = 0; sCategory.clear(); vchName.clear(); linkAliasTuple.first.clear(); vchAssetAllocation.clear(); nHeight = 0; txHash.SetNull(); aliasTuple.first.clear(); vchPubData.clear(); }
	inline bool IsNull() const { return (vchAssetAllocation.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};*/

class CAsset {
public:
	std::vector<unsigned char> vchAsset;
	CNameTXIDTuple ownerAliasTuple;
	CNameTXIDTuple aliasTuple;
	// to modify alias in assettransfer
	CNameTXIDTuple linkAliasTuple;
    uint256 txHash;
    uint64_t nHeight;
	std::vector<unsigned char> vchName;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
	CAmount nAmount;
	bool bInstantSendLocked;
    CAsset() {
        SetNull();
    }
    CAsset(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	inline void ClearAsset()
	{
		vchPubData.clear();
		vchName.clear();
		sCategory.clear();
	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {		
		READWRITE(vchName);
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(linkAliasTuple);
		READWRITE(vchAsset);
		READWRITE(sCategory);
		READWRITE(aliasTuple);
		READWRITE(nAmount);
		READWRITE(bInstantSendLocked);
	}
    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
		a.vchAsset == b.vchAsset
        );
    }

    inline CAsset operator=(const CAsset &b) {
		vchName = b.vchName;
		vchPubData = b.vchPubData;
		txHash = b.txHash;
        nHeight = b.nHeight;
		aliasTuple = b.aliasTuple;
		linkAliasTuple = b.linkAliasTuple;
		vchAsset = b.vchAsset;
		sCategory = b.sCategory;
		nAmount = b.nAmount;
        return *this;
    }

    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
	inline void SetNull() { nAmount = 0; sCategory.clear(); vchName.clear(); linkAliasTuple.first.clear(); vchAsset.clear(); nHeight = 0; txHash.SetNull(); aliasTuple.first.clear(); vchPubData.clear(); }
    inline bool IsNull() const { return (vchAsset.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}

    bool WriteAsset(const CAsset& asset, const CAsset& prevAsset, const int &op, const bool& bInstantSend) {
		bool writeState = WriteAssetLastTXID(asset.vchAsset, asset.txHash) && Write(make_pair(std::string("asseti"), CNameTXIDTuple(asset.vchAsset, asset.txHash)), asset);
		if (!bInstantSend && !prevAsset.IsNull())
			writeState = writeState && Write(make_pair(std::string("assetp"), make_pair(asset.vchAsset, prevAsset));
		WriteAssetIndex(asset, op);
        return writeState;
    }

    bool EraseAsset(const CNameTXIDTuple& assetTuple, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("asseti"), assetTuple));
		Erase(make_pair(std::string("assetp"), assetTuple.first));
		EraseAssetLastTXID(assetTuple.first);
		EraseAssetFirstTXID(assetTuple.first);
		EraseAssetIndex(assetTuple.first, cleanup);
        return eraseState;
    }

    bool ReadAsset(const CNameTXIDTuple& assetTuple, CAsset& asset) {
        return Read(make_pair(std::string("asseti"), assetTuple), asset);
    }
	bool WriteAssetLastTXID(const std::vector<unsigned char>& asset, const uint256& txid) {
		return Write(make_pair(std::string("assetlt"), asset), txid);
	}
	bool ReadAssetLastTXID(const std::vector<unsigned char>& asset, uint256& txid) {
		return Read(make_pair(std::string("assetlt"), asset), txid);
	}
	bool EraseAssetLastTXID(const std::vector<unsigned char>& asset) {
		return Erase(make_pair(std::string("assetlt"), asset));
	}
	bool WriteAssetFirstTXID(const std::vector<unsigned char>& asset, const uint256& txid) {
		return Write(make_pair(std::string("assetft"), asset), txid);
	}
	bool ReadAssetFirstTXID(const std::vector<unsigned char>& asset, uint256& txid) {
		return Read(make_pair(std::string("assetft"), asset), txid);
	}
	bool EraseAssetFirstTXID(const std::vector<unsigned char>& asset) {
		return Erase(make_pair(std::string("assetft"), asset));
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteAssetIndex(const CAsset& asset, const int &op);
	void EraseAssetIndex(const std::vector<unsigned char>& vchAsset, bool cleanup);
	void WriteAssetIndexHistory(const CAsset& asset, const int &op);
	void EraseAssetIndexHistory(const std::vector<unsigned char>& vchAsset, bool cleanup);

};
bool GetAsset(const CNameTXIDTuple& assetTuple);
bool GetAsset(const std::vector<unsigned char> &vchAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, UniValue& oName);
bool BuildAssetIndexerJson(const CAsset& asset,UniValue& oName);
bool BuildAssetIndexerHistoryJson(const CAsset& asset, UniValue& oName);
uint64_t GetAssetExpiration(const CAsset& asset);
#endif // ASSET_H
