// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_EVODB_H
#define SYSCOIN_EVO_EVODB_H

#include <dbwrapper.h>
#include <sync.h>
#include <uint256.h>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <utility>
#include <logging.h>

template <typename K, typename V, typename Hasher = std::hash<K>>
class CEvoDB : public CDBWrapper {
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator, Hasher> mapCache;
    std::list<std::pair<K, V>> fifoList;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator, Hasher> mapReadCache;
    std::list<std::pair<K, V>> readFifoList;
    std::unordered_set<K, Hasher> setEraseCache;
    size_t maxCacheSize{0};
    size_t maxReadCacheSize{0};
    DBParams m_db_params;
    bool bFlushOnNextRead{false};
public:
    mutable RecursiveMutex cs;
    using CDBWrapper::CDBWrapper;
    explicit CEvoDB(const DBParams &db_params, size_t maxCacheSizeIn, size_t maxReadCacheSizeIn = 0)
        : CDBWrapper(db_params),
          maxCacheSize(maxCacheSizeIn),
          maxReadCacheSize(maxReadCacheSizeIn),
          m_db_params(db_params)
    {
    }
    ~CEvoDB() {
        FlushCacheToDisk();
    }
private:
    void TrimReadCache()
    {
        while (maxReadCacheSize == 0 ? !readFifoList.empty() : readFifoList.size() > maxReadCacheSize) {
            mapReadCache.erase(readFifoList.front().first);
            readFifoList.pop_front();
        }
    }

    void EraseReadCache(const K& key)
    {
        auto it = mapReadCache.find(key);
        if (it == mapReadCache.end()) {
            return;
        }
        readFifoList.erase(it->second);
        mapReadCache.erase(it);
    }

    void WriteReadCache(const K& key, const V& value)
    {
        if (maxReadCacheSize == 0) {
            return;
        }
        EraseReadCache(key);
        readFifoList.emplace_back(key, value);
        mapReadCache[key] = --readFifoList.end();
        TrimReadCache();
    }

    void TouchReadCache(const typename std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator, Hasher>::iterator& it)
    {
        readFifoList.splice(readFifoList.end(), readFifoList, it->second);
    }
public:
    bool IsCacheFull() const {
        LOCK(cs);
        return maxCacheSize > 0 && (mapCache.size()+setEraseCache.size()) >= maxCacheSize;
    }
    DBParams GetDBParams() const {
        return m_db_params;
    }
    void SetReadCacheSize(size_t maxReadCacheSizeIn)
    {
        LOCK(cs);
        maxReadCacheSize = maxReadCacheSizeIn;
        TrimReadCache();
    }
    size_t GetReadCacheSize() const
    {
        LOCK(cs);
        return mapReadCache.size();
    }


    bool ReadCache(const K& key, V& value) {
        LOCK(cs);
        if(bFlushOnNextRead) {
            bFlushOnNextRead = false;
            LogPrint(BCLog::SYS, "Evodb::ReadCache flushing cache before read\n");
            FlushCacheToDisk();
        }
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            value = it->second->second;
            return true;
        }
        auto it_read = mapReadCache.find(key);
        if (it_read != mapReadCache.end()) {
            value = it_read->second->second;
            TouchReadCache(it_read);
            return true;
        }
        if (!Read(key, value)) {
            return false;
        }
        WriteReadCache(key, value);
        return true;
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
        WriteReadCache(key, fifoList.back().second);
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
        WriteReadCache(key, fifoList.back().second);
        setEraseCache.erase(key);

        if (maxCacheSize > 0 && mapCache.size() > maxCacheSize) {
            auto oldest = fifoList.front();
            fifoList.pop_front();
            mapCache.erase(oldest.first);
        }
    }

    bool ExistsCache(const K& key) {
        LOCK(cs);
        if(bFlushOnNextRead) {
            bFlushOnNextRead = false;
            LogPrint(BCLog::SYS, "Evodb::ReadCache flushing cache before read\n");
            FlushCacheToDisk();
        }
        auto it_read = mapReadCache.find(key);
        if (it_read != mapReadCache.end()) {
            TouchReadCache(it_read);
            return true;
        }
        return (mapCache.find(key) != mapCache.end() || Exists(key));
    }

    void EraseCache(const K& key) {
        LOCK(cs);
        bFlushOnNextRead = true;
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            fifoList.erase(it->second);
            mapCache.erase(it);
        }
        EraseReadCache(key);
        setEraseCache.insert(key);
    }

    bool FlushCacheToDisk(std::size_t CHUNK_ITEMS = 256)
    {
        LOCK(cs);
        if (mapCache.empty() && setEraseCache.empty()) return true;

        CDBBatch batch(*this);
        std::size_t items = 0;
        std::size_t count = 0;
        auto flush = [&]() {
            if (batch.SizeEstimate() == 0) return true;
            if (!WriteBatch(batch, /*sync=*/true)) return false;
            batch.Clear();
            items = 0;
            return true;
        };

        while (!fifoList.empty()) {
            batch.Write(fifoList.front().first, fifoList.front().second);
            ++items;
            count++;
            if (items == CHUNK_ITEMS || fifoList.size() == 1) {
                if (!flush()) return false;
            }
            mapCache.erase(fifoList.front().first);
            fifoList.pop_front();
        }

        items = 0;
        for (auto it = setEraseCache.begin(); it != setEraseCache.end(); ) {
            batch.Erase(*it);
            ++items;
            count++;
            if (items == CHUNK_ITEMS) {
                if (!flush()) return false;
                it = setEraseCache.erase(setEraseCache.begin(), ++it);
                items = 0;
            } else {
                ++it;
            }
        }
        if (!flush()) return false;
        setEraseCache.clear();

        LogPrint(BCLog::SYS,
                "Flushed %zu items to disk (%s) in %zu-item chunks\n",
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
