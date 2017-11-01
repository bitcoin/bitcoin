// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LIMITEDMAP_H
#define BITCOIN_LIMITEDMAP_H

#include <assert.h>
#include <map>

/** STL-like map container that only keeps the N elements with the highest value. */
template <typename K, typename V>
class limitedmap
{
public:
    typedef K key_type;
    typedef V mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef typename std::map<K, V>::const_iterator const_iterator;
    typedef typename std::map<K, V>::size_type size_type;

protected:
    std::map<K, V> map;
    typedef typename std::map<K, V>::iterator iterator;
    std::multimap<V, iterator> rmap;
    typedef typename std::multimap<V, iterator>::iterator rmap_iterator;
    size_type nMaxSize;

public:
    limitedmap(size_type nMaxSizeIn)
    {
        assert(nMaxSizeIn > 0);
        nMaxSize = nMaxSizeIn;
    }
    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
    size_type size() const { return map.size(); }
    bool empty() const { return map.empty(); }
    const_iterator find(const key_type &k) const { return map.find(k); }
    size_type count(const key_type &k) const { return map.count(k); }
    void insert(const value_type &x)
    {
        std::pair<iterator, bool> ret = map.insert(x);
        if (ret.second)
        {
            if (map.size() > nMaxSize)
            {
                map.erase(rmap.begin()->second);
                rmap.erase(rmap.begin());
            }
            rmap.insert(make_pair(x.second, ret.first));
        }
    }
    void erase(const key_type &k)
    {
        iterator itTarget = map.find(k);
        if (itTarget == map.end())
            return;
        std::pair<rmap_iterator, rmap_iterator> itPair = rmap.equal_range(itTarget->second);
        for (rmap_iterator it = itPair.first; it != itPair.second; ++it)
            if (it->second == itTarget)
            {
                rmap.erase(it);
                map.erase(itTarget);
                return;
            }
        // Shouldn't ever get here
        assert(0);
    }
    void update(const_iterator itIn, const mapped_type &v)
    {
        // TODO: When we switch to C++11, use map.erase(itIn, itIn) to get the non-const iterator.
        iterator itTarget = map.find(itIn->first);
        if (itTarget == map.end())
            return;
        std::pair<rmap_iterator, rmap_iterator> itPair = rmap.equal_range(itTarget->second);
        for (rmap_iterator it = itPair.first; it != itPair.second; ++it)
            if (it->second == itTarget)
            {
                rmap.erase(it);
                itTarget->second = v;
                rmap.insert(make_pair(v, itTarget));
                return;
            }
        // Shouldn't ever get here
        assert(0);
    }
    size_type max_size() const { return nMaxSize; }
    size_type max_size(size_type s)
    {
        assert(s > 0);
        while (map.size() > s)
        {
            map.erase(rmap.begin()->second);
            rmap.erase(rmap.begin());
        }
        nMaxSize = s;
        return nMaxSize;
    }
};

#endif // BITCOIN_LIMITEDMAP_H
