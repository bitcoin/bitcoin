// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MRUSET_H
#define BITCOIN_MRUSET_H

#include <set>
#include <vector>
#include <utility>

/** STL-like set container that only keeps the most recent N elements. */
template <typename T>
class mruset
{
public:
    typedef T key_type;
    typedef T value_type;
    typedef typename std::set<T>::iterator iterator;
    typedef typename std::set<T>::const_iterator const_iterator;
    typedef typename std::set<T>::size_type size_type;

protected:
    std::set<T> set;
    std::vector<iterator> order;
    size_type first_used;
    size_type first_unused;
    const size_type nMaxSize;

public:
    mruset(size_type nMaxSizeIn = 1) : nMaxSize(nMaxSizeIn) { clear(); }
    iterator begin() const { return set.begin(); }
    iterator end() const { return set.end(); }
    size_type size() const { return set.size(); }
    bool empty() const { return set.empty(); }
    iterator find(const key_type& k) const { return set.find(k); }
    size_type count(const key_type& k) const { return set.count(k); }
    void clear()
    {
        set.clear();
        order.assign(nMaxSize, set.end());
        first_used = 0;
        first_unused = 0;
    }
    bool inline friend operator==(const mruset<T>& a, const mruset<T>& b) { return a.set == b.set; }
    bool inline friend operator==(const mruset<T>& a, const std::set<T>& b) { return a.set == b; }
    bool inline friend operator<(const mruset<T>& a, const mruset<T>& b) { return a.set < b.set; }
    std::pair<iterator, bool> insert(const key_type& x)
    {
        std::pair<iterator, bool> ret = set.insert(x);
        if (ret.second) {
            if (set.size() == nMaxSize + 1) {
                set.erase(order[first_used]);
                order[first_used] = set.end();
                if (++first_used == nMaxSize) first_used = 0;
            }
            order[first_unused] = ret.first;
            if (++first_unused == nMaxSize) first_unused = 0;
        }
        return ret;
    }
    size_type max_size() const { return nMaxSize; }
};

#endif // BITCOIN_MRUSET_H
