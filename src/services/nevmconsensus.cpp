// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <services/nevmconsensus.h>
#include <services/assetconsensus.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <nevm/nevm.h>
#include <nevm/address.h>
#include <nevm/sha3.h>
#include <messagesigner.h>
#include <logging.h>
#include <util/rbf.h>
#include <undo.h>
#include <validationinterface.h>
#include <timedata.h>
#include <key_io.h>
#include <logging.h>
std::unique_ptr<CBlockIndexDB> pblockindexdb;
std::unique_ptr<CNEVMDataDB> pnevmdatadb;
std::unique_ptr<CNEVMDataBlobDB> pnevmdatablobdb;
bool fNEVMConnection = false;
bool fRegTest = false;
bool fSigNet = false;

bool DisconnectSyscoinTransaction(const CTransaction& tx, NEVMMintTxSet &setMintTxs) {
 
    if(IsSyscoinMintTx(tx.nVersion)) {
        if(!DisconnectMintAsset(tx, setMintTxs))
            return false;       
    }
    return true;       
}

void CNEVMDataDB::FlushDataToCache(const PoDAMAPMemory &mapPoDA) {
    LOCK(cs_cache);
    if(mapPoDA.empty()) {
        return;
    }
    CDBBatch batchblob(*pnevmdatablobdb);  
    for (auto const& [key, val] : mapPoDA) {
        if(!val.vchNEVMData) {
            continue;
        }
        if(Exists(key)) {
            continue;
        }
        auto inserted = mapCache.try_emplace(key, val.txid, val.nSize, val.nMedianTime);
        if(!inserted.second) {
            continue;
        }
        if(!pnevmdatablobdb->Exists(key)) {
            batchblob.Write(key, val.vchNEVMData);
        }
    }
    if(batchblob.SizeEstimate() > 0) {
        pnevmdatablobdb->WriteBatch(batchblob);
    }
}
bool CNEVMDataDB::FlushCacheToDisk(const int64_t nMedianTime, bool fSync) {
    bool cacheEmpty = false;
    {
        LOCK(cs_cache);
        cacheEmpty = mapCache.empty();
    }
    if(cacheEmpty) {
        if(fTestNet) {
            return PruneStandalone(nMedianTime, fSync);
        }
        return true;
    }
    LOCK(cs_cache);
    CDBBatch batch(*this);
    // only prune on testnet flush, mainnet relies only on CL
    if(fTestNet) {
        CDBBatch batchblob(*pnevmdatablobdb);
        if (!PruneToBatch(batch, batchblob, nMedianTime)) {
            LogPrint(BCLog::SYS, "Error: Could not prune nevm blobs\n");
            return false;
        }
        pnevmdatablobdb->WriteBatch(batchblob);
    }
    for (auto const& [key, val] : mapCache) {
        batch.Write(key, val);
    }
    if(mapCache.size() > 0)
        LogPrint(BCLog::SYS, "Flushing cache to disk, storing %d nevm blobs\n", mapCache.size());
    bool res = WriteBatch(batch, fSync);
    if(res) {
        mapCache.clear();
    }
    return res;
}
bool CNEVMDataDB::FlushErase(const NEVMDataVec &vecDataKeys) {
    LOCK(cs_cache);
    if(vecDataKeys.empty())
        return true;
    CDBBatch batch(*this);    
    for (const auto &key : vecDataKeys) {
        batch.Erase(key);
        // remove from cache as well
        auto it = mapCache.find(key);
        if(it != mapCache.end())
            mapCache.erase(it);
    }
    if(vecDataKeys.size() > 0)
        LogPrint(BCLog::SYS, "Flushing, erasing %d nevm blob keys\n", vecDataKeys.size());
    return WriteBatch(batch, true) && pnevmdatablobdb->FlushErase(vecDataKeys);
}
bool CNEVMDataBlobDB::FlushErase(const NEVMDataVec &vecDataKeys) {
    CDBBatch batch(*this);    
    for (const auto &key : vecDataKeys) {
        batch.Erase(key);
    }
    return WriteBatch(batch, true);
}
bool CNEVMDataDB::BlobExists(const std::vector<uint8_t>& vchVersionHash) {
    LOCK(cs_cache);
    return (mapCache.find(vchVersionHash) != mapCache.end()) || Exists(vchVersionHash);
}
bool CNEVMDataDB::GetBlobMetaData(const std::vector<uint8_t>& vchVersionHash, MapPoDAPayloadMeta& meta) {
    LOCK(cs_cache);
    auto it = mapCache.find(vchVersionHash);
    if (it != mapCache.end()) {
        meta = it->second;
        return true;
    } 
    if(Exists(vchVersionHash)) {
        return Read(vchVersionHash, meta);
    }
    return false;
}
const PoDAMAPMemory& CNEVMDataDB::GetCache() const {
    AssertLockHeld(cs_cache);
    return mapCache;
}
bool CNEVMDataDB::PruneToBatch(
    CDBBatch& batch,
    CDBBatch& batchblob,
    const int64_t nMedianTime)
{
    AssertLockHeld(cs_cache);
    int nCount = 0;
    auto it = mapCache.begin();
    while (it != mapCache.end()) {
        const int64_t entryTime = it->second.nMedianTime;
        bool isExpired = nMedianTime > (entryTime + NEVM_DATA_EXPIRE_TIME);
        if (isExpired) {
            batchblob.Erase(it->first);
            it = mapCache.erase(it);
            ++nCount;
        } else {
            ++it;
        }
    }
    
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    std::vector<uint8_t> vchVersionHash;
    MapPoDAPayloadMeta meta;
    while (pcursor->Valid()) {
        try {
            if (!pcursor->GetKey(vchVersionHash)) {
                pcursor->Next();
                continue;
            }
            if (pcursor->GetValue(meta)) {
                bool isExpired = nMedianTime > (meta.nMedianTime + NEVM_DATA_EXPIRE_TIME);
                if (isExpired) {
                    batch.Erase(vchVersionHash);
                    batchblob.Erase(vchVersionHash);
                    ++nCount;
                }
            }
            pcursor->Next();
        } catch (const std::exception& e) {
            return error("%s() : deserialize error: %s", __func__, e.what());
        }
    }
    if(nCount > 0)
        LogPrint(BCLog::SYS, "PruneToBatch pruned %d nevm blobs\n", nCount);

    return true;
}

bool CNEVMDataDB::PruneStandalone(const int64_t nMedianTime, bool fSync)
{
    LOCK(cs_cache);
    CDBBatch batch(*this);
    CDBBatch batchblob(*pnevmdatablobdb);
    if (!PruneToBatch(batch, batchblob, nMedianTime)) {
        return false;
    }
    return WriteBatch(batch, fSync) && pnevmdatablobdb->WriteBatch(batchblob, fSync);
}
