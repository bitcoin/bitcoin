// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <support/allocators/node_allocator/node_size.h>
#include <test/node_allocator_helpers.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <tuple>
#include <typeindex>
#include <unordered_map>

namespace {

struct AllocationInfo {
    AllocationInfo(size_t size) : size(size) {}

    const size_t size{};
    size_t num_allocations{};
};

/**
 * Singleton used by CountSingleAllocationsAllocator to record allocations for each type
 */
std::unordered_map<std::type_index, AllocationInfo>& SingletonAllocationInfo()
{
    static std::unordered_map<std::type_index, AllocationInfo> map{};
    return map;
}

/**
 * A minimal allocator that records all n==1 allocations into SingletonAllocationInfo().
 * That way we can actually find out the size of the allocated nodes.
 */
template <typename T>
struct CountSingleAllocationsAllocator {
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    CountSingleAllocationsAllocator() = default;
    CountSingleAllocationsAllocator(const CountSingleAllocationsAllocator&) noexcept = default;

    template <typename U>
    CountSingleAllocationsAllocator(CountSingleAllocationsAllocator<U> const&) noexcept
    {
    }

    T* allocate(size_t n)
    {
        if (n == 1) {
            auto [it, isInserted] = SingletonAllocationInfo().try_emplace(std::type_index(typeid(T)), sizeof(T));
            it->second.num_allocations += 1;
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n)
    {
        ::operator delete(p);
    }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>>
void TestCorrectNodeSize()
{
    std::unordered_map<Key, Value, Hash, std::equal_to<Key>, CountSingleAllocationsAllocator<std::pair<const Key, Value>>> map;

    SingletonAllocationInfo().clear();

    const size_t num_entries = 123;
    for (size_t i = 0; i < num_entries; ++i) {
        map[i];
    }

    // there should be a entry with exactly 123 allocations
    auto it = std::find_if(SingletonAllocationInfo().begin(), SingletonAllocationInfo().end(), [](const auto& kv) {
        return kv.second.num_allocations == 123;
    });
    BOOST_CHECK(it != SingletonAllocationInfo().end());

    static constexpr auto node_size = node_allocator::NodeSize<std::unordered_map<Key, Value, Hash>>::VALUE;
    BOOST_CHECK_EQUAL(it->second.size, node_size);
}

} // namespace

BOOST_AUTO_TEST_SUITE(node_size_tests)

using MappedTypes = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t, std::string, AlignedSize<16, 16>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(node_sizes, T, MappedTypes)
{
    TestCorrectNodeSize<uint8_t, T>();
    TestCorrectNodeSize<uint8_t, T, NotNoexceptHash<uint8_t>>();

    TestCorrectNodeSize<uint16_t, T>();
    TestCorrectNodeSize<uint16_t, T, NotNoexceptHash<uint16_t>>();

    TestCorrectNodeSize<uint32_t, T>();
    TestCorrectNodeSize<uint32_t, T, NotNoexceptHash<uint32_t>>();

    TestCorrectNodeSize<uint64_t, T>();
    TestCorrectNodeSize<uint64_t, T, NotNoexceptHash<uint64_t>>();
}

BOOST_AUTO_TEST_SUITE_END()
