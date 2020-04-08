// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSET_H
#define SYSCOIN_SERVICES_ASSET_H


#include <dbwrapper.h>
#include <script/standard.h>
#include <serialize.h>
#include <primitives/transaction.h>
#include <services/assetallocation.h>
#include <sys/types.h>
#include <univalue.h>
#ifdef ENABLE_WALLET
#include <wallet/ismine.h>
#endif
class CTransaction;
class CCoinsViewCache;
class COutPoint;
#ifdef ENABLE_WALLET
class CWallet;
#endif

const int SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN = 128;
const int SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION = 129;
const int SYSCOIN_TX_VERSION_ASSET_ACTIVATE = 130;
const int SYSCOIN_TX_VERSION_ASSET_UPDATE = 131;
const int SYSCOIN_TX_VERSION_ASSET_SEND = 132;
const int SYSCOIN_TX_VERSION_ALLOCATION_MINT = 133;
const int SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM = 134;
const int SYSCOIN_TX_VERSION_ALLOCATION_SEND = 135;

static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;
static const uint32_t MAX_ETHEREUM_TX_ROOTS = 120000;
static const uint32_t DOWNLOAD_ETHEREUM_TX_ROOTS = 50000;
std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
int GetSyscoinDataOutput(const CTransaction& tx);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, int& nOut);
bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData);
#ifdef ENABLE_WALLET
bool SysTxToJSON(const CTransaction &tx, UniValue &entry, const CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
bool SysTxToJSON(const CTransaction &tx, UniValue &entry);
bool IsOutpointMature(const COutPoint& outpoint);
bool FlushSyscoinDBs();
COutPoint FindAssetOwnerOutPoint(const CCoinsViewCache &inputs, const CTransaction& tx);
bool IsAssetAllocationTx(const int &nVersion);
bool IsZdagTx(const int &nVersion);
bool IsSyscoinTx(const int &nVersion);
bool IsSyscoinWithInputTx(const int &nVersion);
bool IsAssetTx(const int &nVersion);
bool IsSyscoinMintTx(const int &nVersion);
CAmount getaddressbalance(const std::string& strAddress);
int32_t GenerateSyscoinGuid(const COutPoint& outPoint);


bool AssetTxToJSON(const CTransaction& tx, UniValue &entry);
bool AssetTxToJSON(const CTransaction& tx, const int& nHeight, const uint256& blockhash, UniValue &entry);
std::string assetFromTx(const int &nVersion);
enum {
    ASSET_UPDATE_ADMIN=1, // god mode flag, governs flags field below
    ASSET_UPDATE_DATA=2, // can you update public data field?
    ASSET_UPDATE_CONTRACT=4, // can you update smart contract?
    ASSET_UPDATE_SUPPLY=8, // can you update supply?
    ASSET_UPDATE_FLAGS=16, // can you update flags? if you would set permanently disable this one and admin flag as well
    ASSET_UPDATE_ALL=31
};

class CAsset {
public:
    CAssetAllocation assetAllocation;
    std::vector<unsigned char> vchContract;
    std::string strSymbol;
    std::vector<unsigned char> vchPubData;
    CAmount nBalance;
    CAmount nTotalSupply;
    CAmount nMaxSupply;
    CAmount nBurnBalance;
    unsigned char nPrecision;
    unsigned char nUpdateFlags;
    CAsset() {
        SetNull();
        nAsset = 0;
    }
    explicit CAsset(const CTransaction &tx) {
        SetNull();
        nAsset = 0;
        UnserializeFromTx(tx);
    }
    inline void ClearAsset()
    {
        vchPubData.clear();
        vchContract.clear();
        outpoint.SetNull();

    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {		
        READWRITE(vchPubData);
        READWRITE(outpoint);
        READWRITE(nAsset);
        READWRITE(strSymbol);
        READWRITE(nBalance);
        READWRITE(nTotalSupply);
        READWRITE(nMaxSupply);
        READWRITE(nBurnBalance);
        READWRITE(nUpdateFlags);
        READWRITE(nPrecision);
        READWRITE(vchContract);
    }
    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
        a.nAsset == b.nAsset
        );
    }


    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
    inline void SetNull() { ClearAsset(); nPrecision = 8; nBurnBalance = 0; nMaxSupply = -1; nTotalSupply = -1; nBalance = -1; }
    inline bool IsNull() const { return (nBalance == -1 && nTotalSupply == -1 && nMaxSupply == -1); }
    bool UnserializeFromTx(const CTransaction &tx);
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    void Serialize(std::vector<unsigned char>& vchData);
};
typedef std::unordered_map<uint32_t, CAsset > AssetMap;
class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}
    bool EraseAsset(const uint32_t& nAsset) {
        return Erase(nAsset);
    }   
    bool ReadAsset(const uint32_t& nAsset, CAsset& asset) {
        return Read(nAsset, asset);
    }  	
	bool ScanAssets(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes);
    bool Flush(const AssetMap &mapAssets);
};

static CAsset emptyAsset;
bool GetAsset(const uint32_t &nAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, UniValue& oName);
#ifdef ENABLE_WALLET
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output, const CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output);
extern std::unique_ptr<CAssetDB> passetdb;
#endif // SYSCOIN_SERVICES_ASSET_H
