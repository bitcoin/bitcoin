// Copyright (c) 2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMUSAGE_H
#define BITCOIN_MEMUSAGE_H

#include <stdlib.h>

#include <map>
#include <set>
#include <vector>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

namespace memusage
{

/** Compute the total memory used by allocating alloc bytes. */
static size_t MallocUsage(size_t alloc);

/** Compute the memory used for dynamically allocated but owned data structures.
 *  For generic data types, this is *not* recursive. DynamicUsage(vector<vector<int> >)
 *  will compute the memory used for the vector<int>'s, but not for the ints inside.
 *  This is for efficiency reasons, as these functions are intended to be fast. If
 *  application data structures require more accurate inner accounting, they should
 *  do the recursion themselves, or use more efficient caching + updating on modification.
 */
template<typename X> static size_t DynamicUsage(const std::vector<X>& v);
template<typename X> static size_t DynamicUsage(const std::set<X>& s);
template<typename X, typename Y> static size_t DynamicUsage(const std::map<X, Y>& m);
template<typename X, typename Y> static size_t DynamicUsage(const boost::unordered_set<X, Y>& s);
template<typename X, typename Y, typename Z> static size_t DynamicUsage(const boost::unordered_map<X, Y, Z>& s);
template<typename X> static size_t DynamicUsage(const X& x);

static inline size_t MallocUsage(size_t alloc)
{
    // Measured on libc6 2.19 on Linux.
    if (sizeof(void*) == 8) {
        return ((alloc + 31) >> 4) << 4;
    } else if (sizeof(void*) == 4) {
        return ((alloc + 15) >> 3) << 3;
    } else {
        assert(0);
    }
}

// STL data structures

template<typename X>
struct stl_tree_node
{
private:
    int color;
    void* parent;
    void* left;
    void* right;
    X x;
};

template<typename X>
static inline size_t DynamicUsage(const std::vector<X>& v)
{
    return MallocUsage(v.capacity() * sizeof(X));
}

template<typename X>
static inline size_t DynamicUsage(const std::set<X>& s)
{
    return MallocUsage(sizeof(stl_tree_node<X>)) * s.size();
}

template<typename X, typename Y>
static inline size_t DynamicUsage(const std::map<X, Y>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >)) * m.size();
}

// Boost data structures

template<typename X>
struct boost_unordered_node : private X
{
private:
    void* ptr;
};

template<typename X, typename Y>
static inline size_t DynamicUsage(const boost::unordered_set<X, Y>& s)
{
    return MallocUsage(sizeof(boost_unordered_node<X>)) * s.size() + MallocUsage(sizeof(void*) * s.bucket_count());
}

template<typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const boost::unordered_map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(boost_unordered_node<std::pair<const X, Y> >)) * m.size() + MallocUsage(sizeof(void*) * m.bucket_count());
}

// Dispatch to class method as fallback

template<typename X>
static inline size_t DynamicUsage(const X& x)
{
    return x.DynamicMemoryUsage();
}

}

#endif
