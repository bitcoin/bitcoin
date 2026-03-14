// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_EVODB_H
#define SYSCOIN_EVO_EVODB_H

#include <dbwrapper.h>
#include <sync.h>
#include <uint256.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <list>
#include <utility>
#include <logging.h>

template <typename K, typename V, typename Hasher = std::hash<K>>
class CEvoDB : public CDBWrapper {
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator, Hasher> mapCache;
    std::list<std::pair<K, V>> fifoList;
    std::unordered_set<K, Hasher> setEraseCache;
    size_t maxCacheSize{0};
    DBParams m_db_params;
    bool bFlushOnNextRead{false};
    std::function<bool(CDBBatch&)> m_write_batch_hook_for_testing;
    std::function<void()> m_before_read_lock_hook_for_testing;

    struct PendingState {
        std::list<std::pair<K, V>> writes;
        std::unordered_set<K, Hasher> erases;

        bool Empty() const
        {
            return writes.empty() && erases.empty();
        }
    };

    void RunBeforeReadLockHookForTesting()
    {
        std::function<void()> hook;
        {
            LOCK(cs);
            hook = m_before_read_lock_hook_for_testing;
        }

        if (hook) {
            hook();
        }
    }

    PendingState ExtractPendingState()
    {
        LOCK(cs);
        PendingState pending;
        pending.writes = std::move(fifoList);
        pending.erases = std::move(setEraseCache);
        mapCache.clear();
        fifoList.clear();
        setEraseCache.clear();
        return pending;
    }

    void RebuildMapCache()
    {
        mapCache.clear();
        for (auto it = fifoList.begin(); it != fifoList.end(); ++it) {
            mapCache[it->first] = it;
        }
    }

    void RestorePendingState(PendingState&& pending)
    {
        LOCK(cs);
        if (pending.Empty()) {
            return;
        }

        std::list<std::pair<K, V>> restored_writes;
        for (auto& entry : pending.writes) {
            if (mapCache.find(entry.first) != mapCache.end() || setEraseCache.find(entry.first) != setEraseCache.end()) {
                continue;
            }
            restored_writes.emplace_back(std::move(entry));
        }

        if (!restored_writes.empty()) {
            restored_writes.splice(restored_writes.end(), fifoList);
            fifoList.swap(restored_writes);
            RebuildMapCache();
        }

        for (const auto& key : pending.erases) {
            if (mapCache.find(key) == mapCache.end()) {
                setEraseCache.insert(key);
            }
        }
    }

    bool SubmitBatch(CDBBatch& batch)
    {
        std::function<bool(CDBBatch&)> hook;
        {
            LOCK(cs);
            hook = m_write_batch_hook_for_testing;
        }

        if (hook) {
            return hook(batch);
        }
        return WriteBatch(batch, /*sync=*/true);
    }
public:
    mutable RecursiveMutex cs;
    using CDBWrapper::CDBWrapper;
    explicit CEvoDB(const DBParams &db_params, size_t maxCacheSizeIn) : CDBWrapper(db_params), maxCacheSize(maxCacheSizeIn), m_db_params(db_params) {
    }
    ~CEvoDB() {
        FlushCacheToDisk();
    }
    bool IsCacheFull() const {
        LOCK(cs);
        return maxCacheSize > 0 && (mapCache.size()+setEraseCache.size()) >= maxCacheSize;
    }
    DBParams GetDBParams() const {
        return m_db_params;
    }
    bool SubmitBatchForTesting(CDBBatch& batch)
    {
        return SubmitBatch(batch);
    }
    void SetWriteBatchHookForTesting(std::function<bool(CDBBatch&)> hook)
    {
        LOCK(cs);
        m_write_batch_hook_for_testing = std::move(hook);
    }
    void SetBeforeReadLockHookForTesting(std::function<void()> hook)
    {
        LOCK(cs);
        m_before_read_lock_hook_for_testing = std::move(hook);
    }
    bool ReadCache(const K& key, V& value) {
        while (true) {
            RunBeforeReadLockHookForTesting();
            std::unique_lock<RecursiveMutex> lock(cs);
            if (bFlushOnNextRead) {
                bFlushOnNextRead = false;
                lock.unlock();
                LogPrint(BCLog::SYS, "Evodb::ReadCache flushing cache before read\n");
                FlushCacheToDisk();
                continue;
            }
            auto it = mapCache.find(key);
            if (it != mapCache.end()) {
                value = it->second->second;
                return true;
            }
            lock.unlock();
            return Read(key, value);
        }
    }
    template <typename Callback>
    void ForEachCachedEntry(Callback&& cb) {
        while (true) {
            RunBeforeReadLockHookForTesting();
            std::unique_lock<RecursiveMutex> lock(cs);
            if (bFlushOnNextRead) {
                bFlushOnNextRead = false;
                lock.unlock();
                LogPrint(BCLog::SYS, "Evodb::ReadCache flushing cache before read\n");
                FlushCacheToDisk();
                continue;
            }
            for (const auto& [key, it] : mapCache) {
                cb(key, it->second);
            }
            return;
        }
    }

    std::unordered_set<K, Hasher> GetEraseCacheCopy() const {
        LOCK(cs);
        return setEraseCache;
    }

    template <typename Callback>
    void ForEachEraseEntry(Callback&& cb) const {
        LOCK(cs);
        for (const auto& key : setEraseCache) {
            cb(key);
        }
    }

    void RestoreCaches(const std::unordered_map<K, V, Hasher>& mapCacheCopy, const std::unordered_set<K, Hasher>& eraseCacheCopy) {
        LOCK(cs);
        for (const auto& [key, value] : mapCacheCopy) {
            WriteCache(key, value);
        }
        setEraseCache = eraseCacheCopy;
    }

    void ClearCaches() {
        LOCK(cs);
        mapCache.clear();
        fifoList.clear();
        setEraseCache.clear();
        bFlushOnNextRead = false;
    }

    void WriteCache(const K& key, V&& value) {
        LOCK(cs);
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            fifoList.erase(it->second);
            mapCache.erase(it);
        }
        fifoList.emplace_back(key, std::move(value));
        mapCache[key] = --fifoList.end();
        setEraseCache.erase(key);

        if (maxCacheSize > 0 && mapCache.size() > maxCacheSize) {
            auto oldest = fifoList.front();
            fifoList.pop_front();
            mapCache.erase(oldest.first);
        }
    }

    void WriteCache(const K& key, const V& value) {
        LOCK(cs);
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            fifoList.erase(it->second);
            mapCache.erase(it);
        }
        fifoList.emplace_back(key, value);
        mapCache[key] = --fifoList.end();
        setEraseCache.erase(key);

        if (maxCacheSize > 0 && mapCache.size() > maxCacheSize) {
            auto oldest = fifoList.front();
            fifoList.pop_front();
            mapCache.erase(oldest.first);
        }
    }

    bool ExistsCache(const K& key) {
        while (true) {
            RunBeforeReadLockHookForTesting();
            std::unique_lock<RecursiveMutex> lock(cs);
            if (bFlushOnNextRead) {
                bFlushOnNextRead = false;
                lock.unlock();
                LogPrint(BCLog::SYS, "Evodb::ReadCache flushing cache before read\n");
                FlushCacheToDisk();
                continue;
            }
            if (mapCache.find(key) != mapCache.end()) {
                return true;
            }
            lock.unlock();
            return Exists(key);
        }
    }

    void EraseCache(const K& key) {
        LOCK(cs);
        bFlushOnNextRead = true;
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            fifoList.erase(it->second);
            mapCache.erase(it);
        }
        setEraseCache.insert(key);
    }

    bool FlushCacheToDisk(std::size_t CHUNK_ITEMS = 256)
    {
        PendingState pending{ExtractPendingState()};
        if (pending.Empty()) return true;

        CDBBatch batch(*this);
        std::size_t items = 0;
        std::size_t count = 0;
        auto flush = [&]() {
            if (batch.SizeEstimate() == 0) return true;
            if (!SubmitBatch(batch)) return false;
            batch.Clear();
            items = 0;
            return true;
        };

        auto write_it = pending.writes.begin();
        while (write_it != pending.writes.end()) {
            batch.Write(write_it->first, write_it->second);
            ++items;
            count++;
            ++write_it;
            if (items == CHUNK_ITEMS) {
                auto committed_end = write_it;
                auto failing_begin = committed_end;
                std::advance(failing_begin, -static_cast<long>(items));
                if (!flush()) {
                    pending.writes.erase(pending.writes.begin(), failing_begin);
                    RestorePendingState(std::move(pending));
                    return false;
                }
                pending.writes.erase(pending.writes.begin(), committed_end);
            }
        }
        if (!flush()) {
            RestorePendingState(std::move(pending));
            return false;
        }
        pending.writes.clear();

        items = 0;
        auto chunk_begin = pending.erases.begin();
        for (auto it = pending.erases.begin(); it != pending.erases.end(); ) {
            if (items == 0) {
                chunk_begin = it;
            }
            batch.Erase(*it);
            ++items;
            count++;
            if (items == CHUNK_ITEMS) {
                auto committed_end = std::next(it);
                if (!flush()) {
                    pending.erases.erase(pending.erases.begin(), chunk_begin);
                    RestorePendingState(std::move(pending));
                    return false;
                }
                it = pending.erases.erase(pending.erases.begin(), committed_end);
                items = 0;
            } else {
                ++it;
            }
        }
        if (!flush()) {
            RestorePendingState(std::move(pending));
            return false;
        }
        pending.erases.clear();

        LogPrint(BCLog::SYS,
                "Flushed %zu items to cache (%s) in %zu-item chunks (sync_each=1)\n",
                count, GetName().c_str(), CHUNK_ITEMS);
        return true;
    }

    int64_t CountPersistedEntries() {
        try {
            std::unique_ptr<CDBIterator> pcursor(NewIterator());
            if (!pcursor) {
                 LogPrint(BCLog::SYS, "CEvoDB::%s -- Failed to create DB iterator\n", __func__);
                 return -1; // Indicate error
            }
            int64_t count = 0;
            // We only need to iterate keys, values are not needed for count
            pcursor->SeekToFirst();
            while (pcursor->Valid()) {
                count++;
                pcursor->Next();
            }
            return count;
        } catch (const std::exception& e) {
             LogPrint(BCLog::SYS, "CEvoDB::%s -- Exception during iteration: %s\n", __func__, e.what());
            return -1; // Indicate error
        } catch (...) {
             LogPrint(BCLog::SYS, "CEvoDB::%s -- Unknown exception during iteration\n", __func__);
            return -1; // Indicate error
        }
    }
    size_t GetReadWriteCacheSize() {
        LOCK(cs);
        return mapCache.size();
    }
    size_t GetEraseCacheSize() {
        LOCK(cs);
        return setEraseCache.size();
    }
    // Getter for testing purposes
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator, Hasher> GetMapCache() const {
        return mapCache;
    }

    std::list<std::pair<K, V>> GetFifoList() const {
        return fifoList;
    }

};

#endif // SYSCOIN_EVO_EVODB_H
