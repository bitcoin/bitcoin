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

const int SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN = 0x7400;
const int SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION = 0x7401;
const int SYSCOIN_TX_VERSION_ASSET_ACTIVATE = 0x7402;
const int SYSCOIN_TX_VERSION_ASSET_UPDATE = 0x7403;
const int SYSCOIN_TX_VERSION_ASSET_TRANSFER = 0x7404;
const int SYSCOIN_TX_VERSION_ASSET_SEND = 0x7405;
const int SYSCOIN_TX_VERSION_ALLOCATION_MINT = 0x7406;
const int SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM = 0x7407;
const int SYSCOIN_TX_VERSION_ALLOCATION_SEND = 0x7408;
const int SYSCOIN_TX_VERSION_ALLOCATION_LOCK = 0x7409;

static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;
static const uint32_t MAX_ETHEREUM_TX_ROOTS = 120000;
static const uint32_t DOWNLOAD_ETHEREUM_TX_ROOTS = 50000;
static COutPoint emptyPoint;
std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
int GetSyscoinDataOutput(const CTransaction& tx);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, int& nOut);
bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData);
bool GetSyscoinBurnData(const CScript &scriptPubKey, std::vector<std::vector<unsigned char> > &vchData);
bool GetSyscoinBurnData(const CTransaction &tx, CAssetAllocation* theAssetAllocation, std::vector<unsigned char> &vchEthAddress, std::vector<unsigned char> &vchEthContract);
bool GetSyscoinBurnData(const CTransaction &tx, uint32_t& nAssetFromScript, CWitnessAddress& burnWitnessAddress, CAmount &nAmountFromScript, std::vector<unsigned char> &vchEthAddress, uint8_t &nPrecision, std::vector<unsigned char> &vchEthContract);
#ifdef ENABLE_WALLET
bool SysTxToJSON(const CTransaction &tx, UniValue &entry, CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
bool SysTxToJSON(const CTransaction &tx, UniValue &entry);
bool IsOutpointMature(const COutPoint& outpoint);
bool FlushSyscoinDBs();
bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress& witnessAddressToMatch);
bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress& witnessAddressToMatch, const COutPoint& lockedOutpoint);
bool IsAssetAllocationTx(const int &nVersion);
bool IsSyscoinTx(const int &nVersion);
bool IsAssetTx(const int &nVersion);
bool IsSyscoinMintTx(const int &nVersion);
CAmount getaddressbalance(const std::string& strAddress);
int GenerateSyscoinGuid();


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
    uint32_t nAsset;
    CWitnessAddress witnessAddress;
    CWitnessAddress witnessAddressTransfer;
    std::vector<unsigned char> vchContract;
    std::string strSymbol;
    uint256 txHash;
    unsigned int nHeight;
    std::vector<unsigned char> vchPubData;
    CAmount nBalance;
    CAmount nTotalSupply;
    CAmount nMaxSupply;
    unsigned char nPrecision;
    unsigned char nUpdateFlags;
    char nDumurrageOrInterest;
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
        txHash.SetNull();
        witnessAddressTransfer.SetNull();

    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {		
        READWRITE(vchPubData);
        READWRITE(txHash);
        READWRITE(nAsset);
        READWRITE(strSymbol);
        READWRITE(witnessAddress);
        READWRITE(witnessAddressTransfer);
        READWRITE(nBalance);
        READWRITE(nTotalSupply);
        READWRITE(nMaxSupply);
        READWRITE(nHeight);
        READWRITE(nUpdateFlags);
        READWRITE(nPrecision);
        READWRITE(vchContract);   
        READWRITE(nDumurrageOrInterest); 
    }
    inline friend bool operator==(const CAsset &a, const CAsset &b) {
        return (
        a.nAsset == b.nAsset
        );
    }


    inline friend bool operator!=(const CAsset &a, const CAsset &b) {
        return !(a == b);
    }
    inline void SetNull() { ClearAsset(); nPrecision = 8; nMaxSupply = -1; nTotalSupply = -1; nBalance = -1; }
    inline bool IsNull() const { return (nBalance == -1 && nTotalSupply == -1 && nMaxSupply == -1); }
    bool UnserializeFromTx(const CTransaction &tx);
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    void Serialize(std::vector<unsigned char>& vchData);
};
class CMintSyscoin {
public:
    CAssetAllocationTuple assetAllocationTuple;
    std::vector<unsigned char> vchTxValue;
    std::vector<unsigned char> vchTxParentNodes;
    std::vector<unsigned char> vchTxRoot;
    std::vector<unsigned char> vchTxPath;
    std::vector<unsigned char> vchReceiptValue;
    std::vector<unsigned char> vchReceiptParentNodes;
    std::vector<unsigned char> vchReceiptRoot;
    std::vector<unsigned char> vchReceiptPath;   
    uint32_t nBlockNumber;
    
    CAmount nValueAsset;
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
        READWRITE(vchTxValue);
        READWRITE(vchTxParentNodes);
        READWRITE(vchTxRoot);
        READWRITE(vchTxPath);   
        READWRITE(vchReceiptValue);
        READWRITE(vchReceiptParentNodes);
        READWRITE(vchReceiptRoot);
        READWRITE(vchReceiptPath);        
        READWRITE(nBlockNumber);

        READWRITE(assetAllocationTuple);  
        READWRITE(nValueAsset);  
    }
    inline void SetNull() { nValueAsset = 0; assetAllocationTuple.SetNull(); vchTxRoot.clear(); vchTxValue.clear(); vchTxParentNodes.clear(); vchTxPath.clear(); vchReceiptRoot.clear(); vchReceiptValue.clear(); vchReceiptParentNodes.clear(); vchReceiptPath.clear(); nBlockNumber = 0;  }
    inline bool IsNull() const { return (vchTxValue.empty() && vchReceiptValue.empty()); }
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    bool UnserializeFromTx(const CTransaction &tx);
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
    bool ReadAssetsByAddress(const CWitnessAddress &address, std::vector<uint32_t> &assetGuids){
        return Read(address, assetGuids);
    }
     bool ExistsAssetsByAddress(const CWitnessAddress &address){
        return Exists(address);
    }   
	bool WriteAssetIndex(const CTransaction& tx, const uint256& txid, const CAsset& dbAsset, const int& nHeight, const uint256& blockhash);
	bool ScanAssets(const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes);
    bool Flush(const AssetMap &mapAssets);
};
class CAssetIndexDB : public CDBWrapper {
public:
    CAssetIndexDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetindex", nCacheSize, fMemory, fWipe) {
    }
    bool ReadIndexTXIDs(const CAssetAllocationTuple& allocationTuple, const uint32_t &page, std::vector<uint256> &TXIDS) {
        return Read(std::make_pair(allocationTuple.ToString(), page), TXIDS);
    }
    bool WriteIndexTXIDs(const CAssetAllocationTuple& allocationTuple, const uint32_t &page, const std::vector<uint256> &TXIDS) {
        return Write(std::make_pair(allocationTuple.ToString(), page), TXIDS);
    }
    bool ReadIndexTXIDs(const uint32_t &assetGuid, const uint32_t &page, std::vector<uint256> &TXIDS){
        return Read(std::make_pair(assetGuid, page), TXIDS);
    }
    bool WriteIndexTXIDs(const uint32_t &assetGuid, const uint32_t &page, const std::vector<uint256> &TXIDS) {
        return Write(std::make_pair(assetGuid, page), TXIDS);
    }
   bool EraseIndexTXID(const CAssetAllocationTuple& allocationTuple, const uint256 &txid) {
        uint32_t page=0;
        std::vector<uint256> TXIDS;
        if(!ReadAssetAllocationPage(allocationTuple.nAsset, page))
            return false;
        uint32_t walkBackPage = page;
        const std::string &allocationTupleStr = allocationTuple.ToString();
        while(true){
            if(!ReadIndexTXIDs(allocationTuple, walkBackPage, TXIDS)){
                if(walkBackPage == 0)
                    break;
                walkBackPage--;
                continue;
            }
            if(TXIDS.empty()){
                const std::pair<std::string, uint32_t> &pagePair = std::make_pair(allocationTupleStr, walkBackPage);
                if(Exists(pagePair) && !Erase(pagePair))
                    return false;
                if(walkBackPage == 0)
                    break;
                walkBackPage--;

                continue;
            }
            std::vector<uint256>::iterator it = std::find(TXIDS.begin(), TXIDS.end(), txid);
            if(it == TXIDS.end()){
                if(walkBackPage == 0)
                    break;
                walkBackPage--;
                continue;
            }
            TXIDS.erase(it);
            break;
        }
        if(TXIDS.empty() && walkBackPage == page){
            const std::pair<std::string, uint32_t> &pagePair = std::make_pair(allocationTupleStr, page);
            if(Exists(pagePair) && !Erase(pagePair))
                return false;
            if(page == 0)
                return true;
            page--;
            return WriteAssetAllocationPage(allocationTuple.nAsset, page);
        }
        return Write(std::make_pair(allocationTupleStr, page), TXIDS);
    }    
    bool EraseIndexTXID(const uint32_t &assetGuid, const uint256 &txid) {
        uint32_t page=0;
        std::vector<uint256> TXIDS;
        if(!ReadAssetPage(assetGuid, page))
            return false;
        uint32_t walkBackPage = page;
        while(true){
            if(!ReadIndexTXIDs(assetGuid, walkBackPage, TXIDS)){
                if(walkBackPage == 0)
                    break;
                walkBackPage--;
                continue;
            }
            if(TXIDS.empty()){
                const std::pair<uint32_t, uint32_t> &pagePair = std::make_pair(assetGuid, walkBackPage);
                if(Exists(pagePair) && !Erase(pagePair))
                    return false;
                if(walkBackPage == 0)
                    break;
                walkBackPage--;
                continue;
            }            
            std::vector<uint256>::iterator it = std::find(TXIDS.begin(), TXIDS.end(), txid);
            if(it == TXIDS.end()){
                if(walkBackPage == 0)
                    break;
                walkBackPage--;
                continue;
            }
            TXIDS.erase(it);
            break;
        }
        if(TXIDS.empty() && walkBackPage == page){
            const std::pair<uint32_t, uint32_t> &pagePair = std::make_pair(assetGuid, page);
            if(Exists(pagePair) && !Erase(pagePair))
                return false;
            if(page == 0)
                return true;
            page--;
            return WriteAssetPage(assetGuid, page);
        }
        return Write(std::make_pair(assetGuid, page), TXIDS);
    }    
    bool ReadAssetPage(const uint32_t &assetGuid, uint32_t& page) {
        return Read(std::make_pair(true, assetGuid), page);
    }
    bool WriteAssetPage(const uint32_t &assetGuid, const uint32_t &page) {
        return Write(std::make_pair(true, assetGuid), page);
    }
    bool ReadAssetAllocationPage(const uint32_t &assetGuid, uint32_t& page) {
        return Read(std::make_pair(false, assetGuid), page);
    }
    bool WriteAssetAllocationPage(const uint32_t &assetGuid, const uint32_t &page) {
        return Write(std::make_pair(false, assetGuid), page);
    }
    bool WritePayload(const uint256& txid, const UniValue& payload) {
        return Write(txid, payload.write());
    }
    bool ReadPayload(const uint256& txid, UniValue& payload) {
        std::string strPayload;
        bool res = Read(txid, strPayload);
        return res && payload.read(strPayload);
    }        
    bool ScanAssetIndex(uint32_t page, const UniValue& oOptions, UniValue& oRes);
    bool FlushErase(const std::vector<uint256> &vecTXIDs);
};
static CAsset emptyAsset;
static CWitnessAddress burnWitness(0, vchFromString("burn"));
static std::string burnWitnessStr(burnWitness.ToString());
bool GetAsset(const uint32_t &nAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, UniValue& oName);
#ifdef ENABLE_WALLET
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output, CWallet* const pwallet, const isminefilter* filter_ismine);
#endif
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output);
bool WriteAssetIndexTXID(const uint32_t& nAsset, const uint256& txid);
extern std::unique_ptr<CAssetDB> passetdb;
extern std::unique_ptr<CAssetAllocationDB> passetallocationdb;
extern std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;
extern std::unique_ptr<CAssetIndexDB> passetindexdb;
#endif // SYSCOIN_SERVICES_ASSET_H
