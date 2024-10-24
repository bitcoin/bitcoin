// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_POOL_H
#define BITCOIN_SUPPORT_ALLOCATORS_POOL_H

#include <array>
#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/**
 * A memory resource similar to std::pmr::unsynchronized_pool_resource, but
 * optimized for node-based containers. It has the following properties:
 *
 * * Owns the allocated memory and frees it on destruction, even when deallocate
 *   has not been called on the allocated blocks.
 *
 * * Consists of a number of pools, each one for a different block size.
 *   Each pool holds blocks of uniform size in a freelist.
 *
 * * Exhausting memory in a freelist causes a new allocation of a fixed size chunk.
 *   This chunk is used to carve out blocks.
 *
 * * Block sizes or alignments that can not be served by the pools are allocated
 *   and deallocated by operator new().
 *
 * PoolResource is not thread-safe. It is intended to be used by PoolAllocator.
 *
 * @tparam MAX_BLOCK_SIZE_BYTES Maximum size to allocate with the pool. If larger
 *         sizes are requested, allocation falls back to new().
 *
 * @tparam ALIGN_BYTES Required alignment for the allocations.
 *
 * An example: If you create a PoolResource<128, 8>(262144) and perform a bunch of
 * allocations and deallocate 2 blocks with size 8 bytes, and 3 blocks with size 16,
 * the members will look like this:
 *
 *     m_free_lists                         m_allocated_chunks
 *        ┌───┐                                ┌───┐  ┌────────────-------──────┐
 *        │   │  blocks                        │   ├─►│    262144 B             │
 *        │   │  ┌─────┐  ┌─────┐              └─┬─┘  └────────────-------──────┘
 *        │ 1 ├─►│ 8 B ├─►│ 8 B │                │
 *        │   │  └─────┘  └─────┘                :
 *        │   │                                  │
 *        │   │  ┌─────┐  ┌─────┐  ┌─────┐       ▼
 *        │ 2 ├─►│16 B ├─►│16 B ├─►│16 B │     ┌───┐  ┌─────────────────────────┐
 *        │   │  └─────┘  └─────┘  └─────┘     │   ├─►│          ▲              │ ▲
 *        │   │                                └───┘  └──────────┬──────────────┘ │
 *        │ . │                                                  │    m_available_memory_end
 *        │ . │                                         m_available_memory_it
 *        │ . │
 *        │   │
 *        │   │
 *        │16 │
 *        └───┘
 *
 * Here m_free_lists[1] holds the 2 blocks of size 8 bytes, and m_free_lists[2]
 * holds the 3 blocks of size 16. The blocks came from the data stored in the
 * m_allocated_chunks list. Each chunk has bytes 262144. The last chunk has still
 * some memory available for the blocks, and when m_available_memory_it is at the
 * end, a new chunk will be allocated and added to the list.
 */
template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
class PoolResource final
{
    static_assert(ALIGN_BYTES > 0, "ALIGN_BYTES must be nonzero");
    static_assert((ALIGN_BYTES & (ALIGN_BYTES - 1)) == 0, "ALIGN_BYTES must be a power of two");

    /**
     * In-place linked list of the allocations, used for the freelist.
     */
    struct ListNode {
        ListNode* m_next;

        explicit ListNode(ListNode* next) : m_next(next) {}
    };
    static_assert(std::is_trivially_destructible_v<ListNode>, "Make sure we don't need to manually call a destructor");

    /**
     * Internal alignment value. The larger of the requested ALIGN_BYTES and alignof(FreeList).
     */
    static constexpr std::size_t ELEM_ALIGN_BYTES = std::max(alignof(ListNode), ALIGN_BYTES);
    static_assert((ELEM_ALIGN_BYTES & (ELEM_ALIGN_BYTES - 1)) == 0, "ELEM_ALIGN_BYTES must be a power of two");
    static_assert(sizeof(ListNode) <= ELEM_ALIGN_BYTES, "Units of size ELEM_SIZE_ALIGN need to be able to store a ListNode");
    static_assert((MAX_BLOCK_SIZE_BYTES & (ELEM_ALIGN_BYTES - 1)) == 0, "MAX_BLOCK_SIZE_BYTES needs to be a multiple of the alignment.");

    /**
     * Size in bytes to allocate per chunk
     */
    const size_t m_chunk_size_bytes;

    /**
     * Contains all allocated pools of memory, used to free the data in the destructor.
     */
    std::list<std::byte*> m_allocated_chunks{};

    /**
     * Single linked lists of all data that came from deallocating.
     * m_free_lists[n] will serve blocks of size n*ELEM_ALIGN_BYTES.
     */
    std::array<ListNode*, MAX_BLOCK_SIZE_BYTES / ELEM_ALIGN_BYTES + 1> m_free_lists{};

    /**
     * Points to the beginning of available memory for carving out allocations.
     */
    std::byte* m_available_memory_it = nullptr;

    /**
     * Points to the end of available memory for carving out allocations.
     *
     * That member variable is redundant, and is always equal to `m_allocated_chunks.back() + m_chunk_size_bytes`
     * whenever it is accessed, but `m_available_memory_end` caches this for clarity and efficiency.
     */
    std::byte* m_available_memory_end = nullptr;

    /**
     * How many multiple of ELEM_ALIGN_BYTES are necessary to fit bytes. We use that result directly as an index
     * into m_free_lists. Round up for the special case when bytes==0.
     */
    [[nodiscard]] static constexpr std::size_t NumElemAlignBytes(std::size_t bytes)
    {
        return (bytes + ELEM_ALIGN_BYTES - 1) / ELEM_ALIGN_BYTES + (bytes == 0);
    }

    /**
     * True when it is possible to make use of the freelist
     */
    [[nodiscard]] static constexpr bool IsFreeListUsable(std::size_t bytes, std::size_t alignment)
    {
        return alignment <= ELEM_ALIGN_BYTES && bytes <= MAX_BLOCK_SIZE_BYTES;
    }

    /**
     * Replaces node with placement constructed ListNode that points to the previous node
     */
    void PlacementAddToList(void* p, ListNode*& node)
    {
        node = new (p) ListNode{node};
    }

    /**
     * Allocate one full memory chunk which will be used to carve out allocations.
     * Also puts any leftover bytes into the freelist.
     *
     * Precondition: leftover bytes are either 0 or few enough to fit into a place in the freelist
     */
    void AllocateChunk()
    {
        // if there is still any available memory left, put it into the freelist.
        size_t remaining_available_bytes = std::distance(m_available_memory_it, m_available_memory_end);
        if (0 != remaining_available_bytes) {
            PlacementAddToList(m_available_memory_it, m_free_lists[remaining_available_bytes / ELEM_ALIGN_BYTES]);
        }

        void* storage = ::operator new (m_chunk_size_bytes, std::align_val_t{ELEM_ALIGN_BYTES});
        m_available_memory_it = new (storage) std::byte[m_chunk_size_bytes];
        m_available_memory_end = m_available_memory_it + m_chunk_size_bytes;
        m_allocated_chunks.emplace_back(m_available_memory_it);
    }

    /**
     * Access to internals for testing purpose only
     */
    friend class PoolResourceTester;

public:
    /**
     * Construct a new PoolResource object which allocates the first chunk.
     * chunk_size_bytes will be rounded up to next multiple of ELEM_ALIGN_BYTES.
     */
    explicit PoolResource(std::size_t chunk_size_bytes)
        : m_chunk_size_bytes(NumElemAlignBytes(chunk_size_bytes) * ELEM_ALIGN_BYTES)
    {
        assert(m_chunk_size_bytes >= MAX_BLOCK_SIZE_BYTES);
        AllocateChunk();
    }

    /**
     * Construct a new Pool Resource object, defaults to 2^18=262144 chunk size.
     */
    PoolResource() : PoolResource(262144) {}

    /**
     * Disable copy & move semantics, these are not supported for the resource.
     */
    PoolResource(const PoolResource&) = delete;
    PoolResource& operator=(const PoolResource&) = delete;
    PoolResource(PoolResource&&) = delete;
    PoolResource& operator=(PoolResource&&) = delete;

    /**
     * Deallocates all memory allocated associated with the memory resource.
     */
    ~PoolResource()
    {
        for (std::byte* chunk : m_allocated_chunks) {
            std::destroy(chunk, chunk + m_chunk_size_bytes);
            ::operator delete ((void*)chunk, std::align_val_t{ELEM_ALIGN_BYTES});
        }
    }

    /**
     * Allocates a block of bytes. If possible the freelist is used, otherwise allocation
     * is forwarded to ::operator new().
     */
    void* Allocate(std::size_t bytes, std::size_t alignment)
    {
        if (IsFreeListUsable(bytes, alignment)) {
            const std::size_t num_alignments = NumElemAlignBytes(bytes);
            if (nullptr != m_free_lists[num_alignments]) {
                // we've already got data in the pool's freelist, unlink one element and return the pointer
                // to the unlinked memory. Since FreeList is trivially destructible we can just treat it as
                // uninitialized memory.
                return std::exchange(m_free_lists[num_alignments], m_free_lists[num_alignments]->m_next);
            }

            // freelist is empty: get one allocation from allocated chunk memory.
            const std::ptrdiff_t round_bytes = static_cast<std::ptrdiff_t>(num_alignments * ELEM_ALIGN_BYTES);
            if (round_bytes > m_available_memory_end - m_available_memory_it) {
                // slow path, only happens when a new chunk needs to be allocated
                AllocateChunk();
            }

            // Make sure we use the right amount of bytes for that freelist (might be rounded up),
            return std::exchange(m_available_memory_it, m_available_memory_it + round_bytes);
        }

        // Can't use the pool => use operator new()
        return ::operator new (bytes, std::align_val_t{alignment});
    }

    /**
     * Returns a block to the freelists, or deletes the block when it did not come from the chunks.
     */
    void Deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept
    {
        if (IsFreeListUsable(bytes, alignment)) {
            const std::size_t num_alignments = NumElemAlignBytes(bytes);
            // put the memory block into the linked list. We can placement construct the FreeList
            // into the memory since we can be sure the alignment is correct.
            PlacementAddToList(p, m_free_lists[num_alignments]);
        } else {
            // Can't use the pool => forward deallocation to ::operator delete().
            ::operator delete (p, std::align_val_t{alignment});
        }
    }

    /**
     * Number of allocated chunks
     */
    [[nodiscard]] std::size_t NumAllocatedChunks() const
    {
        return m_allocated_chunks.size();
    }

    /**
     * Size in bytes to allocate per chunk, currently hardcoded to a fixed size.
     */
    [[nodiscard]] size_t ChunkSizeBytes() const
    {
        return m_chunk_size_bytes;
    }
};


/**
 * Forwards all allocations/deallocations to the PoolResource.
 */
template <class T, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES = alignof(T)>
class PoolAllocator
{
    PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>* m_resource;

    template <typename U, std::size_t M, std::size_t A>
    friend class PoolAllocator;

public:
    using value_type = T;
    using ResourceType = PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>;

    /**
     * Not explicit so we can easily construct it with the correct resource
     */
    PoolAllocator(ResourceType* resource) noexcept
        : m_resource(resource)
    {
    }

    PoolAllocator(const PoolAllocator& other) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator& other) noexcept = default;

    template <class U>
    PoolAllocator(const PoolAllocator<U, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& other) noexcept
        : m_resource(other.resource())
    {
    }

    /**
     * The rebind struct here is mandatory because we use non type template arguments for
     * PoolAllocator. See https://en.cppreference.com/w/cpp/named_req/Allocator#cite_note-2
     */
    template <typename U>
    struct rebind {
        using other = PoolAllocator<U, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>;
    };

    /**
     * Forwards each call to the resource.
     */
    T* allocate(size_t n)
    {
        return static_cast<T*>(m_resource->Allocate(n * sizeof(T), alignof(T)));
    }

    /**
     * Forwards each call to the resource.
     */
    void deallocate(T* p, size_t n) noexcept
    {
        m_resource->Deallocate(p, n * sizeof(T), alignof(T));
    }

    ResourceType* resource() const noexcept
    {
        return m_resource;
    }
};

template <class T1, class T2, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
bool operator==(const PoolAllocator<T1, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& a,
                const PoolAllocator<T2, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& b) noexcept
{
    return a.resource() == b.resource();
}

template <class T1, class T2, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
bool operator!=(const PoolAllocator<T1, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& a,
                const PoolAllocator<T2, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& b) noexcept
{
    return !(a == b);
}

#endif // BITCOIN_SUPPORT_ALLOCATORS_POOL_H
