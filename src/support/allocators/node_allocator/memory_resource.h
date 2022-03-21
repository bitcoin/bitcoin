// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_MEMORY_RESOURCE_H
#define BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_MEMORY_RESOURCE_H

namespace node_allocator {

namespace detail {

/**
 * In-place linked list of the allocations, used for the free list.
 */
struct FreeList {
    FreeList* next;
};

static_assert(std::is_trivially_destructible_v<FreeList>, "make sure we don't need to manually destruct the FreeList");

/**
 * Calculates the required allocation size for the given type.
 * The memory needs to be correctly aligned and large enough for both both T and FreeList.
 */
template <typename T>
[[nodiscard]] constexpr size_t RequiredAllocationSizeBytes() noexcept
{
    const auto alignment_max = std::max(std::alignment_of_v<T>, std::alignment_of_v<FreeList>);
    const auto size_max = std::max(sizeof(T), sizeof(FreeList));

    // find closest multiple of alignment_max that holds size_max
    return ((size_max + alignment_max - 1U) / alignment_max) * alignment_max;
}

/**
 * Calculates dynamic memory usage of a MemoryResource. Not a member of MemoryResource so we can implement this
 * in a cpp and don't depend on memusage.h in the header.
 */
[[nodiscard]] size_t DynamicMemoryUsage(size_t pool_size_bytes, std::vector<std::unique_ptr<char[]>> const& allocated_pools);

} // namespace detail

/**
 * Actually holds and provides memory to an allocator. MemoryResource is an immobile object. It
 * stores a number of memory pools which are used to quickly give out memory of a fixed allocation
 * size. The class is purposely kept very simple. It only knows about "Allocations" and "Pools".
 *
 * - Pool: MemoryResource allocates one memory pool at a time. These pools are kept around until the
 * memory resource is destroyed.
 *
 * - Allocations: Node-based containers allocate one node at a time. Whenever that happens, the
 * MemoryResource's Allocate() gives out memory for one node. These are carved out from a previously
 * allocated memory pool, or from a free list if it contains entries. Whenever a node is given back
 * with Deallocate(), it is put into that free list.
 */
template <size_t ALLOCATION_SIZE_BYTES>
class MemoryResource
{
    /**
     * Size in bytes to allocate per pool, currently hardcoded to multiple of ALLOCATION_SIZE_BYTES
     * that comes closest to 256 KiB.
     */
    static constexpr size_t POOL_SIZE_BYTES = (262144 / ALLOCATION_SIZE_BYTES) * ALLOCATION_SIZE_BYTES;

public:
    /**
     * Construct a new MemoryResource object that uses the specified allocation size to optimize for.
     */
    MemoryResource() = default;

    /**
     * Copying/moving a memory resource is not allowed; it is an immobile object.
     */
    MemoryResource(const MemoryResource&) = delete;
    MemoryResource& operator=(const MemoryResource&) = delete;
    MemoryResource(MemoryResource&&) = delete;
    MemoryResource& operator=(MemoryResource&&) = delete;

    /**
     * Deallocates all allocated pools.
     *
     * There's no Clear() method on purpose, because it would be dangerous. E.g. when calling
     * clear() on an unordered_map, it is not certain that all allocated nodes are given back to the
     * MemoryResource. Microsoft's STL still uses a control structure that might have the same size
     * as the nodes, and therefore needs to be kept around until the map is actually destroyed.
     */
    ~MemoryResource() = default;

    /**
     * Allocates memory for sizeof(T).
     *
     * @tparam T Object to allocate memory for.
     */
    template <typename T>
    [[nodiscard]] T* Allocate()
    {
        static_assert(ALLOCATION_SIZE_BYTES == detail::RequiredAllocationSizeBytes<T>());
        static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= std::alignment_of_v<T>, "make sure AllocatePool() aligns correctly");

        if (m_free_allocations) {
            // we've already got data in the free list, unlink one element
            auto old_head = m_free_allocations;
            m_free_allocations = m_free_allocations->next;
            return reinterpret_cast<T*>(old_head);
        }

        // free list is empty: get one allocation from allocated pool memory.
        // It makes sense to not create the fully linked list of an allocated pool up-front, for
        // several reasons. On the one hand, the latency is higher when we need to iterate and
        // update pointers for the whole pool at once. More importantly, most systems lazily
        // allocate data. So when we allocate a big pool of memory the memory for a page is only
        // actually made available to the program when it is first touched. So when we allocate
        // a big pool and only use very little memory from it, the total memory usage is lower
        // than what has been malloc'ed.
        if (m_untouched_memory_iterator == m_untouched_memory_end) {
            // slow path, only happens when a new pool needs to be allocated
            AllocatePool();
        }

        // peel off one allocation from the untouched memory. The next pointer of in-use
        // elements doesn't matter until it is deallocated, only then it is used to form the
        // free list.
        char* tmp = m_untouched_memory_iterator;
        m_untouched_memory_iterator = tmp + ALLOCATION_SIZE_BYTES;
        return reinterpret_cast<T*>(tmp);
    }

    /**
     * Puts p back into the free list.
     */
    template <typename T>
    void Deallocate(void* p) noexcept
    {
        static_assert(ALLOCATION_SIZE_BYTES == detail::RequiredAllocationSizeBytes<T>());

        // put it into the linked list.
        //
        // Note: We can't just static_cast<detail::FreeList*>(p) because this is technically
        // undefined behavior. But we can use placement new instead. Correct alignment is
        // guaranteed by the static_asserts in Allocate().
        auto* const allocation = new (p) detail::FreeList;
        allocation->next = m_free_allocations;
        m_free_allocations = allocation;
    }

    /**
     * Calculates bytes allocated by the memory resource.
     */
    [[nodiscard]] size_t DynamicMemoryUsage() const noexcept
    {
        return detail::DynamicMemoryUsage(POOL_SIZE_BYTES, m_allocated_pools);
    }

private:
    //! Access to internals for testing purpose only
    friend class MemoryResourceTester;

    /**
     * Allocate one full memory pool which is used to carve out allocations.
     */
    void AllocatePool()
    {
        m_allocated_pools.emplace_back(new char[POOL_SIZE_BYTES]);
        m_untouched_memory_iterator = m_allocated_pools.back().get();
        m_untouched_memory_end = m_untouched_memory_iterator + POOL_SIZE_BYTES;
    }

    //! Contains all allocated pools of memory, used to free the data in the destructor.
    std::vector<std::unique_ptr<char[]>> m_allocated_pools{};

    //! A single linked list of all data available in the MemoryResource. This list is used for
    //! allocations of single elements.
    detail::FreeList* m_free_allocations = nullptr;

    //! Points to the beginning of available memory for carving out allocations.
    char* m_untouched_memory_iterator = nullptr;

    /**
     * Points to the end of available memory for carving out allocations.
     *
     * That member variable is redundant, and is always equal to
     * `m_allocated_pools.back().get() + POOL_SIZE_BYTES` whenever it is accessed, but
     * `m_untouched_memory_end` caches this for clarity and efficiency.
     */
    char* m_untouched_memory_end = nullptr;
};

} // namespace node_allocator

#endif // BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_MEMORY_RESOURCE_H
