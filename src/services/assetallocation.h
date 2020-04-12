// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETALLOCATION_H
#define SYSCOIN_SERVICES_ASSETALLOCATION_H

#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <compressor.h>
class CTransaction;
class CAsset;
class CMintSyscoin;
bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, UniValue &entry);

std::string assetAllocationFromTx(const int &nVersion);
static const int ONE_YEAR_IN_BLOCKS = 525600;
static const int ONE_HOUR_IN_BLOCKS = 60;
static const int ONE_MONTH_IN_BLOCKS = 43800;
enum {
	ZDAG_NOT_FOUND = -1,
	ZDAG_STATUS_OK = 0,
	ZDAG_WARNING_RBF,
	ZDAG_MAJOR_CONFLICT
};
class CAmountCompressed {
public:
    CAmount nValue;

    SERIALIZE_METHODS(CAmountCompressed, obj)
    {
        READWRITE(Using<AmountCompression>(obj.nValue));
    }

	CAmountCompressed() {
		nAmount = 0;
	}
    CAmountCompressed(const CAmount& nAmountIn): nAmount(nAmountIn) {}

    inline friend CAmount operator()() {
        return nValue;
    }
};
class CAssetAllocation {
public:
    uint32_t nAsset;
	std::vector<CAmountCompressed> voutAssets;

    SERIALIZE_METHODS(CAssetAllocation, obj)
    {
        READWRITE(obj.nAsset);
        READWRITE(obj.voutAssets);
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
		voutAssets.clear();
	}

	inline friend bool operator==(const CAssetAllocation &a, const CAssetAllocation &b) {
		return (a.nAsset == b.nAsset && a.voutAssets == b.voutAssets
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
    inline bool IsNull() { return voutAssets.empty();}
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
    SERIALIZE_METHODS(CMintSyscoin, obj)
    {
        READWRITE(obj.assetAllocation);
        READWRITE(obj.vchTxValue);
        READWRITE(obj.vchTxParentNodes);
        READWRITE(obj.vchTxRoot);
        READWRITE(obj.vchTxPath);   
        READWRITE(obj.vchReceiptValue);
        READWRITE(obj.vchReceiptParentNodes);
        READWRITE(obj.vchReceiptRoot);
        READWRITE(obj.vchReceiptPath);
        READWRITE(obj.nBlockNumber); 
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
    CBurnSyscoin() {
        SetNull();
    }
    explicit CBurnSyscoin(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    SERIALIZE_METHODS(CBurnSyscoin, obj) {
        READWRITE(obj.assetAllocation);
        READWRITE(obj.vchEthAddress);     
    }

    inline void SetNull() { assetAllocation.SetNull(); vchEthAddress.clear();  }
    inline bool IsNull() const { return (vchEthAddress.empty() && assetAllocation.IsNull()); }
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    bool UnserializeFromTx(const CTransaction &tx);
    void Serialize(std::vector<unsigned char>& vchData);
};
bool AssetAllocationTxToJSON(const CTransaction &tx, UniValue &entry);
#endif // SYSCOIN_SERVICES_ASSETALLOCATION_H
