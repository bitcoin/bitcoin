// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSET_H
#define SYSCOIN_SERVICES_ASSET_H
#include <primitives/transaction.h>
#include <pubkey.h>
static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 1024;
static const unsigned int MAX_SYMBOL_SIZE = 12; // up to 9 characters
static const unsigned int MIN_SYMBOL_SIZE = 4;
// this should be set well after mempool expiry (DEFAULT_MEMPOOL_EXPIRY)
// so that miner will expire txs before they hit expiry errors if for some reason they aren't getting mined (geth issue etc)
static const int64_t MAINNET_MAX_MINT_AGE = 302400; // 0.5 week in seconds, should send to network in half a week or less
static const int64_t MAINNET_MAX_VALIDATE_AGE = 1512000; // 2.5 weeks in seconds
static const int64_t MAINNET_MIN_MINT_AGE = 3600; // 1 hr
static const uint32_t DOWNLOAD_ETHEREUM_TX_ROOTS = 150000; // roughly 7k blocks a day * 3 weeks
static const uint32_t MAX_ETHEREUM_TX_ROOTS = DOWNLOAD_ETHEREUM_TX_ROOTS*2.5;
std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromString(const std::string &str);
uint32_t GenerateSyscoinGuid(const COutPoint& outPoint);
std::string stringFromSyscoinTx(const int &nVersion);
std::string assetFromTx(const int &nVersion);
enum {
    ASSET_UPDATE_ADMIN=1, // god mode flag, governs flags field below
    ASSET_UPDATE_DATA=2, // can you update public data field?
    ASSET_UPDATE_CONTRACT=4, // can you update smart contract?
    ASSET_UPDATE_SUPPLY=8, // can you update supply?
    ASSET_UPDATE_WITNESS=16, // can you update witness?
    ASSET_UPDATE_FLAGS=32, // can you update flags? if you would set permanently disable this one and admin flag as well
    ASSET_UPDATE_ALL=63
};

class CAsset: public CAssetAllocation {
public:
    std::vector<unsigned char> vchContract;
    std::vector<unsigned char> vchPrevContract;
    std::string strSymbol;
    std::string strPubData;
    std::string strPrevPubData;
    uint64_t nBalance;
    uint64_t nTotalSupply;
    uint64_t nMaxSupply;
    unsigned char nPrecision;
    unsigned char nUpdateFlags;
    unsigned char nPrevUpdateFlags;
    CKeyID notaryKeyID;
    CKeyID prevNotaryKeyID;
    CAsset() {
        SetNull();
    }
    explicit CAsset(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    inline void ClearAsset() {
        strPubData.clear();
        vchContract.clear();
        voutAssets.clear();
        strPrevPubData.clear();
        vchPrevContract.clear();
        voutAssets.clear();
        strSymbol.clear();
        nPrevUpdateFlags = 0;
        nBalance = 0;
        nTotalSupply = 0;
        nMaxSupply = 0;
        notaryKeyID.SetNull();
        prevNotaryKeyID.SetNull();
    }

    SERIALIZE_METHODS(CAsset, obj) {
        READWRITEAS(CAssetAllocation, obj);
        READWRITE(obj.nPrecision, obj.vchContract, obj.strPubData, obj.strSymbol, obj.nUpdateFlags, obj.notaryKeyID, obj.prevNotaryKeyID, obj.vchPrevContract, obj.strPrevPubData, obj.nPrevUpdateFlags,
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
    // set precision to an invalid amount so isnull will identity this asset as invalid state
    inline void SetNull() { ClearAsset(); nPrecision = 9; }
    inline bool IsNull() const { return (nPrecision == 9); }
    bool UnserializeFromTx(const CTransaction &tx);
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    void SerializeData(std::vector<unsigned char>& vchData);
};
static CAsset emptyAsset;
bool GetAsset(const uint32_t &nAsset,CAsset& txPos);
bool CheckTxInputsAssets(const CTransaction &tx, TxValidationState &state, const uint32_t &nAsset, std::unordered_map<uint32_t, std::pair<bool, CAmount> > &mapAssetIn, const std::unordered_map<uint32_t, std::pair<bool, CAmount> > &mapAssetOut);
#endif // SYSCOIN_SERVICES_ASSET_H
