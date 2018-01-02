// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSETALLOCATION_H
#define ASSETALLOCATION_H

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

bool CheckAssetAllocationInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeAssetAllocationTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseAssetAllocationTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char& type);
bool DecodeAssetAllocationScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsAssetAllocationOp(int op);
void AssetAllocationTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string assetAllocationFromOp(int op);
bool RemoveAssetAllocationScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
class CAssetAllocationTuple {
public:
	std::vector<unsigned char> vchAsset;
	std::vector<unsigned char> vchAlias;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchAsset);
		READWRITE(vchAlias);
	}

	CAssetAllocationTuple(const std::vector<unsigned char> &asset, const std::vector<unsigned char> &alias) {
		vchAsset = asset;
		vchAlias = alias;
	}

	CAssetAllocationTuple() {
		SetNull();
	}
	inline CAssetAllocationTuple operator=(const CAssetAllocationTuple& other) {
		this->vchAsset = other.vchAsset;
		this->vchAlias = other.vchAlias;
		return *this;
	}
	inline bool operator==(const CAssetAllocationTuple& other) const {
		return this->vchAsset == other.vchAsset && this->vchAlias == other.vchAlias;
	}
	inline bool operator!=(const CAssetAllocationTuple& other) const {
		return (this->vchAsset != other.vchAsset || this->vchAlias != other.vchAlias);
	}
	inline void SetNull() {
		vchAsset.clear();
		vchAlias.clear();
	}
	inline bool IsNull() {
		return (vchAsset.empty() && vchAlias.empty());
	}
};
class CAssetAllocation {
public:
	std::vector<unsigned char> vchAsset;
	// current owner
	std::vector<unsigned char> vchAlias;
	// previous owner
	std::vector<unsigned char> vchLinkAlias;
	uint256 txHash;
	uint64_t nHeight;
	// if allocations are tracked by individual outputs
	std::vector<std::string> listAllocations;
	CAmount nAmount;
	CAssetAllocation() {
		SetNull();
	}
	CAssetAllocation(const CTransaction &tx) {
		SetNull();
		UnserializeFromTx(tx);
	}
	ADD_SERIALIZE_METHODS;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchAsset);
		READWRITE(vchAlias);
		READWRITE(vchLinkAlias);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(listAllocations);
		READWRITE(nAmount);
	}
	inline friend bool operator==(const CAssetAllocation &a, const CAssetAllocation &b) {
		return (a.vchAsset == b.vchAsset && a.vchAlias == b.vchAlias
			);
	}

	inline CAssetAllocation operator=(const CAssetAllocation &b) {
		vchAsset = b.vchAsset;
		txHash = b.txHash;
		nHeight = b.nHeight;
		vchAlias = b.vchAlias;
		vchLinkAlias = b.vchLinkAlias;
		listAllocations = b.listAllocations;
		nAmount = b.nAmount;
		return *this;
	}

	inline friend bool operator!=(const CAssetAllocation &a, const CAssetAllocation &b) {
		return !(a == b);
	}
	inline void SetNull() { listAllocations.clear();  nAmount = 0; vchAsset.clear(); vchLinkAlias.clear(); nHeight = 0; txHash.SetNull(); vchAlias.clear(); }
	inline bool IsNull() const { return (vchAsset.empty()); }
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CAssetAllocationDB : public CDBWrapper {
public:
	CAssetAllocationDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetallocations", nCacheSize, fMemory, fWipe) {}

    bool WriteAssetAllocation(const CAssetAllocation& assetallocation, const CAssetAllocation& prevAssetAllocation, const int &op, const bool& fJustCheck) {
		const CAssetAllocationTuple allocationTuple(assetallocation.vchAsset, assetallocation.vchAlias);
		bool writeState = Write(make_pair(std::string("assetallocationi"), allocationTuple), assetallocation);
		if (!fJustCheck && !prevAssetAllocation.IsNull())
			writeState = writeState && Write(make_pair(std::string("assetallocationp"), allocationTuple), prevAssetAllocation);
		else if (fJustCheck)
			writeState = writeState && Write(make_pair(std::string("assetallocationl"), allocationTuple), fJustCheck);
		WriteAssetAllocationIndex(assetallocation, op);
        return writeState;
    }
	bool EraseAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("assetallocationi"), assetAllocationTuple));
		Erase(make_pair(std::string("assetp"), assetAllocationTuple));
		EraseISLock(assetAllocationTuple);
		EraseAssetAllocationIndex(assetAllocationTuple, cleanup);
		return eraseState;
	}
	bool CleanupDatabase(int &servicesCleaned);
    bool ReadAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple, CAssetAllocation& assetallocation) {
        return Read(make_pair(std::string("assetallocationi"), assetAllocationTuple), assetallocation);
    }
	bool ReadLastAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple, CAssetAllocation& assetallocation) {
		return Read(make_pair(std::string("assetallocationp"), assetAllocationTuple), assetallocation);
	}
	bool ReadISLock(const CAssetAllocationTuple& assetAllocationTuple, bool& lock) {
		return Read(make_pair(std::string("assetallocationl"), assetAllocationTuple), lock);
	}
	bool EraseISLock(const CAssetAllocationTuple& assetAllocationTuple) {
		return Erase(make_pair(std::string("assetallocationl"), assetAllocationTuple));
	}
	void WriteAssetAllocationIndex(const CAssetAllocation& assetAllocationTuple, const int &op);
	void EraseAssetAllocationIndex(const CAssetAllocationTuple& assetAllocationTuple, bool cleanup);

};
bool GetAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple,CAssetAllocation& txPos);
bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, UniValue& oName);
bool BuildAssetAllocationIndexerJson(const CAssetAllocation& assetallocation,UniValue& oName);
uint64_t GetAssetAllocationExpiration(const CAssetAllocation& assetallocation);
#endif // ASSETALLOCATION_H
