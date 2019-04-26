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
extern std::unique_ptr<CBlockIndexDB> pblockindexdb;
bool DisconnectSyscoinTransaction(const CTransaction& tx, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations, std::vector<uint256> & vecTXIDs);
bool DisconnectAssetActivate(const CTransaction &tx, AssetMap &mapAssets);
bool DisconnectAssetSend(const CTransaction &tx, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations);
bool DisconnectAssetUpdate(const CTransaction &tx, AssetMap &mapAssets);
bool DisconnectAssetAllocation(const CTransaction &tx, AssetAllocationMap &mapAssetAllocations);
bool DisconnectMintAsset(const CTransaction &tx, AssetAllocationMap &mapAssetAllocations);
bool CheckSyscoinMint(const bool ibd, const CTransaction& tx, std::string& errorMessage, const bool &fJustCheck, const bool& bSanity, const bool& bMiner, const int& nHeight, const uint256& blockhash, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations);
bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const uint256& blockhash, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations, std::string &errorMessage, const bool &bSanityCheck=false, const bool &bMiner=false);
static std::vector<uint256> DEFAULT_VECTOR;
bool CheckSyscoinInputs(const bool ibd, const CTransaction& tx, CValidationState &state, const CCoinsViewCache &inputs, bool fJustCheck, bool &bOverflow, int nHeight, const CBlock& block, const bool &bSanity = false, const bool &bMiner = false, std::vector<uint256>& txsToRemove=DEFAULT_VECTOR);
static CAssetAllocation emptyAllocation;
bool ResetAssetAllocation(const std::string &senderStr, const uint256 &txHash, const bool &bMiner=false, const bool &bExpiryOnly=false);
void ResyncAssetAllocationStates();
bool CheckAssetAllocationInputs(const CTransaction &tx, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, const uint256& blockhash, AssetAllocationMap &mapAssetAllocations, std::string &errorMessage, bool& bOverflow, const bool &bSanityCheck = false, const bool &bMiner = false);
#endif // SYSCOIN_SERVICES_ASSETCONSENSUS_H
