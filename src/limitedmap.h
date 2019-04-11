// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LIMITEDMAP_H
#define BITCOIN_LIMITEDMAP_H

#include <assert.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

/** STL-like map container that only keeps the N elements with the highest value. */
// WARNING, this was initially the "limitedmap" class from Bitcoin, but now does not maintain ordering. If any backports
// ever start using this map in a way that requires ordering, do NOT use this as it is but instead reintroduce the original
// limitedmap
template <typename K, typename V, typename Hash = std::hash<K>>
class unordered_limitedmap
{
public:
    typedef K key_type;
    typedef V mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef typename std::unordered_map<K, V, Hash>::const_iterator const_iterator;
    typedef typename std::unordered_map<K, V, Hash>::size_type size_type;

protected:
    std::unordered_map<K, V, Hash> map;
    typedef typename std::unordered_map<K, V, Hash>::iterator iterator;
    std::unordered_multimap<V, iterator> rmap;
    typedef typename std::unordered_multimap<V, iterator>::iterator rmap_iterator;
    size_type nMaxSize;
    size_type nPruneAfterSize;

public:
    unordered_limitedmap(size_type nMaxSizeIn, size_type nPruneAfterSizeIn = 0)
    {
        assert(nMaxSizeIn > 0);
        nMaxSize = nMaxSizeIn;
        if (nPruneAfterSizeIn == 0) {
            nPruneAfterSize = nMaxSize;
        } else {
            nPruneAfterSize = nPruneAfterSizeIn;
        }
        assert(nPruneAfterSize >= nMaxSize);
    }
    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
    size_type size() const { return map.size(); }
    bool empty() const { return map.empty(); }
    const_iterator find(const key_type& k) const { return map.find(k); }
    size_type count(const key_type& k) const { return map.count(k); }
    void insert(const value_type& x)
    {
        std::pair<iterator, bool> ret = map.insert(x);
        if (ret.second) {
            prune();
            rmap.insert(make_pair(x.second, ret.first));
        }
    }
    void erase(const key_type& k)
    {
        iterator itTarget = map.find(k);
        if (itTarget == map.end())
            return;
        std::pair<rmap_iterator, rmap_iterator> itPair = rmap.equal_range(itTarget->second);
        for (rmap_iterator it = itPair.first; it != itPair.second; ++it)
            if (it->second == itTarget) {
                rmap.erase(it);
                map.erase(itTarget);
                return;
            }
        // Shouldn't ever get here
        assert(0);
    }
    void update(const_iterator itIn, const mapped_type& v)
    {
        // Using map::erase() with empty range instead of map::find() to get a non-const iterator,
        // since it is a constant time operation in C++11. For more details, see
        // https://stackoverflow.com/questions/765148/how-to-remove-constness-of-const-iterator
        iterator itTarget = map.erase(itIn, itIn);
        
        if (itTarget == map.end())
            return;
        std::pair<rmap_iterator, rmap_iterator> itPair = rmap.equal_range(itTarget->second);
        for (rmap_iterator it = itPair.first; it != itPair.second; ++it)
            if (it->second == itTarget) {
                rmap.erase(it);
                itTarget->second = v;
                rmap.insert(make_pair(v, itTarget));
                return;
            }
        // Shouldn't ever get here
        assert(0);
    }
    size_type max_size() const { return nMaxSize; }
    size_type max_size(size_type nMaxSizeIn, size_type nPruneAfterSizeIn = 0)
    {
        assert(nMaxSizeIn > 0);
        nMaxSize = nMaxSizeIn;
        if (nPruneAfterSizeIn == 0) {
            nPruneAfterSize = nMaxSize;
        } else {
            nPruneAfterSize = nPruneAfterSizeIn;
        }
        assert(nPruneAfterSize >= nMaxSize);
        prune();
        return nMaxSize;
    }
    void prune()
    {
        if (map.size() <= nPruneAfterSize) {
            return;
        }

        std::vector<rmap_iterator> sortedIterators;
        sortedIterators.reserve(map.size());
        for (auto it = rmap.begin(); it != rmap.end(); ++it) {
            sortedIterators.emplace_back(it);
        }
        std::sort(sortedIterators.begin(), sortedIterators.end(), [](const rmap_iterator& it1, const rmap_iterator& it2) {
            return it1->first < it2->first;
        });

        size_type tooMuch = map.size() - nMaxSize;
        assert(tooMuch > 0);
        sortedIterators.resize(tooMuch);

        for (auto& it : sortedIterators) {
            map.erase(it->second);
            rmap.erase(it);
        }
    }
};

#endif // BITCOIN_LIMITEDMAP_H
