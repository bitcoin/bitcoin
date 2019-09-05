// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETCONSENSUS_H
#define SYSCOIN_SERVICES_ASSETCONSENSUS_H


#include <primitives/transaction.h>
#include <services/asset.h>
class CBlockIndexDB : public CDBWrapper {
public:
    CBlockIndexDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "blockindex", nCacheSize, fMemory, fWipe) {}
    
    bool ReadBlockHash(const uint256& txid, uint256& block_hash){
        return Read(txid, block_hash);
    } 
    bool FlushWrite(const std::vector<std::pair<uint256, uint256> > &blockIndex);
    bool FlushErase(const std::vector<uint256> &vecTXIDs);
};
class CLockedOutpointsDB : public CDBWrapper {
public:
	CLockedOutpointsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "lockedoutpoints", nCacheSize, fMemory, fWipe) {}

	bool ReadOutpoint(const COutPoint& outpoint, bool& locked) {
		return Read(outpoint, locked);
	}
	bool FlushWrite(const std::vector<COutPoint> &lockedOutpoints);
	bool FlushErase(const std::vector<COutPoint> &lockedOutpoints);
};
class EthereumTxRoot {
    public:
    std::vector<unsigned char> vchBlockHash;
    std::vector<unsigned char> vchPrevHash;
    std::vector<unsigned char> vchTxRoot;
    std::vector<unsigned char> vchReceiptRoot;
    int64_t nTimestamp;
    
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {      
        READWRITE(vchBlockHash);
        READWRITE(vchPrevHash);
        READWRITE(vchTxRoot);
        READWRITE(vchReceiptRoot);
        READWRITE(nTimestamp);
    }
};
typedef std::unordered_map<uint32_t, EthereumTxRoot> EthereumTxRootMap;
class CEthereumTxRootsDB : public CDBWrapper {
public:
    CEthereumTxRootsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "ethereumtxroots", nCacheSize, fMemory, fWipe) {
       Init();
    } 
    bool ReadTxRoots(const uint32_t& nHeight, EthereumTxRoot& txRoot) {
        return Read(nHeight, txRoot);
    } 
    void AuditTxRootDB(std::vector<std::pair<uint32_t, uint32_t> > &vecMissingBlockRanges);
    bool Init();
    bool Clear();
    bool PruneTxRoots(const uint32_t &fNewGethSyncHeight);
    bool FlushErase(const std::vector<uint32_t> &vecHeightKeys);
    bool FlushWrite(const EthereumTxRootMap &mapTxRoots);
};
typedef std::vector<std::pair<uint64_t, uint32_t> > EthereumMintTxVec;
class CEthereumMintedTxDB : public CDBWrapper {
public:
    CEthereumMintedTxDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "ethereumminttx", nCacheSize, fMemory, fWipe) {
    } 
    bool ExistsKey(const std::pair<uint64_t, uint32_t> &ethKey) {
        return Exists(ethKey);
    } 
    bool FlushErase(const EthereumMintTxVec &vecMintKeys);
    bool FlushWrite(const EthereumMintTxVec &vecMintKeys);
};
extern std::unique_ptr<CBlockIndexDB> pblockindexdb;
extern std::unique_ptr<CLockedOutpointsDB> plockedoutpointsdb;
extern std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
extern std::unique_ptr<CEthereumMintedTxDB> pethereumtxmintdb;
bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats);
bool DisconnectAssetSend(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations);
bool DisconnectAssetUpdate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectAssetTransfer(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys);
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys, ActorSet &actorSet);
int ResetAssetAllocation(const std::string &senderStr);
int ResetAssetAllocations(const ActorSet &actorSet);
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, CValidationState &state, const bool &fJustCheck, const bool& bSanity, const bool& bMiner, const int& nHeight, const int64_t& nTime, const uint256& blockhash, AssetMap& mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, EthereumMintTxVec &vecMintKeys);
bool CheckAssetInputs(const CTransaction &tx, const uint256& txHash, CValidationState &state,const CCoinsViewCache &inputs, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations, const bool &bSanityCheck=false, const bool &bMiner=false);
bool CheckSyscoinInputs(const CTransaction& tx, const uint256& txHash, CValidationState& state, const CCoinsViewCache &inputs, const bool &fJustCheck, const int &nHeight, const int64_t& nTime,const bool &bSanity);
bool CheckSyscoinInputs(const bool &ibd, const CTransaction& tx, const uint256& txHash, CValidationState &state, const CCoinsViewCache &inputs, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanity, const bool &bMiner, ActorSet &actorSet, AssetAllocationMap &mapAssetAllocations, AssetMap &mapAssets, AssetSupplyStatsMap &mapAssetSupplyStats, EthereumMintTxVec &vecMintKeys, std::vector<COutPoint> &vecLockedOutpoints);
static CAssetAllocationDBEntry emptyAllocation;
void ResyncAssetAllocationStates();
bool CheckSyscoinLockedOutpoints(const CTransactionRef &tx, CValidationState& state);
bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, const CAssetAllocation &theAssetAllocation, CValidationState &state, const CCoinsViewCache &inputs, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetSupplyStatsMap &mapAssetSupplyStats, AssetAllocationMap &mapAssetAllocations, std::vector<COutPoint> &vecLockedOutpoints,  const bool &bSanityCheck = false, const bool &bMiner = false);
bool FormatSyscoinErrorMessage(CValidationState& state, const std::string errorMessage, bool bErrorNotInvalid = true, bool bConsensus = true);
#endif // SYSCOIN_SERVICES_ASSETCONSENSUS_H
