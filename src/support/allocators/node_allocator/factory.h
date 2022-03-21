// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_FACTORY_H
#define BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_FACTORY_H

#include <support/allocators/node_allocator/allocator.h>
#include <support/allocators/node_allocator/node_size.h>

#include <cassert>
#include <unordered_map>

namespace node_allocator {

/**
 * Generic factory to create node_allocator based containers.
 */
template <typename Container>
class Factory;

/**
 * Helper to create std::unordered_map which uses the node_allocator.
 *
 * This calculates the size of the container's internally used node correctly for all supported
 * platforms, which is also asserted by the unit tests.
 */
template <typename Key, typename Value, typename Hash, typename Equals>
class Factory<std::unordered_map<Key, Value, Hash, Equals>>
{
    using BaseContainerType = std::unordered_map<Key, Value, Hash, Equals>;
    using ValueType = typename BaseContainerType::value_type;
    static constexpr size_t ALLOCATION_SIZE_BYTES = detail::RequiredAllocationSizeBytes<typename NodeSize<BaseContainerType>::SimulatedNodeType>();

public:
    using MemoryResourceType = MemoryResource<ALLOCATION_SIZE_BYTES>;
    using AllocatorType = Allocator<std::pair<const Key, Value>, ALLOCATION_SIZE_BYTES>;
    using ContainerType = std::unordered_map<Key, Value, Hash, Equals, AllocatorType>;

    /**
     * Creates the std::unordered_map container, and asserts that the specified memory_resource is correct.
     */
    [[nodiscard]] static ContainerType CreateContainer(MemoryResourceType* memory_resource)
    {
        assert(memory_resource != nullptr);
        return ContainerType{0, Hash{}, Equals{}, AllocatorType{memory_resource}};
    }
};

} // namespace node_allocator

#endif // BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_FACTORY_H
