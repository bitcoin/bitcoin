// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMUSAGE_H
#define BITCOIN_MEMUSAGE_H

#include <indirectmap.h>
#include <prevector.h>
#include <support/allocators/pool.h>

#include <cassert>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>


namespace memusage
{

/** Compute the total memory used by allocating alloc bytes. */
static size_t MallocUsage(size_t alloc);

/** Dynamic memory usage for built-in types is zero. */
static inline size_t DynamicUsage(const int8_t& v) { return 0; }
static inline size_t DynamicUsage(const uint8_t& v) { return 0; }
static inline size_t DynamicUsage(const int16_t& v) { return 0; }
static inline size_t DynamicUsage(const uint16_t& v) { return 0; }
static inline size_t DynamicUsage(const int32_t& v) { return 0; }
static inline size_t DynamicUsage(const uint32_t& v) { return 0; }
static inline size_t DynamicUsage(const int64_t& v) { return 0; }
static inline size_t DynamicUsage(const uint64_t& v) { return 0; }
static inline size_t DynamicUsage(const float& v) { return 0; }
static inline size_t DynamicUsage(const double& v) { return 0; }
template<typename X> static inline size_t DynamicUsage(X * const &v) { return 0; }
template<typename X> static inline size_t DynamicUsage(const X * const &v) { return 0; }

/** Compute the memory used for dynamically allocated but owned data structures.
 *  For generic data types, this is *not* recursive. DynamicUsage(vector<vector<int> >)
 *  will compute the memory used for the vector<int>'s, but not for the ints inside.
 *  This is for efficiency reasons, as these functions are intended to be fast. If
 *  application data structures require more accurate inner accounting, they should
 *  iterate themselves, or use more efficient caching + updating on modification.
 */
static inline size_t MallocUsage(size_t alloc)
{
    // There are few if any actual zero-length allocations; when
    // DynamicUsage(std::vector<X>& v) calls this function, and v.capacity() == 0,
    // for example, there has not been a zero-byte allocation (which would require
    // some physical space). std::vector has optimized this case. Experimental
    // evidence indicates the same is true of other data structures -- there are
    // no actual zero-length allocations observed, although of course this is
    // library-dependent.
    if (alloc == 0) return 0;

    static constexpr size_t overhead = sizeof(void*);
#if defined(__arm__)
    // tested with ARM 32bit
    static constexpr size_t step = sizeof(void*) * 2;
    static constexpr size_t min_alloc = 9;
#else
    static constexpr size_t step = 16U;
    static constexpr size_t min_alloc = sizeof(void*) == 8 ? 9 : 0;
#endif
    // step should be nonzero and a power of 2
    static_assert(step > 0);
    static_assert((step & (step-1)) == 0);
    // tested with Linux glibc 2.31 and 2.38, 32bit and 64bit
    return (std::max(min_alloc, alloc) + overhead + (step - 1)) & ~(step - 1);
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

struct stl_shared_counter
{
    /* Various platforms use different sized counters here.
     * Conservatively assume that they won't be larger than size_t. */
    void* class_type;
    size_t use_count;
    size_t weak_count;
};

template<typename X>
static inline size_t DynamicUsage(const std::vector<X>& v)
{
    return MallocUsage(v.capacity() * sizeof(X));
}

template<unsigned int N, typename X, typename S, typename D>
static inline size_t DynamicUsage(const prevector<N, X, S, D>& v)
{
    return MallocUsage(v.allocated_memory());
}

template<typename X, typename Y>
static inline size_t DynamicUsage(const std::set<X, Y>& s)
{
    return MallocUsage(sizeof(stl_tree_node<X>)) * s.size();
}

template<typename X, typename Y>
static inline size_t IncrementalDynamicUsage(const std::set<X, Y>& s)
{
    return MallocUsage(sizeof(stl_tree_node<X>));
}

template<typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >)) * m.size();
}

template<typename X, typename Y, typename Z>
static inline size_t IncrementalDynamicUsage(const std::map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >));
}

// indirectmap has underlying map with pointer as key

template<typename X, typename Y>
static inline size_t DynamicUsage(const indirectmap<X, Y>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X*, Y> >)) * m.size();
}

template<typename X, typename Y>
static inline size_t IncrementalDynamicUsage(const indirectmap<X, Y>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X*, Y> >));
}

template<typename X>
static inline size_t DynamicUsage(const std::unique_ptr<X>& p)
{
    return p ? MallocUsage(sizeof(X)) : 0;
}

template<typename X>
static inline size_t DynamicUsage(const std::shared_ptr<X>& p)
{
    // A shared_ptr can either use a single continuous memory block for both
    // the counter and the storage (when using std::make_shared), or separate.
    // We can't observe the difference, however, so assume the worst.
    return p ? MallocUsage(sizeof(X)) + MallocUsage(sizeof(stl_shared_counter)) : 0;
}

template<typename X>
struct list_node
{
private:
    void* ptr_next;
    void* ptr_prev;
    X x;
};

template<typename X>
static inline size_t DynamicUsage(const std::list<X>& l)
{
    return MallocUsage(sizeof(list_node<X>)) * l.size();
}

// struct unordered_node adds the container overhead to the given structure X,
// although this varies across library container implementations (platforms).
// It is believed that some containers use two pointers per node, so this
// generates a conservative memory usage estimate (it may be slightly too high).
template<typename X>
struct unordered_node : private X
{
private:
    void* ptr_back;
    void* ptr_forward;
};

template<typename X, typename Y>
static inline size_t DynamicUsage(const std::unordered_set<X, Y>& s)
{
    return MallocUsage(sizeof(unordered_node<X>)) * s.size() + MallocUsage(sizeof(void*) * s.bucket_count());
}

template<typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::unordered_map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(unordered_node<std::pair<const X, Y> >)) * m.size() + MallocUsage(sizeof(void*) * m.bucket_count());
}

template <class Key, class T, class Hash, class Pred, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
static inline size_t DynamicUsage(const std::unordered_map<Key,
                                                           T,
                                                           Hash,
                                                           Pred,
                                                           PoolAllocator<std::pair<const Key, T>,
                                                                         MAX_BLOCK_SIZE_BYTES,
                                                                         ALIGN_BYTES>>& m)
{
    auto* pool_resource = m.get_allocator().resource();

    // The allocated chunks are stored in a std::list. Size per node should
    // therefore be 3 pointers: next, previous, and a pointer to the chunk.
    size_t estimated_list_node_size = MallocUsage(sizeof(void*) * 3);
    size_t usage_resource = estimated_list_node_size * pool_resource->NumAllocatedChunks();
    size_t usage_chunks = MallocUsage(pool_resource->ChunkSizeBytes()) * pool_resource->NumAllocatedChunks();
    return usage_resource + usage_chunks + MallocUsage(sizeof(void*) * m.bucket_count());
}

} // namespace memusage

#endif // BITCOIN_MEMUSAGE_H
