// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_NEVMCONSENSUS_H
#define SYSCOIN_SERVICES_NEVMCONSENSUS_H
#include <primitives/transaction.h>
#include <dbwrapper.h>
#include <consensus/params.h>
#include <util/hasher.h>
#include <sync.h>
class TxValidationState;
class CCoinsViewCache;
class CTxUndo;
class CBlock;
class BlockValidationState;
class CBlockIndexDB;
class CNEVMData;
class CDBBatch;
namespace node {
    struct NodeContext;
} // namespace node
    
class CNEVMDataDB : public CDBWrapper {
public:
    mutable Mutex cs_cache; // Mutex to protect cache operations
private:
    PoDAMAPMemory mapCache GUARDED_BY(cs_cache);
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const NEVMDataVec &vecDataKeys) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool FlushCacheToDisk(const int64_t nMedianTime, bool fSync = true) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    void FlushDataToCache(const PoDAMAPMemory &mapPoDA) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool PruneStandalone(const int64_t nMedianTime, bool fSync = true) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool PruneToBatch(CDBBatch& batch,CDBBatch& batchblob,  const int64_t nMedianTime) EXCLUSIVE_LOCKS_REQUIRED(cs_cache);
    bool GetBlobMetaData(const std::vector<uint8_t>& vchVersionhash, MapPoDAPayloadMeta& meta) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool BlobExists(const std::vector<uint8_t>& vchVersionhash) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    const PoDAMAPMemory& GetCache() const EXCLUSIVE_LOCKS_REQUIRED(cs_cache);
};
class CNEVMDataBlobDB : public CDBWrapper {
    public:
        using CDBWrapper::CDBWrapper;
        bool FlushErase(const NEVMDataVec &vecDataKeys);
    };
extern std::unique_ptr<CNEVMDataDB> pnevmdatadb;
extern std::unique_ptr<CNEVMDataBlobDB> pnevmdatablobdb;
bool DisconnectSyscoinTransaction(const CTransaction& tx, NEVMMintTxSet &setMintTxs);
#endif // SYSCOIN_SERVICES_NEVMCONSENSUS_H
