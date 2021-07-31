// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNORDERED_LRU_CACHE_H
#define BITCOIN_UNORDERED_LRU_CACHE_H

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

template<typename Key, typename Value, typename Hasher, size_t MaxSize = 0, size_t TruncateThreshold = 0>
class unordered_lru_cache
{
private:
    typedef std::unordered_map<Key, std::pair<Value, int64_t>, Hasher> MapType;

    MapType cacheMap;
    size_t maxSize;
    size_t truncateThreshold;
    int64_t accessCounter{0};

public:
    explicit unordered_lru_cache(size_t _maxSize = MaxSize, size_t _truncateThreshold = TruncateThreshold) :
        maxSize(_maxSize),
        truncateThreshold(_truncateThreshold == 0 ? _maxSize * 2 : _truncateThreshold)
    {
        // either specify maxSize through template arguments or the constructor and fail otherwise
        assert(_maxSize != 0);
    }

    size_t max_size() const { return maxSize; }

    template<typename Value2>
    void _emplace(const Key& key, Value2&& v)
    {
        truncate_if_needed();
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            cacheMap.emplace(key, std::make_pair(std::forward<Value2>(v), accessCounter++));
        } else {
            it->second.first = std::forward<Value2>(v);
            it->second.second = accessCounter++;
        }
    }

    void emplace(const Key& key, Value&& v)
    {
        _emplace(key, v);
    }

    void insert(const Key& key, const Value& v)
    {
        _emplace(key, v);
    }

    bool get(const Key& key, Value& value)
    {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            it->second.second = accessCounter++;
            value = it->second.first;
            return true;
        }
        return false;
    }

    bool exists(const Key& key)
    {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            it->second.second = accessCounter++;
            return true;
        }
        return false;
    }

    void erase(const Key& key)
    {
        cacheMap.erase(key);
    }

    void clear()
    {
        cacheMap.clear();
    }

private:
    void truncate_if_needed()
    {
        typedef typename MapType::iterator Iterator;

        if (cacheMap.size() <= truncateThreshold) {
            return;
        }

        std::vector<Iterator> vec;
        vec.reserve(cacheMap.size());
        for (auto it = cacheMap.begin(); it != cacheMap.end(); ++it) {
            vec.emplace_back(it);
        }
        // sort by last access time (descending order)
        std::sort(vec.begin(), vec.end(), [](const Iterator& it1, const Iterator& it2) {
            return it1->second.second > it2->second.second;
        });

        for (size_t i = maxSize; i < vec.size(); i++) {
            cacheMap.erase(vec[i]);
        }
    }
};

#endif // BITCOIN_UNORDERED_LRU_CACHE_H
