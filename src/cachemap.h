// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CACHEMAP_H_
#define CACHEMAP_H_

#include <map>
#include <list>
#include <cstddef>

#include "serialize.h"

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

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(key);
        READWRITE(value);
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

    size_type nCurrentSize;

    list_t listItems;

    map_t mapIndex;

public:
    CacheMap(size_type nMaxSizeIn = 0)
        : nMaxSize(nMaxSizeIn),
          nCurrentSize(0),
          listItems(),
          mapIndex()
    {}

    CacheMap(const CacheMap<K,V>& other)
        : nMaxSize(other.nMaxSize),
          nCurrentSize(other.nCurrentSize),
          listItems(other.listItems),
          mapIndex()
    {
        RebuildIndex();
    }

    void Clear()
    {
        mapIndex.clear();
        listItems.clear();
        nCurrentSize = 0;
    }

    void SetMaxSize(size_type nMaxSizeIn)
    {
        nMaxSize = nMaxSizeIn;
    }

    size_type GetMaxSize() const {
        return nMaxSize;
    }

    size_type GetSize() const {
        return nCurrentSize;
    }

    void Insert(const K& key, const V& value)
    {
        map_it it = mapIndex.find(key);
        if(it != mapIndex.end()) {
            item_t& item = *(it->second);
            item.value = value;
            return;
        }
        if(nCurrentSize == nMaxSize) {
            PruneLast();
        }
        listItems.push_front(item_t(key, value));
        mapIndex[key] = listItems.begin();
        ++nCurrentSize;
    }

    bool HasKey(const K& key) const
    {
        map_cit it = mapIndex.find(key);
        return (it != mapIndex.end());
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
        --nCurrentSize;
    }

    const list_t& GetItemList() const {
        return listItems;
    }

    CacheMap<K,V>& operator=(const CacheMap<K,V>& other)
    {
        nMaxSize = other.nMaxSize;
        nCurrentSize = other.nCurrentSize;
        listItems = other.listItems;
        RebuildIndex();
        return *this;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(nMaxSize);
        READWRITE(nCurrentSize);
        READWRITE(listItems);
        if(ser_action.ForRead()) {
            RebuildIndex();
        }
    }

private:
    void PruneLast()
    {
        if(nCurrentSize < 1) {
            return;
        }
        item_t& item = listItems.back();
        mapIndex.erase(item.key);
        listItems.pop_back();
        --nCurrentSize;
    }

    void RebuildIndex()
    {
        mapIndex.clear();
        for(list_it it = listItems.begin(); it != listItems.end(); ++it) {
            mapIndex[it->key] = it;
        }
    }
};

#endif /* CACHEMAP_H_ */
