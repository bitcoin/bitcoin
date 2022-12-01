// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMUSAGE_H
#define BITCOIN_MEMUSAGE_H

#include <indirectmap.h>
#include <prevector.h>

#include <cassert>
#include <cstdlib>
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

/** Wrapping allocator that accounts accurately.
 *
 * To use accounting, the AccountAllocator(size_t&) constructor must be used.
 * Any containers using such an allocator, and containers move constructed or
 * move assigned from such containers will then increment/decrement size_t when
 * allocating/deallocating memory.
 */
template <typename T>
class AccountingAllocator
{
    using base = std::allocator<T>;

    //! Base allocator
    base m_base;

    //! Pointer to accounting variable, if any.
    size_t* m_allocated = nullptr;

    template <typename U>
    friend class AccountingAllocator;

public:
    using value_type = typename base::value_type;
    using size_type = typename base::size_type;
    using difference_type = typename base::size_type;

    //! Default constructor constructs a non-accounting allocator.
    AccountingAllocator() = default;

    /** Construct an allocator that increments/decrements 'allocated' on allocate/free.
     *
     * In a multithreaded environment, the accounting variable needs to be
     * protected by the same lock as the container(s) that use this allocator.
     */
#if __cplusplus >= 202002L
    constexpr
#endif
    explicit AccountingAllocator(size_t& allocated) noexcept : m_allocated{&allocated} {}

    //! A copy-constructed container will be non-accounting.
    AccountingAllocator select_on_container_copy_construction() const { return {}; }
    //! A copy-assigned container will be non-accounting.
    using propagate_on_container_copy_assignment = std::false_type;
    //! The accounting will follow a container as it's moved.
    using propagate_on_container_move_assignment = std::true_type;
    //! The accounting will follow a container as it's swapped.
    using propagate_on_container_swap = std::true_type;

    using is_always_equal = std::false_type;

    // Construct an allocator for a different data type, inheriting the accounting.
    template <typename U>
    AccountingAllocator(AccountingAllocator<U>&& a) noexcept : m_base(std::move(a.m_base)), m_allocated(a.m_allocated) {}
    template <typename U>
    AccountingAllocator(const AccountingAllocator<U>& a) noexcept : m_base(a.m_base), m_allocated(a.m_allocated) {}

#if __cplusplus >= 202002L
    [[nodiscard]] constexpr
#endif
    value_type* allocate(std::size_t n)
    {
        if (m_allocated) *m_allocated += MallocUsage(sizeof(value_type) * n);
        return m_base.allocate(n);
    }

#if __cplusplus >= 202002L
    constexpr
#endif
    void deallocate(typename base::value_type* p, std::size_t n)
    {
        m_base.deallocate(p, n);
        if (m_allocated) *m_allocated -= MallocUsage(sizeof(value_type) * n);
    }

    template <typename U>
    struct rebind {
        typedef AccountingAllocator<U> other;
    };

    friend bool operator==(const AccountingAllocator<T>& a, const AccountingAllocator<T>& b) noexcept { return a.m_allocated == b.m_allocated; }
    friend bool operator!=(const AccountingAllocator<T>& a, const AccountingAllocator<T>& b) noexcept { return a.m_allocated != b.m_allocated; }
};

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
    // Measured on libc6 2.19 on Linux.
    if (alloc == 0) {
        return 0;
    } else if (sizeof(void*) == 8) {
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
struct unordered_node : private X
{
private:
    void* ptr;
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

}

#endif // BITCOIN_MEMUSAGE_H
