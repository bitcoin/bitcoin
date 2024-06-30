// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_EVODB_H
#define SYSCOIN_EVO_EVODB_H

#include <dbwrapper.h>
#include <sync.h>
#include <uint256.h>
#include <unordered_map>
#include <list>
#include <utility>
#include <logging.h>

template <typename K, typename V>
class CEvoDB : public CDBWrapper {
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> mapCache;
    std::list<std::pair<K, V>> fifoList;
    std::unordered_set<K> setEraseCache;
    mutable RecursiveMutex cs;
    size_t maxCacheSize;

public:
    using CDBWrapper::CDBWrapper;
    explicit CEvoDB(const DBParams &db_params, size_t maxCacheSize) : CDBWrapper(db_params), maxCacheSize(maxCacheSize) {}

    bool ReadCache(const K& key, V& value) const {
        LOCK(cs);
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            value = it->second->second;
            return true;
        }
        return Read(key, value);
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

        if (mapCache.size() > maxCacheSize) {
            auto oldest = fifoList.front();
            fifoList.pop_front();
            mapCache.erase(oldest.first);
        }
    }

    bool ExistsCache(const K& key) const {
        LOCK(cs);
        return (mapCache.find(key) != mapCache.end() || Exists(key));
    }

    void EraseCache(const K& key) {
        LOCK(cs);
        auto it = mapCache.find(key);
        if (it != mapCache.end()) {
            fifoList.erase(it->second);
            mapCache.erase(it);
        }
        setEraseCache.insert(key);
    }

    bool FlushCacheToDisk() {
        LOCK(cs);
        if (mapCache.empty() && setEraseCache.empty()) {
            return true;
        }
        CDBBatch batch(*this);
        for (const auto& [key, it] : mapCache) {
            batch.Write(key, it->second);
        }
        for (const auto& key : setEraseCache) {
            batch.Erase(key);
        }
        LogPrintf("Flushing cache (%s) to disk, storing %d items, erasing %d items\n", GetName(), mapCache.size(), setEraseCache.size());
        bool res = WriteBatch(batch, true);
        mapCache.clear();
        fifoList.clear();
        setEraseCache.clear();
        return res;
    }
};

#endif // SYSCOIN_EVO_EVODB_H
