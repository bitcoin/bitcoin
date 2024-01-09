// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_POOLRESOURCETESTER_H
#define BITCOIN_TEST_UTIL_POOLRESOURCETESTER_H

#include <support/allocators/pool.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * Helper to get access to private parts of PoolResource. Used in unit tests and in the fuzzer
 */
class PoolResourceTester
{
    struct PtrAndBytes {
        uintptr_t ptr;
        std::size_t size;

        PtrAndBytes(const void* p, std::size_t s)
            : ptr(reinterpret_cast<uintptr_t>(p)), size(s)
        {
        }

        /**
         * defines a sort ordering by the pointer value
         */
        friend bool operator<(PtrAndBytes const& a, PtrAndBytes const& b)
        {
            return a.ptr < b.ptr;
        }
    };

public:
    /**
     * Extracts the number of elements per freelist
     */
    template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
    static std::vector<std::size_t> FreeListSizes(const PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& resource)
    {
        auto sizes = std::vector<std::size_t>();
        for (const auto* ptr : resource.m_free_lists) {
            size_t size = 0;
            while (ptr != nullptr) {
                ++size;
                ptr = ptr->m_next;
            }
            sizes.push_back(size);
        }
        return sizes;
    }

    /**
     * How many bytes are still available from the last allocated chunk
     */
    template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
    static std::size_t AvailableMemoryFromChunk(const PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& resource)
    {
        return resource.m_available_memory_end - resource.m_available_memory_it;
    }

    /**
     * Once all blocks are given back to the resource, tests that the freelists are consistent:
     *
     * * All data in the freelists must come from the chunks
     * * Memory doesn't overlap
     * * Each byte in the chunks can be accounted for in either the freelist or as available bytes.
     */
    template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
    static void CheckAllDataAccountedFor(const PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& resource)
    {
        // collect all free blocks by iterating all freelists
        std::vector<PtrAndBytes> free_blocks;
        for (std::size_t freelist_idx = 0; freelist_idx < resource.m_free_lists.size(); ++freelist_idx) {
            std::size_t bytes = freelist_idx * resource.ELEM_ALIGN_BYTES;
            auto* ptr = resource.m_free_lists[freelist_idx];
            while (ptr != nullptr) {
                free_blocks.emplace_back(ptr, bytes);
                ptr = ptr->m_next;
            }
        }
        // also add whatever has not yet been used for blocks
        auto num_available_bytes = resource.m_available_memory_end - resource.m_available_memory_it;
        if (num_available_bytes > 0) {
            free_blocks.emplace_back(resource.m_available_memory_it, num_available_bytes);
        }

        // collect all chunks
        std::vector<PtrAndBytes> chunks;
        for (const std::byte* ptr : resource.m_allocated_chunks) {
            chunks.emplace_back(ptr, resource.ChunkSizeBytes());
        }

        // now we have all the data from all freelists on the one hand side, and all chunks on the other hand side.
        // To check if all of them match, sort by address and iterate.
        std::sort(free_blocks.begin(), free_blocks.end());
        std::sort(chunks.begin(), chunks.end());

        auto chunk_it = chunks.begin();
        auto chunk_ptr_remaining = chunk_it->ptr;
        auto chunk_size_remaining = chunk_it->size;
        for (const auto& free_block : free_blocks) {
            if (chunk_size_remaining == 0) {
                assert(chunk_it != chunks.end());
                ++chunk_it;
                assert(chunk_it != chunks.end());
                chunk_ptr_remaining = chunk_it->ptr;
                chunk_size_remaining = chunk_it->size;
            }
            assert(free_block.ptr == chunk_ptr_remaining);                   // ensure addresses match
            assert(free_block.size <= chunk_size_remaining);                 // ensure no overflow
            assert((free_block.ptr & (resource.ELEM_ALIGN_BYTES - 1)) == 0); // ensure correct alignment
            chunk_ptr_remaining += free_block.size;
            chunk_size_remaining -= free_block.size;
        }
        // ensure we are at the end of the chunks
        assert(chunk_ptr_remaining == chunk_it->ptr + chunk_it->size);
        ++chunk_it;
        assert(chunk_it == chunks.end());
        assert(chunk_size_remaining == 0);
    }
};

#endif // BITCOIN_TEST_UTIL_POOLRESOURCETESTER_H
