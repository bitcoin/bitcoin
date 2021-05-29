// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CACHEMAP_H
#define BITCOIN_CACHEMAP_H

#include <map>
#include <list>
#include <cstddef>

#include <serialize.h>

/**
 * Serializable structure for key/value items
 */
template<typename K, typename V>
struct CacheItem
{
    CacheItem()
    {}

    CacheItem(const K& keyIn, const V& valueIn)
    : key(keyIn),
      value(valueIn)
    {}

    K key;
    V value;

    SERIALIZE_METHODS(CacheItem, obj)
    {
        READWRITE(obj.key, obj.value);
    }
};


/**
 * Map like container that keeps the N most recently added items
 */
template<typename K, typename V, typename Size = uint32_t>
class CacheMap
{
public:
    typedef Size size_type;

    typedef CacheItem<K,V> item_t;

    typedef std::list<item_t> list_t;

    typedef typename list_t::iterator list_it;

    typedef typename list_t::const_iterator list_cit;

    typedef std::map<K, list_it> map_t;

    typedef typename map_t::iterator map_it;

    typedef typename map_t::const_iterator map_cit;

private:
    size_type nMaxSize;

    list_t listItems;

    map_t mapIndex;

public:
    explicit CacheMap(size_type nMaxSizeIn = 0)
        : nMaxSize(nMaxSizeIn),
          listItems(),
          mapIndex()
    {}

    CacheMap(const CacheMap<K,V>& other)
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
        if(mapIndex.find(key) != mapIndex.end()) {
            return false;
        }
        if(listItems.size() == nMaxSize) {
            PruneLast();
        }
        listItems.push_front(item_t(key, value));
        mapIndex.emplace(key, listItems.begin());
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
        item_t& item = *(it->second);
        value = item.value;
        return true;
    }

    void Erase(const K& key)
    {
        map_it it = mapIndex.find(key);
        if(it == mapIndex.end()) {
            return;
        }
        listItems.erase(it->second);
        mapIndex.erase(it);
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

    SERIALIZE_METHODS(CacheMap, obj)
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
        item_t& item = listItems.back();
        mapIndex.erase(item.key);
        listItems.pop_back();
    }

    void RebuildIndex()
    {
        mapIndex.clear();
        for(list_it it = listItems.begin(); it != listItems.end(); ++it) {
            mapIndex.emplace(it->key, it);
        }
    }
};

#endif // BITCOIN_CACHEMAP_H
