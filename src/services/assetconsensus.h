// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETCONSENSUS_H
#define SYSCOIN_SERVICES_ASSETCONSENSUS_H
#include <primitives/transaction.h>
#include <dbwrapper.h>
class TxValidationState;
class CCoinsViewCache;
class CTxUndo;
class EthereumTxRoot {
    public:
    std::vector<unsigned char> vchBlockHash;
    std::vector<unsigned char> vchPrevHash;
    std::vector<unsigned char> vchTxRoot;
    std::vector<unsigned char> vchReceiptRoot;
    int64_t nTimestamp;
    
    SERIALIZE_METHODS(EthereumTxRoot, obj)
    {
        READWRITE(obj.vchBlockHash, obj.vchPrevHash, obj.vchTxRoot, obj.vchReceiptRoot, obj.nTimestamp);
    }
};
typedef std::unordered_map<uint32_t, EthereumTxRoot> EthereumTxRootMap;
class CEthereumTxRootsDB : public CDBWrapper {
public:
    explicit CEthereumTxRootsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
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

class CEthereumMintedTxDB : public CDBWrapper {
public:
    explicit CEthereumMintedTxDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool FlushErase(const EthereumMintTxMap &mapMintKeys);
    bool FlushWrite(const EthereumMintTxMap &mapMintKeys);
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
extern std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
extern std::unique_ptr<CEthereumMintedTxDB> pethereumtxmintdb;
bool DisconnectAssetActivate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectAssetSend(const CTransaction &tx, const uint256& txHash, const CTxUndo& txundo, AssetMap &mapAssets);
bool DisconnectAssetUpdate(const CTransaction &tx, const uint256& txHash, AssetMap &mapAssets);
bool DisconnectMintAsset(const CTransaction &tx, const uint256& txHash, EthereumMintTxMap &mapMintKeys);
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CTxUndo& txundo, CCoinsViewCache& view, AssetMap &mapAssets, EthereumMintTxMap &mapMintKeys);
bool CheckSyscoinMint(const bool &ibd, const CTransaction& tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const bool& bSanityCheck, const int& nHeight, const int64_t& nTime, const uint256& blockhash, EthereumMintTxMap &mapMintKeys);
bool CheckAssetInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, AssetMap &mapAssets, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut);
bool CheckSyscoinInputs(const CTransaction& tx, const uint256& txHash, TxValidationState &tstate, const int &nHeight, const int64_t& nTime, EthereumMintTxMap &mapMintKeys, const bool &bSanityCheck, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut);
bool CheckSyscoinInputs(const bool &ibd, const CTransaction& tx,  const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const int64_t& nTime, const uint256 & blockHash, const bool &bSanityCheck, AssetMap &mapAssets, EthereumMintTxMap &mapMintKeys, const CAssetsMap& mapAssetIn, const CAssetsMap& mapAssetOut);
bool CheckAssetAllocationInputs(const CTransaction &tx, const uint256& txHash, TxValidationState &tstate, const bool &fJustCheck, const int &nHeight, const uint256& blockhash, const bool &bSanityCheck, const CAssetsMap &mapAssetIn, const CAssetsMap &mapAssetOut);
uint256 GetNotarySigHash(const CTransaction &tx, const CAssetOut &vecOut);
#endif // SYSCOIN_SERVICES_ASSETCONSENSUS_H
