// Copyright (c) 2022 The Bitcoin Core developers
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

    auto data = std::vector<Span<uint8_t>>();

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
        if (ptr_size_alignment.empty() || 0 != InsecureRandRange(4)) {
            // allocate a random item
            std::size_t alignment = std::size_t{1} << InsecureRandRange(8);          // 1, 2, ..., 128
            std::size_t size = (InsecureRandRange(200) / alignment + 1) * alignment; // multiple of alignment
            void* ptr = resource.Allocate(size, alignment);
            BOOST_TEST(ptr != nullptr);
            BOOST_TEST((reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0);
            ptr_size_alignment.push_back({ptr, size, alignment});
        } else {
            // deallocate a random item
            auto& x = ptr_size_alignment[InsecureRandRange(ptr_size_alignment.size())];
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


size_t total_allocated_bytes = 0;

/**
 * A simple allocator that increases/decreases a counter for each allocate/deallocate.
 * Based on https://howardhinnant.github.io/allocator_boilerplate.html
 */
template <class T>
class SimpleGlobalCountingAllocator
{
public:
    using value_type = T;

    SimpleGlobalCountingAllocator() = default;

    template <class U>
    SimpleGlobalCountingAllocator(SimpleGlobalCountingAllocator<U> const&) noexcept
    {
    }

    value_type* allocate(std::size_t n)
    {
        total_allocated_bytes += memusage::MallocUsage(n * sizeof(value_type));
        return static_cast<value_type*>(::operator new(n * sizeof(value_type)));
    }

    void deallocate(value_type* p, std::size_t n) noexcept
    {
        total_allocated_bytes -= memusage::MallocUsage(n * sizeof(value_type));
        ::operator delete(p);
    }
};


template <class T, class U>
bool operator==(SimpleGlobalCountingAllocator<T> const&, SimpleGlobalCountingAllocator<U> const&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(SimpleGlobalCountingAllocator<T> const& x, SimpleGlobalCountingAllocator<U> const& y) noexcept
{
    return !(x == y);
}


BOOST_AUTO_TEST_CASE(memusage_test)
{
    total_allocated_bytes = 0;
    using StdMap = std::unordered_map<int64_t, int64_t, std::hash<int64_t>, std::equal_to<int64_t>, SimpleGlobalCountingAllocator<std::pair<const int64_t, int64_t>>>;
    using Map = std::unordered_map<int64_t,
                                   int64_t,
                                   std::hash<int64_t>,
                                   std::equal_to<int64_t>,
                                   PoolAllocator<std::pair<const int64_t, int64_t>,
                                                 sizeof(std::pair<const int64_t, int64_t>) + sizeof(void*) * 4>>;
    auto resource = Map::allocator_type::ResourceType(1024);

    PoolResourceTester::CheckAllDataAccountedFor(resource);

    {
        auto std_map = StdMap{};
        auto resource_map = Map{0, std::hash<int64_t>{}, std::equal_to<int64_t>{}, &resource};

        // can't have the same resource usage
        for (size_t i = 0; i < 10000; ++i) {
            std_map[i];
            resource_map[i];
        }

        // Eventually the resource_map should have a much lower memory usage because it has less malloc overhead
        BOOST_TEST(memusage::DynamicUsage(resource_map) <= total_allocated_bytes * 90 / 100);

        // Make sure the pool is actually used by the nodes
        auto max_nodes_per_chunk = resource.ChunkSizeBytes() / sizeof(Map::value_type);
        auto min_num_allocated_chunks = resource_map.size() / max_nodes_per_chunk + 1;
        BOOST_TEST(resource.NumAllocatedChunks() >= min_num_allocated_chunks);
    }

    // check if counting was correct, everything should be deallocated by now
    BOOST_TEST(total_allocated_bytes == 0U);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
}


struct alignas(sizeof(void*) * 2) Large {
    int i;
};

BOOST_AUTO_TEST_CASE(test_incorrect_alignment)
{
    using Map = std::unordered_map<int64_t,
                                   Large,
                                   std::hash<int64_t>,
                                   std::equal_to<int64_t>,
                                   PoolAllocator<std::pair<const int64_t, Large>,
                                                 sizeof(std::pair<const int64_t, Large>) + sizeof(void*) * 4,
                                                 alignof(void*) // this is too small => pool won't be used for nodes
                                                 >>;
    auto resource = Map::allocator_type::ResourceType(1024);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
    // initially 1 chunk is allocated
    auto initial_size = resource.DynamicMemoryUsage();
    {
        auto resource_map = Map{0, std::hash<int64_t>{}, std::equal_to<int64_t>{}, &resource};
        resource_map.reserve(10000);
        for (size_t i = 0; i < 10000; ++i) {
            resource_map[i];
        }

        // make sure both give the same result
        BOOST_TEST(memusage::DynamicUsage(resource_map) == resource.DynamicMemoryUsage());

        // alignment is too large => resource can't be used for nodes. It might be used for one or two bucket arrays when the map was small.
        BOOST_TEST(resource.NumAllocatedChunks() <= 1);

        // the resource keeps track of everything allocated/deallocated, not just chunk sizes
        auto low_estimate_for_node_size = memusage::MallocUsage(sizeof(Map::value_type) + sizeof(void*) * 2) * resource_map.size();
        auto bucket_array_size = memusage::MallocUsage(resource_map.bucket_count() * sizeof(void*));

        // In my test (Linux 64bit) this estimate is pretty close: 723296 >= 722208
        BOOST_TEST(memusage::DynamicUsage(resource_map) >= low_estimate_for_node_size + bucket_array_size);
    }

    // map is destroyed, so everything allocated by new() is now gone; so the resource should be pretty empty
    BOOST_TEST(resource.DynamicMemoryUsage() == initial_size);
    PoolResourceTester::CheckAllDataAccountedFor(resource);
}

BOOST_AUTO_TEST_SUITE_END()
