// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CACHEMULTIMAP_H
#define BITCOIN_CACHEMULTIMAP_H

#include <cstddef>
#include <map>
#include <list>
#include <set>

#include <serialize.h>

#include <cachemap.h>

/**
 * Map like container that keeps the N most recently added items
 */
template<typename K, typename V, typename Size = uint32_t>
class CacheMultiMap
{
public:
    typedef Size size_type;

    typedef CacheItem<K,V> item_t;

    typedef std::list<item_t> list_t;

    typedef typename list_t::iterator list_it;

    typedef typename list_t::const_iterator list_cit;

    typedef std::map<V,list_it> it_map_t;

    typedef typename it_map_t::iterator it_map_it;

    typedef typename it_map_t::const_iterator it_map_cit;

    typedef std::map<K, it_map_t> map_t;

    typedef typename map_t::iterator map_it;

    typedef typename map_t::const_iterator map_cit;

private:
    size_type nMaxSize;

    list_t listItems;

    map_t mapIndex;

public:
    explicit CacheMultiMap(size_type nMaxSizeIn = 0)
        : nMaxSize(nMaxSizeIn),
          listItems(),
          mapIndex()
    {}

    explicit CacheMultiMap(const CacheMap<K,V>& other)
        : nMaxSize(other.nMaxSize),
          listItems(other.listItems),
          mapIndex()
    {
        RebuildIndex();
    }

    void Clear()
    {
        mapIndex.clear();
        listItems.clear();
    }

    void SetMaxSize(size_type nMaxSizeIn)
    {
        nMaxSize = nMaxSizeIn;
    }

    size_type GetMaxSize() const {
        return nMaxSize;
    }

    size_type GetSize() const {
        return listItems.size();
    }

    bool Insert(const K& key, const V& value)
    {
        map_it mit = mapIndex.find(key);
        if(mit == mapIndex.end()) {
            mit = mapIndex.emplace(key, it_map_t()).first;
        }
        it_map_t& mapIt = mit->second;

        if(mapIt.count(value) > 0) {
            // Don't insert duplicates
            return false;
        }

        if(listItems.size() == nMaxSize) {
            PruneLast();
        }
        listItems.push_front(item_t(key, value));
        mapIt.emplace(value, listItems.begin());
        return true;
    }

    bool HasKey(const K& key) const
    {
        return (mapIndex.find(key) != mapIndex.end());
    }

    bool Get(const K& key, V& value) const
    {
        map_cit it = mapIndex.find(key);
        if(it == mapIndex.end()) {
            return false;
        }
        const it_map_t& mapIt = it->second;
        const item_t& item = *(mapIt.begin()->second);
        value = item.value;
        return true;
    }

    bool GetAll(const K& key, std::vector<V>& vecValues)
    {
        map_cit mit = mapIndex.find(key);
        if(mit == mapIndex.end()) {
            return false;
        }
        const it_map_t& mapIt = mit->second;

        for(it_map_cit it = mapIt.begin(); it != mapIt.end(); ++it) {
            const item_t& item = *(it->second);
            vecValues.push_back(item.value);
        }
        return true;
    }

    void GetKeys(std::vector<K>& vecKeys)
    {
        for(map_cit it = mapIndex.begin(); it != mapIndex.end(); ++it) {
            vecKeys.push_back(it->first);
        }
    }

    void Erase(const K& key)
    {
        map_it mit = mapIndex.find(key);
        if(mit == mapIndex.end()) {
            return;
        }
        it_map_t& mapIt = mit->second;

        for(it_map_it it = mapIt.begin(); it != mapIt.end(); ++it) {
            listItems.erase(it->second);
        }

        mapIndex.erase(mit);
    }

    void Erase(const K& key, const V& value)
    {
        map_it mit = mapIndex.find(key);
        if(mit == mapIndex.end()) {
            return;
        }
        it_map_t& mapIt = mit->second;

        it_map_it it = mapIt.find(value);
        if(it == mapIt.end()) {
            return;
        }

        listItems.erase(it->second);
        mapIt.erase(it);

        if(mapIt.empty()) {
            mapIndex.erase(mit);
        }
    }

    const list_t& GetItemList() const {
        return listItems;
    }

    CacheMap<K,V>& operator=(const CacheMap<K,V>& other)
    {
        nMaxSize = other.nMaxSize;
        listItems = other.listItems;
        RebuildIndex();
        return *this;
    }

    SERIALIZE_METHODS(CacheMultiMap, obj)
    {
        READWRITE(obj.nMaxSize, obj.listItems);
        SER_READ(obj, obj.RebuildIndex());
    }

private:
    void PruneLast()
    {
        if(listItems.empty()) {
            return;
        }

        list_it lit = listItems.end();
        --lit;
        item_t& item = *lit;

        map_it mit = mapIndex.find(item.key);

        if(mit != mapIndex.end()) {
            it_map_t& mapIt = mit->second;

            mapIt.erase(item.value);

            if(mapIt.empty()) {
                mapIndex.erase(item.key);
            }
        }

        listItems.pop_back();
    }

    void RebuildIndex()
    {
        mapIndex.clear();
        for(list_it lit = listItems.begin(); lit != listItems.end(); ++lit) {
            item_t& item = *lit;
            map_it mit = mapIndex.find(item.key);
            if(mit == mapIndex.end()) {
                mit = mapIndex.emplace(item.key, it_map_t()).first;
            }
            it_map_t& mapIt = mit->second;
            mapIt.emplace(item.value, lit);
        }
    }
};

#endif // BITCOIN_CACHEMULTIMAP_H
