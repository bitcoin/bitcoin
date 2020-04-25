// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSET_H
#define SYSCOIN_SERVICES_ASSET_H
#include <primitives/transaction.h>
static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const int64_t MAINNET_MAX_MINT_AGE = 604800; // 1 week in seconds
static const int64_t TESTNET_MAX_MINT_AGE = 10800; // 3 hours
static const int64_t MAINNET_MIN_MINT_AGE = 3600; // 1 hr
static const int64_t TESTNET_MIN_MINT_AGE = 600; // 10 mins
static const uint32_t MAX_ETHEREUM_TX_ROOTS = 120000;

static const uint32_t DOWNLOAD_ETHEREUM_TX_ROOTS = 50000;
std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromString(const std::string &str);
int32_t GenerateSyscoinGuid(const COutPoint& outPoint);
std::string stringFromSyscoinTx(const int &nVersion);
bool ReserializeAssetCommitment(CMutableTransaction& tx);
std::string assetFromTx(const int &nVersion);
enum {
    ASSET_UPDATE_ADMIN=1, // god mode flag, governs flags field below
    ASSET_UPDATE_DATA=2, // can you update public data field?
    ASSET_UPDATE_CONTRACT=4, // can you update smart contract?
    ASSET_UPDATE_SUPPLY=8, // can you update supply?
    ASSET_UPDATE_FLAGS=16, // can you update flags? if you would set permanently disable this one and admin flag as well
    ASSET_UPDATE_ALL=31
};

class CAsset: public CAssetAllocation {
public:
    std::vector<unsigned char> vchContract;
    std::string strSymbol;
    std::vector<unsigned char> vchPubData;
    CAmount nBalance;
    CAmount nTotalSupply;
    CAmount nMaxSupply;
    unsigned char nPrecision;
    unsigned char nUpdateFlags;
    CAsset() {
        SetNull();
    }
    explicit CAsset(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    inline void ClearAsset() {
        vchPubData.clear();
        vchContract.clear();
        voutAssets.clear();
    }

    SERIALIZE_METHODS(CAsset, obj) {
        READWRITEAS(CAssetAllocation, obj);
        READWRITE(obj.vchPubData,
        obj.strSymbol, obj.nUpdateFlags, obj.nPrecision, obj.vchContract,
        Using<AmountCompression>(obj.nBalance), Using<AmountCompression>(obj.nTotalSupply), Using<AmountCompression>(obj.nMaxSupply));
    }

    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
        a.voutAssets == b.voutAssets
        );
    }


    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
    inline void SetNull() { ClearAsset(); nPrecision = 8; nMaxSupply = -1; nTotalSupply = -1; nBalance = -1; }
    inline bool IsNull() const { return (nBalance == -1 && nTotalSupply == -1 && nMaxSupply == -1); }
    bool UnserializeFromTx(const CTransaction &tx);
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    void SerializeData(std::vector<unsigned char>& vchData);
};

static CAsset emptyAsset;
bool GetAsset(const int32_t &nAsset,CAsset& txPos);
#endif // SYSCOIN_SERVICES_ASSET_H
