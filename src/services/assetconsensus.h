// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETCONSENSUS_H
#define SYSCOIN_SERVICES_ASSETCONSENSUS_H
#include <primitives/transaction.h>
#include <dbwrapper.h>
#include <consensus/params.h>
class TxValidationState;
class CCoinsViewCache;
class CTxUndo;
class CNEVMTxRootsDB : public CDBWrapper {
public:
    explicit CNEVMTxRootsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool ReadTxRoots(const uint256& nBlockHash, NEVMTxRoot& txRoot) {
        return Read(nBlockHash, txRoot);
    } 
    bool Clear();
    bool FlushErase(const std::vector<uint256> &vchBlockHashes);
    bool FlushWrite(const NEVMTxRootMap &mapNEVMTxRoots);
};

class CNEVMMintedTxDB : public CDBWrapper {
public:
    explicit CNEVMMintedTxDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool FlushErase(const NEVMMintTxMap &mapMintKeys);
    bool FlushWrite(const NEVMMintTxMap &mapMintKeys);
};

class CAssetDB : public CDBWrapper {
public:
    explicit CAssetDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool EraseAsset(const uint32_t& nBaseAsset) {
        return Erase(nBaseAsset);
    }   
    bool ReadAsset(const uint32_t& nBaseAsset, CAsset& asset) {
        return Read(nBaseAsset, asset);
    } 
    bool ReadAssetNotaryKeyID(const uint32_t& nBaseAsset, std::vector<unsigned char>& keyID) {
        const auto& pair = std::make_pair(nBaseAsset, true);
        return Exists(pair) && Read(pair, keyID);
    }  
    bool Flush(const AssetMap &mapAssets);
};

class CAssetNFTDB : public CDBWrapper {
public:
    explicit CAssetNFTDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool ExistsNFTAsset(const uint64_t& nAsset) {
        return Exists(nAsset);
    }
    bool Flush(const AssetMap &mapAssets);
};

class CAssetOldDB : public CDBWrapper {
public:
    explicit CAssetOldDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool Empty();
};
extern std::unique_ptr<CAssetDB> passetdb;
extern std::unique_ptr<CAssetNFTDB> passetnftdb;
extern std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
extern std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectAssetSend(const CTransaction &tx, const uint256& txHash, const CTxUndo& txundo, AssetMap &mapAssets);
bool DisconnectAssetUpdate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, NEVMMintTxMap &mapMintKeys);
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CTxUndo& txundo, CCoinsViewCache& view, AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys);
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const bool& bSanityCheck, const int& nHeight, const int64_t& nTime, const uint256& blockhash, NEVMMintTxMap &mapMintKeys);
bool CheckAssetInputs(const Consensus::Params& params, const CTransaction &tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap &mapAssets, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut);
bool CheckSyscoinInputs(const CTransaction& tx, const Consensus::Params& params, const uint256& txHash, TxValidationState &tstate, const int &nHeight, const int64_t& nTime, NEVMMintTxMap &mapMintKeys, const bool &bSanityCheck, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut);
bool CheckSyscoinInputs(const bool &ibd, const Consensus::Params& params, const CTransaction& tx,  const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanityCheck, AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut);
bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut);
uint256 GetNotarySigHash(const CTransaction &tx, const CAssetOut &vecOut);
#endif // SYSCOIN_SERVICES_ASSETCONSENSUS_H
