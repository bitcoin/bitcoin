// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETALLOCATION_H
#define SYSCOIN_SERVICES_ASSETALLOCATION_H

#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <unordered_map>
#include <unordered_set>
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

typedef std::vector<std::pair<uint32_t, std::map<uint8_t, CAmount > > > OutToAssetType;
typedef std::vector<uint8_t, CAssetCoinInfo > OutToAssetCoinInfo;
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
	OutToAssetType listReceivingAssets;
	OutToAssetCoinInfo vecAssetInfo;
	template <typename Stream, typename Operation>
	void SerializationOp(Stream& s, Operation ser_action);
	CAssetAllocation() {
		SetNull();
	}
	explicit CAssetAllocation(const CTransaction &tx) {
		SetNull();
		UnserializeFromTx(tx);
	}
	inline void ClearAssetAllocation()
	{
		listReceivingAssets.clear();
		vecAssetInfo.clear();
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
	inline void SetNull() { ClearAssetAllocation();}
    inline bool IsNull() { return listReceivingAssets.empty();}
	bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData);
	void Serialize(std::vector<unsigned char>& vchData);
};
class CMintSyscoin {
public:
    CAssetAllocation assetAllocation;
    std::vector<unsigned char> vchTxValue;
    std::vector<unsigned char> vchTxParentNodes;
    std::vector<unsigned char> vchTxRoot;
    std::vector<unsigned char> vchTxPath;
    std::vector<unsigned char> vchReceiptValue;
    std::vector<unsigned char> vchReceiptParentNodes;
    std::vector<unsigned char> vchReceiptRoot;
    std::vector<unsigned char> vchReceiptPath;   
    uint32_t nBlockNumber;

    CMintSyscoin() {
        SetNull();
    }
    explicit CMintSyscoin(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(assetAllocation);
        READWRITE(vchTxValue);
        READWRITE(vchTxParentNodes);
        READWRITE(vchTxRoot);
        READWRITE(vchTxPath);   
        READWRITE(vchReceiptValue);
        READWRITE(vchReceiptParentNodes);
        READWRITE(vchReceiptRoot);
        READWRITE(vchReceiptPath);
        READWRITE(nBlockNumber);     
    }
    inline void SetNull() { assetAllocation.SetNull(); vchTxRoot.clear(); vchTxValue.clear(); vchTxParentNodes.clear(); vchTxPath.clear(); vchReceiptRoot.clear(); vchReceiptValue.clear(); vchReceiptParentNodes.clear(); vchReceiptPath.clear(); nBlockNumber = 0;  }
    inline bool IsNull() const { return (vchTxValue.empty() && vchReceiptValue.empty()); }
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    bool UnserializeFromTx(const CTransaction &tx);
    void Serialize(std::vector<unsigned char>& vchData);
};
class CBurnSyscoin {
public:
    CAssetAllocation assetAllocation;
    std::vector<unsigned char> vchEthAddress;
    unsigned char nPrecision;
    CBurnSyscoin() {
        SetNull();
    }
    explicit CBurnSyscoin(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(assetAllocation);
        READWRITE(vchEthAddress);
        READWRITE(nPrecision);  
    }
    inline void SetNull() { assetAllocation.SetNull(); vchEthAddress.clear(); nPrecision = 0;  }
    inline bool IsNull() const { return (vchEthAddress.empty() && assetAllocation.IsNull()); }
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    bool UnserializeFromTx(const CTransaction &tx);
    void Serialize(std::vector<unsigned char>& vchData);
};
bool BuildAssetAllocationJson(const CAssetCoinInfo& assetallocation, const CAsset& asset, UniValue& oName);
bool AssetAllocationTxToJSON(const CTransaction &tx, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash, UniValue &entry, CAssetAllocation& assetallocation);
#ifdef ENABLE_WALLET
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry, const CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry);
#endif // SYSCOIN_SERVICES_ASSETALLOCATION_H
