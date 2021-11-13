// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_NODE_SIZE_H
#define BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_NODE_SIZE_H

#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace node_allocator {

/**
 * Calculates memory usage of nodes for different containers.
 */
template <typename T>
struct NodeSize;

/**
 * It is important that the calculation here matches exactly the behavior of std::unordered_map
 * so the node_allocator actually works. This is tested in node_allocator_tests/test_allocations_are_used
 * with multiple configurations (alignments, noexcept hash, node sizes).
 */
template <typename Key, typename V, typename Hash, typename Equals, typename Allocator>
struct NodeSize<std::unordered_map<Key, V, Hash, Equals, Allocator>> {
    using ContainerType = std::unordered_map<Key, V, Hash, Equals, Allocator>;
    using ValueType = typename ContainerType::value_type;

#if defined(_MSC_VER)
    // libstdc++, libc++, and MSVC implement the nodes differently. To get the correct size with
    // the correct alignment, we can simulate the memory layouts with accordingly nested std::pairs.
    //
    // list node contains 2 pointers and no hash; see
    // https://github.com/microsoft/STL/blob/main/stl/inc/unordered_map and
    // https://github.com/microsoft/STL/blob/main/stl/inc/list
    using SimulatedNodeType = std::pair<std::pair<void*, void*>, ValueType>;
#elif defined(_LIBCPP_VERSION) // defined in any C++ header from libc++
    // libc++ always stores hash and pointer in the node
    // see https://github.com/llvm/llvm-project/blob/release/13.x/libcxx/include/__hash_table#L92
    using SimulatedNodeType = std::pair<ValueType, std::pair<size_t, void*>>;
#else
    // libstdc++ doesn't store hash when its operator() is noexcept;
    // see hashtable_policy.h, struct _Hash_node
    // https://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/a05689.html
    using SimulatedNodeType = std::conditional_t<noexcept(std::declval<Hash>()(std::declval<const Key&>())),
                                                 std::pair<void*, ValueType>,                     // no hash stored
                                                 std::pair<void*, std::pair<ValueType, size_t>>>; // hash stored along ValueType, and that is wrapped with the pointer.
#endif

    static constexpr size_t VALUE = sizeof(SimulatedNodeType);
};

} // namespace node_allocator

#endif // BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_NODE_SIZE_H
