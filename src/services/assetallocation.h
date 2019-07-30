// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETALLOCATION_H
#define SYSCOIN_SERVICES_ASSETALLOCATION_H

#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <unordered_map>
#include <txmempool.h>
#include <services/witnessaddress.h>
#ifdef ENABLE_WALLET
#include <wallet/ismine.h>
#endif
class CTransaction;
class CAsset;
class CMintSyscoin;
#ifdef ENABLE_WALLET
class CWallet;
#endif
bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, UniValue &entry);
bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, const CMintSyscoin& mintsyscoin, const int& nHeight,  const uint256& blockhash, UniValue &entry);

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
	explicit CAssetAllocationTuple(const uint32_t &asset) {
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
typedef std::vector<std::pair<uint256, uint32_t> > ArrivalTimesMap;
typedef std::unordered_map<std::string, ArrivalTimesMap> ArrivalTimesMapImpl;
typedef std::vector<std::pair<CWitnessAddress, CAmount > > RangeAmountTuples;
typedef std::unordered_set<std::string> ActorSet;
static ArrivalTimesMap emptyArrivalTimes;
static const int ONE_YEAR_IN_BLOCKS = 525600;
static const int ONE_HOUR_IN_BLOCKS = 60;
static const int ONE_MONTH_IN_BLOCKS = 43800;
enum {
	ZDAG_NOT_FOUND = -1,
	ZDAG_STATUS_OK = 0,
	ZDAG_WARNING_RBF,
	ZDAG_MAJOR_CONFLICT
};

class CAssetAllocation {
public:
	CAssetAllocationTuple assetAllocationTuple;
	RangeAmountTuples listSendingAllocationAmounts;
	CAmount nBalance;
	COutPoint lockedOutpoint;
	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(assetAllocationTuple);
		READWRITE(listSendingAllocationAmounts);
		READWRITE(nBalance);
		READWRITE(lockedOutpoint);
	}
	CAssetAllocation() {
		SetNull();
	}
	explicit CAssetAllocation(const CTransaction &tx) {
		SetNull();
		UnserializeFromTx(tx);
	}
	inline void ClearAssetAllocation()
	{
		listSendingAllocationAmounts.clear();
		lockedOutpoint.SetNull();
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
typedef std::unordered_map<std::string, COutPoint > AssetPrevTxMap;
class CAssetAllocationDB : public CDBWrapper {
public:
	CAssetAllocationDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetallocations", nCacheSize, fMemory, fWipe) {}
    
    bool ReadAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple, CAssetAllocation& assetallocation) {
        return Read(assetAllocationTuple, assetallocation);
    }
    bool ReadAssetsByAddress(const CWitnessAddress &address, std::vector<uint32_t> &assetGuids){
        return Read(address, assetGuids);
    }
    bool ExistsAssetsByAddress(const CWitnessAddress &address){
        return Exists(address);
    }	
    bool Flush(const AssetAllocationMap &mapAssetAllocations);
	bool WriteAssetAllocationIndex(const CTransaction &tx, const uint256& txHash, const CAsset& dbAsset, const int &nHeight, const uint256& blockhash);
    bool WriteMintIndex(const CTransaction& tx, const uint256& txHash, const CMintSyscoin& mintSyscoin, const int &nHeight, const uint256& blockhash);
	bool ScanAssetAllocations(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes);
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
	bool WriteAssetAllocationMempoolToRemoveVector(const std::vector<std::pair<uint256, uint32_t> >  &valueMap) {
        return Write(std::string("assetallocationtxmempool"), valueMap, true);
    }
    bool ReadAssetAllocationMempoolToRemoveVector(std::vector<std::pair<uint256, uint32_t> >  &valueMap) {
        return Read(std::string("assetallocationtxmempool"), valueMap);
    } 	  
    bool ScanAssetAllocationMempoolBalances(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes);
};
static COutPoint emptyOutPoint;
bool GetAssetAllocation(const CAssetAllocationTuple& assetAllocationTuple,CAssetAllocation& txPos);
bool BuildAssetAllocationJson(const CAssetAllocation& assetallocation, const CAsset& asset, UniValue& oName);
bool AssetAllocationTxToJSON(const CTransaction &tx, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash, UniValue &entry, CAssetAllocation& assetallocation);
#ifdef ENABLE_WALLET
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry, CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
void GetActorsFromSyscoinTx(const CTransactionRef& txRef, bool bJustSender, bool bGetAddress, ActorSet& actorSet);
void GetActorsFromAssetTx(const CAsset& theAsset, const CAssetAllocation& theAssetAllocation, int nVersion, bool bJustSender, bool bGetAddress, ActorSet& actorSet);
void GetActorsFromAssetAllocationTx(const CAssetAllocation &theAssetAllocation, int nVersion, bool bJustSender, bool bGetAddress, ActorSet& actorSet);
void GetActorsFromMintTx(const CMintSyscoin& theMintSyscoin, bool bJustSender, bool bGetAddress, ActorSet& actorSet);
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry);
bool WriteAssetIndexForAllocation(const CAssetAllocation& assetallocation, const uint256& txid, const UniValue& oName);
bool WriteAssetIndexForAllocation(const CMintSyscoin& mintSyscoin, const uint256& txid, const UniValue& oName);
bool WriteAssetAllocationIndexTXID(const CAssetAllocationTuple& allocationTuple, const uint256& txid);
#endif // SYSCOIN_SERVICES_ASSETALLOCATION_H
