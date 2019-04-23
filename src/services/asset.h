// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSET_H
#define ASSET_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/standard.h"
#include "serialize.h"
#include "primitives/transaction.h"
#include "services/assetallocation.h"
#include <sys/types.h>
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CBlock;
struct CRecipient;
class COutPoint;
class UniValue;
class CTxOut;
class CWallet;

const int SYSCOIN_TX_VERSION_MINT = 0x7400;
const int SYSCOIN_TX_VERSION_BURN = 0x7401;
const int SYSCOIN_TX_VERSION_ASSET_ACTIVATE = 0x7402;
const int SYSCOIN_TX_VERSION_ASSET_UPDATE = 0x7403;
const int SYSCOIN_TX_VERSION_ASSET_TRANSFER = 0x7404;
const int SYSCOIN_TX_VERSION_ASSET_SEND = 0x7405;
const int SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT = 0x7406;
const int SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN = 0x7407;
const int SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND = 0x7408;

    
static const unsigned int MAX_GUID_LENGTH = 20;
static const unsigned int MAX_VALUE_LENGTH = 512;
static const uint64_t ONE_YEAR_IN_SECONDS = 31536000;
static const uint32_t MAX_ETHEREUM_TX_ROOTS = 40000;
static const uint32_t ETHEREUM_CONFIRMS_REQUIRED = 240;
static CCriticalSection cs_ethsyncheight;
std::string stringFromVch(const std::vector<unsigned char> &vch);
std::vector<unsigned char> vchFromValue(const UniValue& value);
std::vector<unsigned char> vchFromString(const std::string &str);
std::string stringFromValue(const UniValue& value);
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient);
unsigned int addressunspent(const std::string& strAddressFrom, COutPoint& outpoint);
int GetSyscoinDataOutput(const CTransaction& tx);
bool GetSyscoinData(const CTransaction &tx, std::vector<unsigned char> &vchData, int& nOut);
bool GetSyscoinData(const CScript &scriptPubKey, std::vector<unsigned char> &vchData);
bool GetSyscoinBurnData(const CScript &scriptPubKey, std::vector<std::vector<unsigned char> > &vchData);
bool GetSyscoinBurnData(const CTransaction &tx, CAssetAllocation* theAssetAllocation);
bool GetSyscoinBurnData(const CTransaction &tx, uint32_t& nAssetFromScript, CWitnessAddress& burnWitnessAddress, CAmount &nAmountFromScript, std::vector<unsigned char> &vchEthAddress);
bool SysTxToJSON(const CTransaction &tx, UniValue &entry);
bool SysBurnTxToJSON(const CTransaction &tx, UniValue &entry);
bool IsOutpointMature(const COutPoint& outpoint);
UniValue syscointxfund_helper(const std::string& strAddress, const int &nVersion, const std::string &vchWitness, std::vector<CRecipient> &vecSend);
bool FlushSyscoinDBs();
void LockMasternodesInDefaultWallet();
bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress& witnessAddressToMatch);
bool IsAssetAllocationTx(const int &nVersion);
bool IsSyscoinTx(const int &nVersion);
bool IsAssetTx(const int &nVersion);
bool IsSyscoinMintTx(const int &nVersion);
CWallet* GetDefaultWallet();

int GenerateSyscoinGuid();


bool AssetTxToJSON(const CTransaction& tx, UniValue &entry);
bool AssetTxToJSON(const CTransaction& tx, const CAsset& dbAsset, const int& nHeight, UniValue &entry);
std::string assetFromTx(const int &nVersion);
/** Upper bound for mantissa.
* 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
* Larger integers cannot consist of arbitrary combinations of 0-9:
*
*   999999999999999999  10^18-1
*  1000000000000000000  10^18		(would overflow)
*  9223372036854775807  (1<<63)-1   (max int64_t)
*  9999999999999999999  10^19-1     (would overflow)
*/
static const CAmount MAX_ASSET = 1000000000000000000LL - 1LL;
inline bool AssetRange(const CAmount& nValue) { return (nValue > 0 && nValue <= MAX_ASSET); }
enum {
    ASSET_UPDATE_ADMIN=1, // god mode flag, governs flags field below
    ASSET_UPDATE_DATA=2, // can you update pubic data field?
    ASSET_UPDATE_CONTRACT=4, // can you update smart contract/burn method signature fields? If you modify this, any subsequent sysx mints will need to wait atleast 10 blocks
    ASSET_UPDATE_SUPPLY=8, // can you update supply?
    ASSET_UPDATE_FLAGS=16, // can you update flags? if you would set permanently disable this one and admin flag as well
    ASSET_UPDATE_ALL=31
};

class CAsset {
public:
	uint32_t nAsset;
	CWitnessAddress witnessAddress;
	std::vector<unsigned char> vchContract;
    uint256 txHash;
    unsigned int nHeight;
	std::vector<unsigned char> vchPubData;
	CAmount nBalance;
	CAmount nTotalSupply;
	CAmount nMaxSupply;
	unsigned char nPrecision;
	unsigned char nUpdateFlags;
    CAsset() {
        SetNull();
        nAsset = 0;
    }
    CAsset(const CAsset&) = delete;
    CAsset(CAsset && other) = default;
    CAsset& operator=(CAsset& other) = delete;
    CAsset& operator=(CAsset&& other) = default;
    CAsset(const CTransaction &tx) {
        SetNull();
        nAsset = 0;
        UnserializeFromTx(tx);
    }
	inline void ClearAsset()
	{
		vchPubData.clear();
		vchContract.clear();
        txHash.SetNull();

	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {		
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(nAsset);
		READWRITE(witnessAddress);
		READWRITE(nBalance);
		READWRITE(nTotalSupply);
		READWRITE(nMaxSupply);
        READWRITE(nHeight);
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
	inline void SetNull() { ClearAsset(); nMaxSupply = 0; nTotalSupply = 0; nBalance = 0; }
    inline bool IsNull() const { return (nBalance == 0 && nTotalSupply == 0 && nMaxSupply == 0); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData);
	void Serialize(std::vector<unsigned char>& vchData);
};
class CMintSyscoin {
public:
    CAssetAllocationTuple assetAllocationTuple;
    std::vector<unsigned char> vchValue;
    std::vector<unsigned char> vchParentNodes;
    std::vector<unsigned char> vchTxRoot;
    uint32_t nBlockNumber;
    std::vector<unsigned char> vchPath;
    CAmount nValueAsset;
    CMintSyscoin() {
        SetNull();
    }
    CMintSyscoin(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {      
        READWRITE(vchValue);
        READWRITE(vchParentNodes);
        READWRITE(nBlockNumber);
        READWRITE(vchTxRoot);
        READWRITE(vchPath);   
        READWRITE(assetAllocationTuple);  
        READWRITE(nValueAsset);  
    }
    inline void SetNull() { nValueAsset = 0; assetAllocationTuple.SetNull(); vchTxRoot.clear(); vchValue.clear(); vchParentNodes.clear(); nBlockNumber = 0; vchPath.clear(); }
    inline bool IsNull() const { return (vchValue.empty()); }
    bool UnserializeFromData(const std::vector<unsigned char> &vchData);
    bool UnserializeFromTx(const CTransaction &tx);
    void Serialize(std::vector<unsigned char>& vchData);
};
typedef std::unordered_map<uint32_t, std::vector<unsigned char> > EthereumTxRootMap;

class CEthereumTxRootsDB : public CDBWrapper {
public:
    CEthereumTxRootsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "ethereumtxroots", nCacheSize, fMemory, fWipe) {
       Init();
    } 
    bool ReadTxRoot(const uint32_t& nHeight, std::vector<unsigned char>& vchTxRoot) {
        return Read(nHeight, vchTxRoot);
    } 
    bool ReadCurrentHeight(uint32_t &nCurrentHeight){
        return Read("currentheight", nCurrentHeight);
    }
    bool WriteCurrentHeight(const uint32_t &nCurrentHeight){
        return Write("currentheight", nCurrentHeight);
    }
    bool ReadHighestHeight(uint32_t &nHighestHeight){
         return Read("highestheight", nHighestHeight);
    }
    bool WriteHighestHeight(const uint32_t &nHighestHeight){
        return Write("highestheight", nHighestHeight);
    }
    bool Init();
    bool PruneTxRoots();
    bool FlushErase(const std::vector<uint32_t> &vecHeightKeys);
    bool FlushWrite(const EthereumTxRootMap &mapTxRoots);
};
typedef std::unordered_map<int, CAsset > AssetMap;
class CAssetDB : public CDBWrapper {
public:
    CAssetDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {}
    bool EraseAsset(const uint32_t& nAsset) {
        return Erase(nAsset);
    }   
    bool ReadAsset(const uint32_t& nAsset, CAsset& asset) {
        return Read(nAsset, asset);
    } 
	void WriteAssetIndex(const CTransaction& tx, const CAsset& dbAsset, const int& nHeight);
	bool ScanAssets(const int count, const int from, const UniValue& oOptions, UniValue& oRes);
    bool Flush(const AssetMap &mapAssets);
};
static std::string assetindexpage = std::string("assetindexpage");
static std::string assetallocationindexpage = std::string("assetallocationindexpage");
class CAssetIndexDB : public CDBWrapper {
public:
    CAssetIndexDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assetindex", nCacheSize, fMemory, fWipe) {
    }
    bool ReadIndexTXIDs(const CAssetAllocationTuple& allocationTuple, const int64_t &page, std::vector<uint256> &TXIDS) {
        return Read(std::make_pair(allocationTuple.ToString(), page), TXIDS);
    }
    bool WriteIndexTXIDs(const CAssetAllocationTuple& allocationTuple, const int64_t &page, const std::vector<uint256> &TXIDS) {
        return Write(std::make_pair(allocationTuple.ToString(), page), TXIDS);
    }
    bool ReadIndexTXIDs(const uint32_t &assetGuid, const int64_t &page, std::vector<uint256> &TXIDS){
        return Read(std::make_pair(assetGuid, page), TXIDS);
    }
    bool WriteIndexTXIDs(const uint32_t &assetGuid, const int64_t &page, const std::vector<uint256> &TXIDS) {
        return Write(std::make_pair(assetGuid, page), TXIDS);
    }
   bool EraseIndexTXID(const CAssetAllocationTuple& allocationTuple, const uint256 &txid) {
        int64_t page=0;
        std::vector<uint256> TXIDS;
        if(!ReadAssetAllocationPage(page))
            return false;
        int64_t walkBackPage = page;
        while(walkBackPage >= 0){
            if(!ReadIndexTXIDs(allocationTuple, walkBackPage, TXIDS)){
                walkBackPage--;
                continue;
            }
            if(TXIDS.empty()){
                Erase(std::make_pair(allocationTuple.ToString(), walkBackPage));
                walkBackPage--;
                continue;
            }
            std::vector<uint256>::iterator it = std::find(TXIDS.begin(), TXIDS.end(), txid);
            if(it == TXIDS.end()){
                walkBackPage--;
                continue;
            }
            TXIDS.erase(it);
            break;
        }
        if(walkBackPage < 0)
            return false;
        if(TXIDS.empty() && walkBackPage == page){
            bool res = Erase(std::make_pair(allocationTuple.ToString(), page));
            if(page <= 0)
                return res;
            page--;
            return res && WriteAssetAllocationPage(page);
        }
        return Write(std::make_pair(allocationTuple.ToString(), page), TXIDS);
    }    
    bool EraseIndexTXID(const uint32_t &assetGuid, const uint256 &txid) {
        int64_t page=0;
        std::vector<uint256> TXIDS;
        if(!ReadAssetPage(page))
            return false;
        int64_t walkBackPage = page;
        while(walkBackPage >= 0){
            if(!ReadIndexTXIDs(assetGuid, walkBackPage, TXIDS)){
                walkBackPage--;
                continue;
            }
            if(TXIDS.empty()){
                Erase(std::make_pair(assetGuid, walkBackPage));
                walkBackPage--;
                continue;
            }            
            std::vector<uint256>::iterator it = std::find(TXIDS.begin(), TXIDS.end(), txid);
            if(it == TXIDS.end()){
                walkBackPage--;
                continue;
            }
            TXIDS.erase(it);
            break;
        }
        if(walkBackPage < 0)
            return false;
        if(TXIDS.empty() && walkBackPage == page){
            bool res = Erase(std::make_pair(assetGuid, page));
            if(page <= 0)
                return res;
            page--;
            return res && WriteAssetPage(page);
        }
        return Write(std::make_pair(assetGuid, page), TXIDS);
    }
    bool ReadAssetsByAddress(const CWitnessAddress &address, std::vector<uint32_t> &assetGuids){
        return Read(address, assetGuids);
    }
    bool WriteAssetsByAddress(const CWitnessAddress &address, const std::vector<uint32_t> &assetGuids) {
        return Write(address, assetGuids);
    }      
    bool ReadAssetPage(int64_t& page) {
        return Read(assetindexpage, page);
    }
    bool WriteAssetPage(const int64_t &page) {
        return Write(assetindexpage, page);
    }
    bool ReadAssetAllocationPage(int64_t& page) {
        return Read(assetallocationindexpage, page);
    }
    bool WriteAssetAllocationPage(const int64_t &page) {
        return Write(assetallocationindexpage, page);
    }
    bool WritePayload(const uint256& txid, const UniValue& payload) {
        return Write(txid, payload.write());
    }
    bool ReadPayload(const uint256& txid, UniValue& payload) {
        std::string strPayload;
        bool res = Read(txid, strPayload);
        return res && payload.read(strPayload);
    }        
    bool ScanAssetIndex(int64_t page, const UniValue& oOptions, UniValue& oRes);
    bool FlushErase(const std::vector<uint256> &vecTXIDs);
};
static CAsset emptyAsset;
bool GetAsset(const int &nAsset,CAsset& txPos);
bool BuildAssetJson(const CAsset& asset, UniValue& oName);
UniValue ValueFromAssetAmount(const CAmount& amount, int precision);
CAmount AssetAmountFromValue(UniValue& value, int precision);
CAmount AssetAmountFromValueNonNeg(const UniValue& value, int precision);
bool AssetRange(const CAmount& amountIn, int precision);
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output);
void WriteAssetIndexTXID(const uint32_t& nAsset, const uint256& txid);
extern std::unique_ptr<CAssetDB> passetdb;
extern std::unique_ptr<CAssetAllocationDB> passetallocationdb;
extern std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;
extern std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
extern std::unique_ptr<CAssetIndexDB> passetindexdb;
#endif // ASSET_H
