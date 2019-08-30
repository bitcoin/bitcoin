// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <support/allocators/bulk_pool.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <list>
#include <map>
#include <string>
#include <unordered_map>

BOOST_FIXTURE_TEST_SUITE(bulk_pool_tests, BasicTestingSetup)

void check(const bulk_pool::Pool& pool, size_t chunk_size, size_t num_free_chunks, size_t num_blocks)
{
    BOOST_CHECK_EQUAL(chunk_size, pool.ChunkSize());
    BOOST_CHECK_EQUAL(num_free_chunks, pool.NumFreeChunks());
    BOOST_CHECK_EQUAL(num_blocks, pool.NumBlocks());
}

BOOST_AUTO_TEST_CASE(specified_size)
{
    auto constexpr s = sizeof(std::string);
    bulk_pool::Pool pool(s);
    check(pool, s, 0, 0);

    // can't allocate something of the wrong size
    BOOST_CHECK(nullptr == pool.Allocate<int>());
    check(pool, s, 0, 0);

    std::string* data = pool.Allocate<std::string>();
    BOOST_CHECK(nullptr != data);
    // Block of 4 chunks allocated, one is returned => 3 remaining free
    check(pool, s, 3, 1);

    pool.Deallocate(data);
    check(pool, s, 4, 1);

    BOOST_CHECK(nullptr != pool.Allocate<std::string>());
    check(pool, s, 3, 1);
    BOOST_CHECK(nullptr != pool.Allocate<std::string>());
    check(pool, s, 2, 1);
    BOOST_CHECK(nullptr != pool.Allocate<std::string>());
    check(pool, s, 1, 1);
    BOOST_CHECK(nullptr != pool.Allocate<std::string>());
    check(pool, s, 0, 1);
    // another block of 8 chunks is allocated, and one returned
    BOOST_CHECK(nullptr != pool.Allocate<std::string>());
    check(pool, s, 7, 2);
}

BOOST_AUTO_TEST_CASE(too_small)
{
    bulk_pool::Pool pool;
    BOOST_CHECK(nullptr == pool.Allocate<char>());
    check(pool, 0, 0, 0);

    BOOST_CHECK(nullptr != pool.Allocate<void*>());
    check(pool, sizeof(void*), 3, 1);
}

BOOST_AUTO_TEST_CASE(std_unordered_map)
{
    using Map = std::unordered_map<uint64_t, uint64_t, std::hash<uint64_t>, std::equal_to<uint64_t>, bulk_pool::Allocator<std::pair<const uint64_t, uint64_t>>>;

    bulk_pool::Pool pool;
    Map m(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool});
    size_t num_free_chunks = 0;
    {
        Map a(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool});
        for (uint64_t i = 0; i < 1000; ++i) {
            a[i] = i;
        }

        num_free_chunks = pool.NumFreeChunks();

        // create a copy of the map, destroy the map => now a lot more free chunks should be available
        {
            Map b = a;
        }

        BOOST_CHECK(pool.NumFreeChunks() > num_free_chunks);
        num_free_chunks = pool.NumFreeChunks();

        // creating another copy, and then destroying everything should reuse all the chunks
        {
            Map b = a;
        }
        BOOST_CHECK_EQUAL(pool.NumFreeChunks(), num_free_chunks);

        // moving the map should not create new nodes
        m = std::move(a);
        BOOST_CHECK_EQUAL(pool.NumFreeChunks(), num_free_chunks);
    }
    // a is destroyed, still all chunks should stay roughly the same.
    BOOST_CHECK(pool.NumFreeChunks() <= num_free_chunks + 5);

    m = Map(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool});

    // now we got everything free
    BOOST_CHECK(pool.NumFreeChunks() > num_free_chunks + 50);
}

BOOST_AUTO_TEST_CASE(std_unordered_map_different_pool)
{
    using Map = std::unordered_map<uint64_t, uint64_t, std::hash<uint64_t>, std::equal_to<uint64_t>, bulk_pool::Allocator<std::pair<const uint64_t, uint64_t>>>;

    bulk_pool::Pool pool_a;
    bulk_pool::Pool pool_b;

    Map map_a(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool_a});
    Map map_b(0, Map::hasher{}, Map::key_equal{}, Map::allocator_type{&pool_b});

    for (int i = 0; i < 100; ++i) {
        map_a[i] = i;
        map_b[i] = i;
    }

    // all the same, so far.
    BOOST_CHECK_EQUAL(pool_a.NumFreeChunks(), pool_b.NumFreeChunks());
    map_a = map_b;
    // note that map_a now uses pool_b, since propagate_on_container_copy_assignment is std::true_type!
    // pool_a is not needed any more and can be destroyed.
    pool_a.Destroy();

    // add some more data to a, should be fine
    for (int i = 100; i < 200; ++i) {
        map_a[i] = i;
    }
}

BOOST_AUTO_TEST_SUITE_END()
