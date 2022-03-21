// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <support/allocators/node_allocator/factory.h>
#include <test/node_allocator_helpers.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace node_allocator {

class MemoryResourceTester
{
public:
    /**
     * Counts number of free entries in the free list. This is an O(n) operation
     */
    template <size_t ALLOCATION_SIZE_BYTES>
    [[nodiscard]] static size_t NumFreeAllocations(
        MemoryResource<ALLOCATION_SIZE_BYTES> const& mr) noexcept
    {
        size_t length = 0;
        auto allocation = mr.m_free_allocations;
        while (allocation) {
            allocation = static_cast<detail::FreeList const*>(allocation)->next;
            ++length;
        }
        return length;
    }

    /**
     * Number of memory pools that have been allocated
     */
    template <size_t ALLOCATION_SIZE_BYTES>
    [[nodiscard]] static size_t NumPools(MemoryResource<ALLOCATION_SIZE_BYTES> const& mr) noexcept
    {
        return mr.m_allocated_pools.size();
    }


    /**
     * Extracts allocation size from the type
     */
    template <size_t ALLOCATION_SIZE_BYTES>
    [[nodiscard]] static constexpr size_t AllocationSizeBytes(MemoryResource<ALLOCATION_SIZE_BYTES> const&) noexcept
    {
        return ALLOCATION_SIZE_BYTES;
    }
};

} // namespace node_allocator

#define CHECK_IN_RANGE(what, lower_inclusive, upper_inclusive) \
    BOOST_TEST(what >= lower_inclusive);                       \
    BOOST_TEST(what <= upper_inclusive);

namespace {

struct TwoMapsSetup {
    using Factory = node_allocator::Factory<std::unordered_map<uint64_t, uint64_t>>;
    Factory::MemoryResourceType mr_a{};
    Factory::MemoryResourceType mr_b{};

    std::optional<Factory::ContainerType> map_a{};
    std::optional<Factory::ContainerType> map_b{};

    TwoMapsSetup()
    {
        map_a = Factory::CreateContainer(&mr_a);
        for (int i = 0; i < 100; ++i) {
            (*map_a)[i] = i;
        }

        map_b = Factory::CreateContainer(&mr_b);
        (*map_b)[123] = 321;

        BOOST_CHECK(map_a->get_allocator() != map_b->get_allocator());
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_b), 0);
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumPools(mr_b), 1);
    }

    void DestroyMapBAndCheckAfterAssignment()
    {
        // map_a now uses mr_b, since propagate_on_container_copy_assignment is std::true_type
        BOOST_CHECK(map_a->get_allocator() == map_b->get_allocator());

        // With MSVC there might be an additional allocation used for a control structure, so we
        // can't just use BOOST_CHECK_EQUAL.
        CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_b), 1U, 2U);
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumPools(mr_b), 1);

        // map_b was now recreated with data from map_a, using mr_a as the memory resource.
        map_b.reset();

        // map_b destroyed, should not have any effect on mr_b
        CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_b), 1U, 2U);
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumPools(mr_b), 1);

        // but we'll get more free allocations in mr_a
        CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_a), 100U, 101U);
    }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>>
void TestAllocationsAreUsed()
{
    using Factory = node_allocator::Factory<std::unordered_map<Key, Value, Hash>>;
    typename Factory::MemoryResourceType mr{};
    BOOST_TEST_MESSAGE(
        strprintf("%u sizeof(void*), %u/%u/%u sizeof Key/Value/Pair, %u ALLOCATION_SIZE_BYTES",
                  sizeof(void*), sizeof(Key), sizeof(Value), sizeof(std::pair<const Key, Value>),
                  node_allocator::MemoryResourceTester::AllocationSizeBytes(mr)));
    {
        auto map = Factory::CreateContainer(&mr);
        for (size_t i = 0; i < 5; ++i) {
            map[i];
        }
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr), 0);
        map.clear();
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr), 5);

        for (size_t i = 0; i < 5; ++i) {
            map[i];
        }
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr), 0);
        map.clear();
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr), 5);
    }

    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr), 5, 6);
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(node_allocator_tests, BasicTestingSetup)

#define CHECK_NUMS_FREES_POOLS(mr, num_free_allocations, num_pools)                  \
    BOOST_CHECK_EQUAL(num_free_allocations,                                          \
                      node_allocator::MemoryResourceTester::NumFreeAllocations(mr)); \
    BOOST_CHECK_EQUAL(num_pools, node_allocator::MemoryResourceTester::NumPools(mr));

BOOST_AUTO_TEST_CASE(too_small)
{
    // Verify that MemoryResource specialized for a larger char pair type will do smaller single char allocations as well
    using T = std::pair<char, char>;
    node_allocator::MemoryResource<node_allocator::detail::RequiredAllocationSizeBytes<T>()> mr{};
    void* ptr{mr.Allocate<char>()};
    BOOST_CHECK(ptr != nullptr);

    // mr is used
    CHECK_NUMS_FREES_POOLS(mr, 0, 1);
    mr.Deallocate<char>(ptr);
    CHECK_NUMS_FREES_POOLS(mr, 1, 1);

    // freelist is used
    ptr = mr.Allocate<T>();
    BOOST_CHECK(ptr != nullptr);
    CHECK_NUMS_FREES_POOLS(mr, 0, 1);
    mr.Deallocate<char>(ptr);
    CHECK_NUMS_FREES_POOLS(mr, 1, 1);
}

BOOST_AUTO_TEST_CASE(std_unordered_map)
{
    using Factory = node_allocator::Factory<std::unordered_map<uint64_t, uint64_t>>;

    Factory::MemoryResourceType mr{};
    auto m = Factory::CreateContainer(&mr);
    size_t num_free_allocations = 0;
    {
        auto a = Factory::CreateContainer(&mr);

        // Allocator compares equal because the same memory resource is used
        BOOST_CHECK(a.get_allocator() == m.get_allocator());
        for (uint64_t i = 0; i < 1000; ++i) {
            a[i] = i;
        }

        num_free_allocations = node_allocator::MemoryResourceTester::NumFreeAllocations(mr);

        // create a copy of the map, destroy the map => now a lot more free allocations should be
        // available
        {
            auto b = a;
        }

        BOOST_CHECK(node_allocator::MemoryResourceTester::NumFreeAllocations(mr) >=
                    num_free_allocations + 1000);
        num_free_allocations = node_allocator::MemoryResourceTester::NumFreeAllocations(mr);

        // creating another copy, and then destroying everything should reuse all the allocations
        {
            auto b = a;
        }
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr),
                          num_free_allocations);

        // moving the map should not create new nodes
        m = std::move(a);
        BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr),
                          num_free_allocations);
    }

    // a is destroyed, still all allocations should stay roughly the same because its contents were
    // moved to m which is still alive
    BOOST_CHECK(node_allocator::MemoryResourceTester::NumFreeAllocations(mr) <=
                num_free_allocations + 5);

    num_free_allocations = node_allocator::MemoryResourceTester::NumFreeAllocations(mr);
    m = Factory::CreateContainer(&mr);

    // now that m is replaced its content is freed
    BOOST_CHECK(node_allocator::MemoryResourceTester::NumFreeAllocations(mr) >=
                num_free_allocations + 1000);
}

BOOST_FIXTURE_TEST_CASE(different_memoryresource_assignment, TwoMapsSetup)
{
    map_b = map_a;

    DestroyMapBAndCheckAfterAssignment();

    // finally map_a is destroyed, getting more free allocations.
    map_a.reset();
    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_a), 200U, 202U);
}

BOOST_FIXTURE_TEST_CASE(different_memoryresource_move, TwoMapsSetup)
{
    map_b = std::move(map_a);

    DestroyMapBAndCheckAfterAssignment();

    // finally map_a is destroyed, but since it was moved, no more free allocations.
    map_a.reset();
    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_a), 100U, 102U);
}


BOOST_FIXTURE_TEST_CASE(different_memoryresource_swap, TwoMapsSetup)
{
    auto alloc_a = map_a->get_allocator();
    auto alloc_b = map_b->get_allocator();

    std::swap(map_a, map_b);

    // The maps have swapped, so their allocators have swapped, too.
    // No additional allocations have occurred!
    BOOST_CHECK(map_a->get_allocator() != map_b->get_allocator());
    BOOST_CHECK(alloc_a == map_b->get_allocator());
    BOOST_CHECK(alloc_b == map_a->get_allocator());
    map_b.reset();

    // map_b destroyed, so mr_a must have plenty of free allocations now
    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_a), 100U, 101U);

    // nothing happened to map_a, so mr_b still has no free allocations
    BOOST_CHECK_EQUAL(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_b), 0);
    map_a.reset();

    // finally map_a is destroyed, so we got an entry back for mr_b.
    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_a), 100U, 101U);
    CHECK_IN_RANGE(node_allocator::MemoryResourceTester::NumFreeAllocations(mr_b), 1U, 2U);
}

BOOST_AUTO_TEST_CASE(calc_required_allocation_size)
{
    static_assert(sizeof(AlignedSize<1, 1>) == 1U);
    static_assert(std::alignment_of_v<AlignedSize<1, 1>> == 1U);

    static_assert(sizeof(AlignedSize<16, 1>) == 16U);
    static_assert(std::alignment_of_v<AlignedSize<16, 1>> == 16U);
    static_assert(sizeof(AlignedSize<16, 16>) == 16U);
    static_assert(std::alignment_of_v<AlignedSize<16, 16>> == 16U);
    static_assert(sizeof(AlignedSize<16, 24>) == 32U);
    static_assert(std::alignment_of_v<AlignedSize<16, 24>> == 16U);

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 1>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 7>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 8>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 9>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 15>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 16>>());
    static_assert(24U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 17>>());
    static_assert(104U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 100>>());

    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 4>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 7>>());
    static_assert(104U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 100>>());
#elif UINTPTR_MAX == 0xFFFFFFFF
    static_assert(4U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 1>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 7>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 8>>());
    static_assert(12U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 9>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 15>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 16>>());
    static_assert(20U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 17>>());
    static_assert(100U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<1, 100>>());

    static_assert(4U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 4>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 7>>());
    static_assert(100U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<4, 100>>());
#else
#error "Invalid sizeof(void*)"
#endif

    // Anything with alignment >= 8 is the same in both 32bit and 64bit

    static_assert(104U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<8, 100>>());

    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<8, 1>>());
    static_assert(8U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<8, 8>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<8, 16>>());

    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<16, 1>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<16, 8>>());
    static_assert(16U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<16, 16>>());
    static_assert(32U == node_allocator::detail::RequiredAllocationSizeBytes<AlignedSize<16, 17>>());
}

using MappedTypes = std::tuple<
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    std::string,
    AlignedSize<__STDCPP_DEFAULT_NEW_ALIGNMENT__, 1> // Biggest allowed alignment is __STDCPP_DEFAULT_NEW_ALIGNMENT__
    >;

BOOST_AUTO_TEST_CASE_TEMPLATE(allocations_are_used, T, MappedTypes)
{
#if defined(_LIBCPP_VERSION) // defined in any C++ header from libc++
    BOOST_TEST_MESSAGE("_LIBCPP_VERSION is defined");
#endif
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
    BOOST_TEST_MESSAGE("__GLIBCXX__ or __GLIBCPP__ is defined");
#endif

    TestAllocationsAreUsed<uint8_t, T>();
    TestAllocationsAreUsed<uint8_t, T, NotNoexceptHash<uint8_t>>();

    TestAllocationsAreUsed<uint16_t, T>();
    TestAllocationsAreUsed<uint16_t, T, NotNoexceptHash<uint16_t>>();

    TestAllocationsAreUsed<uint32_t, T>();
    TestAllocationsAreUsed<uint32_t, T, NotNoexceptHash<uint32_t>>();

    TestAllocationsAreUsed<uint64_t, T>();
    TestAllocationsAreUsed<uint64_t, T, NotNoexceptHash<uint64_t>>();
}

BOOST_AUTO_TEST_SUITE_END()
