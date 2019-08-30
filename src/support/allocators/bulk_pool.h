// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_BULK_POOL_H
#define BITCOIN_SUPPORT_ALLOCATORS_BULK_POOL_H

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace bulk_pool {

/// This pool allocates one object (memory block) at a time. The first call to allocate determine
/// the size of chunks in the pool. Subsequent calls with other types of different size will
/// return nullptr. It is designed for use in node-based container like std::unordered_map or
/// std::list, where most allocations are for 1 element. It allocates increasingly large blocks
/// of memory, and only deallocates at destruction.
///
/// The size of the allocated blocks are doubled until NUM_CHUNKS_ALLOC_MAX is reached. This way we
/// don't have too large an overhead for small pool usage, and still get efficiency for lots of
/// elements since number of mallocs() are very reduced. Also, less bookkeeping is necessary, so
/// it's more space efficient than allocating one chunk at a time.
///
/// Deallocate() does not actually free memory, but puts the data into a linked list which can then
/// be used for allocate() calls when 1 element is requested. The linked list is in place, reusing
/// the memory of the chunks.
///
/// Memory layout
///
///  m_blocks
///        v
///    [ BlockNode, chunk0, chunk1, ... chunk3] // block 0
///        v
///    [ BlockNode, chunk0, chunk1, ... chunk7] // block 1
///        :
///    [ BlockNode, chunk0, chunk1, ... chunk16383] // block 12
///        v
///    [ BlockNode, chunk0, chunk1, ... chunk16383] // block 13
///        v
///      nullptr
///
/// m_free_chunks represents a singly linked list of all T's that have been deallocated.
class Pool
{
public:
    // Inplace linked list of all allocated blocks. Make sure it is aligned in such a way that whatever comes
    // after a BlockNode is correctly aligned.
    struct alignas(alignof(::max_align_t)) BlockNode {
        // make sure to align
        BlockNode* next;
    };

    // Inplace linked list of the allocation chunks
    struct ChunkNode {
        ChunkNode* next;
    };

    /// Explicitly specify the Pool's chunk size. It will only allocate chunks of this size, returning
    /// nullptr if a type of different size is specified. If set to 0, the first Allocate() call will
    /// determine the chunk size of the pool.
    explicit Pool(size_t chunk_size) noexcept
        : m_chunk_size(chunk_size)
    {
    }

    /// Doesn't specify the chunk size, so it's determined by the first call to Allocate.
    Pool() = default;

    // Don't allow moving/copying a pool, it's dangerous
    Pool(Pool&&) = delete;
    Pool& operator=(Pool&&) = delete;
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    /// Deallocates all allocated memory, even when Deallocate() was not yet called.
    ~Pool() noexcept
    {
        Destroy();
    }

    /// Don't allow allocation for types that are smaller than a Node. This does not make sense
    /// because we need to fit a pointer into the memory.
    /// We return nullptr instead of producing a compile error so bulk_pool::Allocator<T>::allocate(size_t n)
    /// does not need any special handling.
    template <typename T, typename std::enable_if<sizeof(T) < sizeof(ChunkNode), int>::type = 0>
    T* Allocate() noexcept
    {
        return nullptr;
    }

    /// As with Allocate(), don't allow for types that are smaller than a Node.
    template <typename T, typename std::enable_if<sizeof(T) < sizeof(ChunkNode), int>::type = 0>
    bool Deallocate() noexcept
    {
        return false;
    }

    /// Tries to allocate one T. If allocation is not possible, returns nullptr and the callee
    /// has to do something else to get memory. First caller decides the size of the pool's data.
    template <typename T, typename std::enable_if<sizeof(T) >= sizeof(ChunkNode), int>::type = 0>
    T* Allocate()
    {
        if (m_chunk_size == 0) {
            // allocator not yet used, so this type determines the size.
            m_chunk_size = sizeof(T);
        } else if (m_chunk_size != sizeof(T)) {
            // allocator's size does not match sizeof(T), don't allocate.
            return nullptr;
        }

        // Make sure we have memory available
        if (m_free_chunks == nullptr) {
            AllocateAndCreateFreelist();
        }

        // pop one element from the linked list, returning previous head
        auto old_head = m_free_chunks;
        m_free_chunks = old_head->next;
        return reinterpret_cast<T*>(old_head);
    }

    /// Puts p back into the freelist, if it was the correct size. Only allowed with objects
    /// that were allocated with this pool!
    template <typename T, typename std::enable_if<sizeof(T) >= sizeof(ChunkNode), int>::type = 0>
    bool Deallocate(T* p) noexcept
    {
        if (m_chunk_size != sizeof(T)) {
            // allocation didn't happen with this allocator
            return false;
        }

        // put it into the linked list
        auto n = reinterpret_cast<ChunkNode*>(p);
        n->next = m_free_chunks;
        m_free_chunks = n;
        return true;
    }

    /// Deallocates all allocated memory, even when Deallocate() was not yet called. Use with care!
    void Destroy() noexcept
    {
        while (m_blocks != nullptr) {
            auto old_head = m_blocks;
            m_blocks = m_blocks->next;
            ::operator delete(old_head);
        }
        m_free_chunks = nullptr;
    }

    /// Counts number of free entries in the freelist. This is an O(n) operation. Mostly for debugging / logging / testing.
    size_t NumFreeChunks() const
    {
        return CountNodes(m_free_chunks);
    }

    /// Counts number of allocated blocks. This is an O(n) operation. Mostly for debugging / logging / testing.
    size_t NumBlocks() const
    {
        return CountNodes(m_blocks);
    }

    /// Size per chunk
    size_t ChunkSize() const
    {
        return m_chunk_size;
    }

private:
    //! Minimum number of chunks to allocate for the first block
    static constexpr size_t NUM_CHUNKS_ALLOC_MIN = 4;

    //! Maximum number of chunks to allocate in one block
    static constexpr size_t NUM_CHUNKS_ALLOC_MAX = 16384;

    // Counts list length by iterating until nullptr is reached.
    template <typename N>
    size_t CountNodes(N* node) const
    {
        size_t length = 0;
        while (node != nullptr) {
            node = node->next;
            ++length;
        }
        return length;
    }

    /// Called when no memory is available (m_free_chunks == nullptr).
    void AllocateAndCreateFreelist()
    {
        auto num_chunks = CalcNumChunksToAlloc();
        auto data = ::operator new(sizeof(BlockNode) + m_chunk_size * num_chunks);

        // link block into blocklist
        auto block = reinterpret_cast<BlockNode*>(data);
        block->next = m_blocks;
        m_blocks = block;

        m_free_chunks = MakeFreelist(block, num_chunks);
    }

    /// Doubles number of elements to alloc until max is reached.
    size_t CalcNumChunksToAlloc() noexcept
    {
        if (m_num_chunks_alloc >= NUM_CHUNKS_ALLOC_MAX) {
            return NUM_CHUNKS_ALLOC_MAX;
        }
        auto prev = m_num_chunks_alloc;
        m_num_chunks_alloc *= 2;
        return prev;
    }

    /// Integrates a previously allocated block of memory (where n > 1) into the linked list
    ChunkNode* MakeFreelist(BlockNode* block, const size_t num_elements) noexcept
    {
        // skip BlockNode to get to the chunks
        auto const data = reinterpret_cast<char*>(block + 1);

        // interlink all chunks
        for (size_t i = 0; i < num_elements; ++i) {
            reinterpret_cast<ChunkNode*>(data + i * m_chunk_size)->next = reinterpret_cast<ChunkNode*>(data + (i + 1) * m_chunk_size);
        }

        // last one points nullptr
        reinterpret_cast<ChunkNode*>(data + (num_elements - 1) * m_chunk_size)->next = nullptr;
        return reinterpret_cast<ChunkNode*>(data);
    }

    //! A single linked list of all data available in the pool. This list is used for allocations of single elements.
    ChunkNode* m_free_chunks{nullptr};

    //! A single linked list of all allocated blocks of memory, used to free the data in the destructor.
    BlockNode* m_blocks{nullptr};

    //! The pool's size for the memory blocks. First call to Allocate() determines the used size.
    size_t m_chunk_size{0};

    //! Number of elements to allocate in bulk. Doubles each time, until m_max_num_allocs is reached.
    size_t m_num_chunks_alloc{NUM_CHUNKS_ALLOC_MIN};
};

/// Allocator that's usable for node-based containers like std::unorderd_map or std::list. It requires a Pool
/// which will be used for allocations of n==1. The pool's memory is only actually freed when the pool's
/// destructor is called, otherwise previously allocated memory will be put back into the pool for further use.
///
/// Be aware that this allocator assumes that the containers will call allocate(1) to allocate nodes, otherwise
/// the pool won't be used. Also, it assumes that the *first* call to allocate(1) is done with the actual node
/// size. Only subsequent allocate(1) calls with the same sizeof(T) will make use of the allocator.
template <typename T>
class Allocator
{
    template <typename U>
    friend class Allocator;

    template <typename X, typename Y>
    friend bool operator==(const Allocator<X>& a, const Allocator<Y>& b) noexcept;

public:
    using value_type = T;

    //! Note: when two containers a and b have different pools and a=b is called, we replace a's allocator with b's.
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = Allocator<U>;
    };

    explicit Allocator(Pool* pool) noexcept
        : m_pool(pool)
    {
    }

    /// Conversion constructor, all Allocators use the same pool.
    template <typename U>
    Allocator(const Allocator<U>& other) noexcept
        : m_pool(other.m_pool)
    {
    }

    /// Allocates n entries. When n==1, the pool is used. The pool might return nullptr if sizeof(T)
    /// does not match the pool's chunk size. In that case, we fall back to new().
    /// This method should be kept short so it's easily inlineable.
    T* allocate(size_t n)
    {
        if (n == 1) {
            auto r = m_pool->Allocate<T>();
            if (r != nullptr) {
                return r;
            }
        }
        return reinterpret_cast<T*>(::operator new(n * sizeof(T)));
    }

    /// If n==1, we might have gotten the object from the pool. This is not the case when sizeof(T) does
    /// not match the pool's chunk size. In that case, we fall back to delete().
    void deallocate(T* p, std::size_t n)
    {
        if (n == 1 && m_pool->Deallocate(p)) {
            return;
        }
        ::operator delete(p);
    }

    /// According to the standard, destroy() is an optional Allocator requirement since C++11.
    /// It seems only g++4.8 requires that this method is present, that's why it is here. It
    /// is deprecated in C++17 and removed in C++20.
    ///
    /// Calls p->~U().
    /// see https://en.cppreference.com/w/cpp/memory/allocator/destroy
    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }

    template <class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        ::new ((void*)p) U(std::forward<Args>(args)...);
    }

private:
    Pool* m_pool;
};

/// Since Allocator is stateful, comparison with another one only returns true if it uses the same pool.
template <typename T, typename U>
bool operator==(const Allocator<T>& a, const Allocator<U>& b) noexcept
{
    return a.m_pool == b.m_pool;
}

template <typename T, typename U>
bool operator!=(const Allocator<T>& a, const Allocator<U>& b) noexcept
{
    return !(a == b);
}

} // namespace bulk_pool

#endif // BITCOIN_SUPPORT_ALLOCATORS_BULK_POOL_H
