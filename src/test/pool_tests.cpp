// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memusage.h>
#include <support/allocators/pool.h>
#include <test/util/poolresourcetester.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(pool_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(basic_allocating)
{
    auto resource = PoolResource<8, 8>();
    PoolResourceTester::CheckAllDataAccountedFor(resource);

    // first chunk is already allocated
    size_t expected_bytes_available = resource.ChunkSizeBytes();
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    // chunk is used, no more allocation
    void* block = resource.Allocate(8, 8);
    expected_bytes_available -= 8;
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    BOOST_TEST(0 == PoolResourceTester::FreeListSizes(resource)[1]);
    resource.Deallocate(block, 8, 8);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);

    // alignment is too small, but the best fitting freelist is used. Nothing is allocated.
    void* b = resource.Allocate(8, 1);
    BOOST_TEST(b == block); // we got the same block of memory as before
    BOOST_TEST(0 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    resource.Deallocate(block, 8, 1);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    // can't use resource because alignment is too big, allocate system memory
    b = resource.Allocate(8, 16);
    BOOST_TEST(b != block);
    block = b;
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    resource.Deallocate(block, 8, 16);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    // can't use chunk because size is too big
    block = resource.Allocate(16, 8);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    resource.Deallocate(block, 16, 8);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    // it's possible that 0 bytes are allocated, make sure this works. In that case the call is forwarded to operator new
    // 0 bytes takes one entry from the first freelist
    void* p = resource.Allocate(0, 1);
    BOOST_TEST(0 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));

    resource.Deallocate(p, 0, 1);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    BOOST_TEST(1 == PoolResourceTester::FreeListSizes(resource)[1]);
    BOOST_TEST(expected_bytes_available == PoolResourceTester::AvailableMemoryFromChunk(resource));
}

// Allocates from 0 to n bytes were n > the PoolResource's data, and each should work
BOOST_AUTO_TEST_CASE(allocate_any_byte)
{
    auto resource = PoolResource<128, 8>(1024);

    uint8_t num_allocs = 200;

    auto data = std::vector<std::span<uint8_t>>();

    // allocate an increasing number of bytes
    for (uint8_t num_bytes = 0; num_bytes < num_allocs; ++num_bytes) {
        uint8_t* bytes = new (resource.Allocate(num_bytes, 1)) uint8_t[num_bytes];
        BOOST_TEST(bytes != nullptr);
        data.emplace_back(bytes, num_bytes);

        // set each byte to num_bytes
        std::fill(bytes, bytes + num_bytes, num_bytes);
    }

    // now that we got all allocated, test if all still have the correct values, and give everything back to the allocator
    uint8_t val = 0;
    for (auto const& span : data) {
        for (auto x : span) {
            BOOST_TEST(val == x);
        }
        std::destroy(span.data(), span.data() + span.size());
        resource.Deallocate(span.data(), span.size(), 1);
        ++val;
    }

    PoolResourceTester::CheckAllDataAccountedFor(resource);
}

BOOST_AUTO_TEST_CASE(random_allocations)
{
    struct PtrSizeAlignment {
        void* ptr;
        size_t bytes;
        size_t alignment;
    };

    // makes a bunch of random allocations and gives all of them back in random order.
    auto resource = PoolResource<128, 8>(65536);
    std::vector<PtrSizeAlignment> ptr_size_alignment{};
    for (size_t i = 0; i < 1000; ++i) {
        // make it a bit more likely to allocate than deallocate
        if (ptr_size_alignment.empty() || 0 != m_rng.randrange(4)) {
            // allocate a random item
            std::size_t alignment = std::size_t{1} << m_rng.randrange(8);          // 1, 2, ..., 128
            std::size_t size = (m_rng.randrange(200) / alignment + 1) * alignment; // multiple of alignment
            void* ptr = resource.Allocate(size, alignment);
            BOOST_TEST(ptr != nullptr);
            BOOST_TEST((reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0);
            ptr_size_alignment.push_back({ptr, size, alignment});
        } else {
            // deallocate a random item
            auto& x = ptr_size_alignment[m_rng.randrange(ptr_size_alignment.size())];
            resource.Deallocate(x.ptr, x.bytes, x.alignment);
            x = ptr_size_alignment.back();
            ptr_size_alignment.pop_back();
        }
    }

    // deallocate all the rest
    for (auto const& x : ptr_size_alignment) {
        resource.Deallocate(x.ptr, x.bytes, x.alignment);
    }

    PoolResourceTester::CheckAllDataAccountedFor(resource);
}

BOOST_AUTO_TEST_CASE(memusage_test)
{
    auto std_map = std::unordered_map<int64_t, int64_t>{};

    using Map = std::unordered_map<int64_t,
                                   int64_t,
                                   std::hash<int64_t>,
                                   std::equal_to<int64_t>,
                                   PoolAllocator<std::pair<const int64_t, int64_t>,
                                                 sizeof(std::pair<const int64_t, int64_t>) + sizeof(void*) * 4>>;
    auto resource = Map::allocator_type::ResourceType(1024);

    PoolResourceTester::CheckAllDataAccountedFor(resource);

    {
        auto resource_map = Map{0, std::hash<int64_t>{}, std::equal_to<int64_t>{}, &resource};

        // can't have the same resource usage
        BOOST_TEST(memusage::DynamicUsage(std_map) != memusage::DynamicUsage(resource_map));

        for (size_t i = 0; i < 10000; ++i) {
            std_map[i];
            resource_map[i];
        }

        // Eventually the resource_map should have a much lower memory usage because it has less malloc overhead
        BOOST_TEST(memusage::DynamicUsage(resource_map) <= memusage::DynamicUsage(std_map) * 90 / 100);

        // Make sure the pool is actually used by the nodes
        auto max_nodes_per_chunk = resource.ChunkSizeBytes() / sizeof(Map::value_type);
        auto min_num_allocated_chunks = resource_map.size() / max_nodes_per_chunk + 1;
        BOOST_TEST(resource.NumAllocatedChunks() >= min_num_allocated_chunks);
    }

    PoolResourceTester::CheckAllDataAccountedFor(resource);
}

BOOST_AUTO_TEST_SUITE_END()
