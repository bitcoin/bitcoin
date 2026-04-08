// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_ASSETCONSENSUS_H
#define SYSCOIN_SERVICES_ASSETCONSENSUS_H
#include <primitives/transaction.h>
#include <dbwrapper.h>
#include <consensus/params.h>
#include <util/hasher.h>
#include <sync.h>
class TxValidationState;
class CTxUndo;
class CBlock;
class CNEVMTxRootsDB : public CDBWrapper {
    NEVMTxRootMap mapCache;
    mutable Mutex cs_cache; // Mutex to protect cache operations (non-recursive for better performance)
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const std::vector<uint256> &vecBlockHashes) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool ReadTxRoots(const uint256& nBlockHash, NEVMTxRoot& txRoot) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool FlushCacheToDisk(std::size_t CHUNK_ITEMS = 100000, bool fSync = true) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    void FlushDataToCache(const NEVMTxRootMap &mapNEVMTxRoots) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
};

class CNEVMMintedTxDB : public CDBWrapper {
    NEVMMintTxSet mapCache;
    mutable Mutex cs_cache; // Mutex to protect cache operations (non-recursive for better performance)
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const NEVMMintTxSet &setMintTxs) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool FlushCacheToDisk(std::size_t CHUNK_ITEMS = 256, bool fSync = true) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    void FlushDataToCache(const NEVMMintTxSet &mapNEVMTxRoots) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool ExistsTx(const uint256& nTxHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
};

extern std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
extern std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
bool DisconnectMintAsset(const CTransaction &tx, NEVMMintTxSet &setMintTxs);
bool CheckSyscoinMint(const CTransaction& tx, 
    const uint256& txHash,
    TxValidationState &tstate,
    const uint32_t& nHeight, 
    const bool &fJustCheck, 
    NEVMMintTxSet &setMintTxs, 
    CAssetsMap &mapAssetIn, 
    CAssetsMap &mapAssetOut);
bool CheckSyscoinMintInternal(const CMintSyscoin &mintSyscoin,
    TxValidationState &state,
    const bool &fJustCheck,
    NEVMMintTxSet &setMintTxs,
    uint64_t &nAssetFromLog,
    CAmount &outputAmount,
    std::string &witnessAddress);
bool CheckSyscoinInputs(const Consensus::Params& params, 
    const CTransaction& tx, 
    const uint256& txHash, 
    TxValidationState &tstate, 
    const uint32_t &nHeight, 
    const bool &fJustCheck, 
    NEVMMintTxSet &setMintTxs, 
    CAssetsMap& mapAssetIn, 
    CAssetsMap& mapAssetOut);
bool CheckAssetAllocationInputs(const CTransaction &tx, 
    const uint256& txHash, 
    TxValidationState &tstate, 
    const uint32_t &nHeight, 
    const bool &fJustCheck, 
    CAssetsMap &mapAssetIn, 
    CAssetsMap &mapAssetOut);
std::string stringFromSyscoinTx(const int &nVersion);
#endif // SYSCOIN_SERVICES_ASSETCONSENSUS_H
