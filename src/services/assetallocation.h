// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETALLOCATION_H
#define SYSCOIN_SERVICES_ASSETALLOCATION_H

#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <unordered_map>
#include <services/graph.h>
#include <txmempool.h>
#include <services/witnessaddress.h>
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CBlock;
class CAsset;
class CMintSyscoin;
bool AssetMintTxToJson(const CTransaction& tx, UniValue &entry);
bool AssetMintTxToJson(const CTransaction& tx, const CMintSyscoin& mintsyscoin, const int& nHeight, UniValue &entry);

std::string assetAllocationFromTx(const int &nVersion);

class CAssetAllocationTuple {
public:
	uint32_t nAsset;
	CWitnessAddress witnessAddress;
	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(nAsset);
		READWRITE(witnessAddress);
	}
	CAssetAllocationTuple(const uint32_t &asset, const CWitnessAddress &witnessAddress_) {
		nAsset = asset;
		witnessAddress = witnessAddress_;
	}
	CAssetAllocationTuple(const uint32_t &asset) {
		nAsset = asset;
		witnessAddress.SetNull();
	}
	CAssetAllocationTuple() {
		SetNull();
	}
    CAssetAllocationTuple(CAssetAllocationTuple & other) = delete;
    CAssetAllocationTuple(CAssetAllocationTuple && other) = default;
    CAssetAllocationTuple& operator=(CAssetAllocationTuple& other) = delete;
    CAssetAllocationTuple& operator=(CAssetAllocationTuple&& other) = default;
    
	inline bool operator==(const CAssetAllocationTuple& other) const {
		return this->nAsset == other.nAsset && this->witnessAddress == other.witnessAddress;
	}
	inline bool operator!=(const CAssetAllocationTuple& other) const {
		return (this->nAsset != other.nAsset || this->witnessAddress != other.witnessAddress);
	}
	inline bool operator< (const CAssetAllocationTuple& right) const
	{
		return ToString() < right.ToString();
	}
	inline void SetNull() {
		nAsset = 0;
		witnessAddress.SetNull();
	}
	std::string ToString() const;
	inline bool IsNull() const {
		return (nAsset == 0 && witnessAddress.IsNull());
	}
};
typedef std::unordered_map<std::string, CAmount> AssetBalanceMap;
typedef std::unordered_map<uint256, int64_t,SaltedTxidHasher> ArrivalTimesMap;
typedef std::unordered_map<std::string, ArrivalTimesMap> ArrivalTimesMapImpl;
typedef std::vector<std::pair<CWitnessAddress, CAmount > > RangeAmountTuples;
static const int ZDAG_MINIMUM_LATENCY_SECONDS = 10;
static const int ONE_YEAR_IN_BLOCKS = 525600;
static const int ONE_HOUR_IN_BLOCKS = 60;
static const int ONE_MONTH_IN_BLOCKS = 43800;
enum {
	ZDAG_NOT_FOUND = -1,
	ZDAG_STATUS_OK = 0,
	ZDAG_MINOR_CONFLICT,
	ZDAG_MAJOR_CONFLICT
};

class CAssetAllocation {
public:
	CAssetAllocationTuple assetAllocationTuple;
	RangeAmountTuples listSendingAllocationAmounts;
	CAmount nBalance;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(assetAllocationTuple);
		READWRITE(listSendingAllocationAmounts);
		READWRITE(nBalance);
	}
	CAssetAllocation() {
		SetNull();
	}
	CAssetAllocation(const CTransaction &tx) {
		SetNull();
		UnserializeFromTx(tx);
	}
	inline void ClearAssetAllocation()
	{
		listSendingAllocationAmounts.clear();
	}
	ADD_SERIALIZE_METHODS;

	inline friend bool operator==(const CAssetAllocation &a, const CAssetAllocation &b) {
		return (a.assetAllocationTuple == b.assetAllocationTuple
			);
	}
    CAssetAllocation(const CAssetAllocation&) = delete;
    CAssetAllocation(CAssetAllocation && other) = default;
    CAssetAllocation& operator=( CAssetAllocation& a ) = delete;
	CAssetAllocation& operator=( CAssetAllocation&& a ) = default;
 
	inline friend bool operator!=(const CAssetAllocation &a, const CAssetAllocation &b) {
		return !(a == b);
	}
	inline void SetNull() { ClearAssetAllocation(); nBalance = 0;}
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData);
	void Serialize(std::vector<unsigned char>& vchData);
};
typedef std::unordered_map<std::string, CAssetAllocation > AssetAllocationMap;
class CAssetAllocationDB : public CDBWrapper {
public:
	CAssetAllocationDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetallocations", nCacheSize, fMemory, fWipe) {}
    
    bool ReadAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple, CAssetAllocation& assetallocation) {
        return Read(assetAllocationTuple, assetallocation);
    }
    bool Flush(const AssetAllocationMap &mapAssetAllocations);
	void WriteAssetAllocationIndex(const CTransaction &tx, const CAsset& dbAsset, const bool& confirmed, int nHeight);
    void WriteMintIndex(const CTransaction& tx, const CMintSyscoin& mintSyscoin, const int &nHeight);
	bool ScanAssetAllocations(const int count, const int from, const UniValue& oOptions, UniValue& oRes);
};

class CAssetAllocationMempoolDB : public CDBWrapper {
public:
    CAssetAllocationMempoolDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetallocationmempoolbalances", nCacheSize, fMemory, fWipe) {
    }

    bool WriteAssetAllocationMempoolBalances(const AssetBalanceMap &valueMap) {
        return Write(std::string("assetallocationtxbalance"), valueMap, true);
    }
    bool ReadAssetAllocationMempoolBalances(AssetBalanceMap &valueMap) {
        return Read(std::string("assetallocationtxbalance"), valueMap);
    }
    bool WriteAssetAllocationMempoolArrivalTimes(const ArrivalTimesMapImpl &valueMap) {
        return Write(std::string("assetallocationtxarrival"), valueMap, true);
    }
    bool ReadAssetAllocationMempoolArrivalTimes(ArrivalTimesMapImpl &valueMap) {
        return Read(std::string("assetallocationtxarrival"), valueMap);
    }   
    bool ScanAssetAllocationMempoolBalances(const int count, const int from, const UniValue& oOptions, UniValue& oRes);
};
bool GetAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple,CAssetAllocation& txPos);
bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, const CAsset& asset, UniValue& oName);
bool AssetAllocationTxToJSON(const CTransaction &tx, const CAsset& dbAsset, const int& nHeight, const bool& confirmed, UniValue &entry, CAssetAllocation& assetallocation);
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry);
void WriteAssetIndexForAllocation(const CAssetAllocation& assetallocation, const uint256& txid, const UniValue& oName);
void WriteAssetIndexForAllocation(const CMintSyscoin& mintSyscoin, const uint256& txid, const UniValue& oName);
void WriteAssetAllocationIndexTXID(const CAssetAllocationTuple& allocationTuple, const uint256& txid);
#endif // SYSCOIN_SERVICES_ASSETALLOCATION_H
