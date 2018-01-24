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
#include "ranges.h"
#include "graph.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CAliasIndex;

bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<unsigned char> &vvchAlias, bool fJustCheck, int nHeight, sorted_vector<CAssetAllocationTuple> &revertedAssetAllocations, std::string &errorMessage, bool dontaddtodb=false);
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
	// if allocations are tracked by individual inputs
	std::vector<CRange> listAllocationInputs;
    uint256 txHash;
    uint64_t nHeight;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
	CAmount nBalance;
	CAmount nTotalSupply;
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
		sCategory.clear();
		vchAlias.clear();
		listAllocationInputs.clear();

	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {		
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchAsset);
		READWRITE(sCategory);
		READWRITE(vchAlias);
		READWRITE(listAllocationInputs);
		READWRITE(nBalance);
		READWRITE(nTotalSupply);
	}
    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
		a.vchAsset == b.vchAsset
        );
    }

    inline CAsset operator=(const CAsset &b) {
		vchPubData = b.vchPubData;
		txHash = b.txHash;
        nHeight = b.nHeight;
		vchAlias = b.vchAlias;
		vchAsset = b.vchAsset;
		sCategory = b.sCategory;
		listAllocationInputs = b.listAllocationInputs;
		nBalance = b.nBalance;
		nTotalSupply = b.nTotalSupply;
        return *this;
    }

    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
	inline void SetNull() { nTotalSupply = 0; nBalance = 0; listAllocationInputs.clear(); sCategory.clear(); vchAsset.clear(); nHeight = 0; txHash.SetNull(); vchAlias.clear(); vchPubData.clear(); }
    inline bool IsNull() const { return (vchAsset.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}

    bool WriteAsset(const CAsset& asset, const int &op) {
		bool writeState = Write(make_pair(std::string("asseti"), asset.vchAsset), asset);
		WriteAssetIndex(asset, op);
        return writeState;
    }
	bool EraseAsset(const std::vector<unsigned char>& vchAsset, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("asseti"), vchAsset));
		EraseAssetIndex(vchAsset, cleanup);
		EraseAssetIndexHistory(vchAsset, cleanup);
		return eraseState;
	}
    bool ReadAsset(const std::vector<unsigned char>& vchAsset, CAsset& asset) {
        return Read(make_pair(std::string("asseti"), vchAsset), asset);
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
