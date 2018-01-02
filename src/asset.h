// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSET_H
#define ASSET_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
#include "primitives/transaction.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CAliasIndex;

bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeAssetTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char& type);
bool DecodeAssetScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsAssetOp(int op);
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string assetFromOp(int op);
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut);


class CAsset {
public:
	std::vector<unsigned char> vchAsset;
	std::vector<unsigned char> vchAlias;
	// to modify alias in assettransfer
	std::vector<unsigned char> vchLinkAlias;
	// if allocations are tracked by individual outputs
	std::vector<std::string> listAllocations;
    uint256 txHash;
    uint64_t nHeight;
	std::vector<unsigned char> vchName;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
	CAmount nAmount;
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
		READWRITE(vchLinkAlias);
		READWRITE(vchAsset);
		READWRITE(sCategory);
		READWRITE(vchAlias);
		READWRITE(nAmount);
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
		vchAlias = b.vchAlias;
		vchLinkAlias = b.vchLinkAlias;
		vchAsset = b.vchAsset;
		sCategory = b.sCategory;
		nAmount = b.nAmount;
        return *this;
    }

    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
	inline void SetNull() { nAmount = 0; sCategory.clear(); vchName.clear(); vchLinkAlias.clear(); vchAsset.clear(); nHeight = 0; txHash.SetNull(); vchAlias.clear(); vchPubData.clear(); }
    inline bool IsNull() const { return (vchAsset.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}

    bool WriteAsset(const CAsset& asset, const CAsset& prevAsset, const int &op, const bool& fJustCheck) {
		bool writeState = Write(make_pair(std::string("asseti"), asset.vchAsset), asset);
		if (!fJustCheck && !prevAsset.IsNull())
			writeState = writeState && Write(make_pair(std::string("assetp"), asset.vchAsset), prevAsset);
		else if (fJustCheck)
			writeState = writeState && Write(make_pair(std::string("assetl"), asset.vchAsset), fJustCheck);
		WriteAssetIndex(asset, op);
        return writeState;
    }
	bool EraseAsset(const std::vector<unsigned char>& vchAsset, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("asseti"), vchAsset));
		Erase(make_pair(std::string("assetp"), vchAsset));
		EraseISLock(vchAsset);
		EraseAssetIndex(vchAsset, cleanup);
		return eraseState;
	}
    bool ReadAsset(const std::vector<unsigned char>& vchAsset, CAsset& asset) {
        return Read(make_pair(std::string("asseti"), vchAsset), asset);
    }
	bool ReadLastAsset(const std::vector<unsigned char>& vchGuid, CAsset& asset) {
		return Read(make_pair(std::string("assetp"), vchGuid), asset);
	}
	bool ReadISLock(const std::vector<unsigned char>& vchGuid, bool& lock) {
		return Read(make_pair(std::string("assetl"), vchGuid), lock);
	}
	bool EraseISLock(const std::vector<unsigned char>& vchGuid) {
		return Erase(make_pair(std::string("assetl"), vchGuid));
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteAssetIndex(const CAsset& asset, const int &op);
	void EraseAssetIndex(const std::vector<unsigned char>& vchAsset, bool cleanup);
	void WriteAssetIndexHistory(const CAsset& asset, const int &op);
	void EraseAssetIndexHistory(const std::vector<unsigned char>& vchAsset, bool cleanup);
	void EraseAssetIndexHistory(const std::string& id);

};
bool GetAsset(const std::vector<unsigned char> &vchAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, UniValue& oName);
bool BuildAssetIndexerJson(const CAsset& asset,UniValue& oName);
bool BuildAssetIndexerHistoryJson(const CAsset& asset, UniValue& oName);
uint64_t GetAssetExpiration(const CAsset& asset);
#endif // ASSET_H
