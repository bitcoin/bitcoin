// Copyright (c) 2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMUSAGE_H
#define BITCOIN_MEMUSAGE_H

#include <stdlib.h>

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

namespace memusage
{
/** Compute the total memory used by allocating alloc bytes. */
static size_t MallocUsage(size_t alloc);

/** Dynamic memory usage for built-in types is zero. */
static inline size_t DynamicUsage(const int8_t &v) { return 0; }
static inline size_t DynamicUsage(const uint8_t &v) { return 0; }
static inline size_t DynamicUsage(const int16_t &v) { return 0; }
static inline size_t DynamicUsage(const uint16_t &v) { return 0; }
static inline size_t DynamicUsage(const int32_t &v) { return 0; }
static inline size_t DynamicUsage(const uint32_t &v) { return 0; }
static inline size_t DynamicUsage(const int64_t &v) { return 0; }
static inline size_t DynamicUsage(const uint64_t &v) { return 0; }
static inline size_t DynamicUsage(const float &v) { return 0; }
static inline size_t DynamicUsage(const double &v) { return 0; }
template <typename X>
static inline size_t DynamicUsage(X *const &v)
{
    return 0;
}
template <typename X>
static inline size_t DynamicUsage(const X *const &v)
{
    return 0;
}

/** Compute the memory used for dynamically allocated but owned data structures.
 *  For generic data types, this is *not* recursive. DynamicUsage(vector<vector<int> >)
 *  will compute the memory used for the vector<int>'s, but not for the ints inside.
 *  This is for efficiency reasons, as these functions are intended to be fast. If
 *  application data structures require more accurate inner accounting, they should
 *  iterate themselves, or use more efficient caching + updating on modification.
 */

static inline size_t MallocUsage(size_t alloc)
{
    // Measured on libc6 2.19 on Linux.
    if (alloc == 0)
    {
        return 0;
    }
    else if (sizeof(void *) == 8)
    {
        return ((alloc + 31) >> 4) << 4;
    }
    else if (sizeof(void *) == 4)
    {
        return ((alloc + 15) >> 3) << 3;
    }
    else
    {
        assert(0);
    }
}

// STL data structures

template <typename X>
struct stl_tree_node
{
private:
    int color;
    void *parent;
    void *left;
    void *right;
    X x;
};

template <typename X>
static inline size_t DynamicUsage(const std::vector<X> &v)
{
    return MallocUsage(v.capacity() * sizeof(X));
}

template <unsigned int N, typename X, typename S, typename D>
static inline size_t DynamicUsage(const prevector<N, X, S, D> &v)
{
    return MallocUsage(v.allocated_memory());
}

template <typename X, typename Y>
static inline size_t DynamicUsage(const std::set<X, Y> &s)
{
    return MallocUsage(sizeof(stl_tree_node<X>)) * s.size();
}

template <typename X, typename Y>
static inline size_t IncrementalDynamicUsage(const std::set<X, Y> &s)
{
    return MallocUsage(sizeof(stl_tree_node<X>));
}

template <typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::map<X, Y, Z> &m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >)) * m.size();
}

template <typename X, typename Y, typename Z>
static inline size_t IncrementalDynamicUsage(const std::map<X, Y, Z> &m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >));
}

// Boost data structures

template <typename X>
struct boost_unordered_node : private X
{
private:
    void *ptr;
};

template <typename X>
struct unordered_node : private X
{
private:
    void *ptr;
};

template <typename X, typename Y>
static inline size_t DynamicUsage(const boost::unordered_set<X, Y> &s)
{
    return MallocUsage(sizeof(boost_unordered_node<X>)) * s.size() + MallocUsage(sizeof(void *) * s.bucket_count());
}

template <typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const boost::unordered_map<X, Y, Z> &m)
{
    return MallocUsage(sizeof(boost_unordered_node<std::pair<const X, Y> >)) * m.size() +
           MallocUsage(sizeof(void *) * m.bucket_count());
}

template <typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::unordered_map<X, Y, Z> &m)
{
    return MallocUsage(sizeof(unordered_node<std::pair<const X, Y> >)) * m.size() +
           MallocUsage(sizeof(void *) * m.bucket_count());
}
}

#endif // BITCOIN_MEMUSAGE_H
