// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_H
#define BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_H

#include <support/allocators/node_allocator/allocator_fwd.h>
#include <support/allocators/node_allocator/memory_resource.h>

#include <cstdint>
#include <new>

/**
 * @brief Efficient allocator for node-based containers.
 *
 * The combination of Allocator and MemoryResource can be used as an optimization for node-based
 * containers that experience heavy load.
 *
 * ## Behavior
 *
 * MemoryResource mallocs pools of memory and uses these to carve out memory for the nodes. Nodes
 * that are freed by the Allocator are actually put back into a free list for further use. This
 * behavior has two main advantages:
 *
 * - Memory: no malloc control structure is required for each node memory; the free list is stored
 * in-place. This typically saves about 16 bytes per node.
 *
 * - Performance: much fewer calls to malloc/free. Accessing / putting back entries are O(1) with
 * low constant overhead.
 *
 * There's no free lunch, so there are also disadvantages:
 *
 * - It is necessary to know the exact size of the container internally used nodes beforehand, but
 * there is no standard way to get this.
 *
 * - Memory that's been used for nodes is always put back into a free list and never given back to
 * the system. Memory is only freed when the MemoryResource is destructed.
 *
 * - The free list is a simple last-in-first-out linked list, it doesn't reorder elements based on
 * proximity. So freeing and malloc'ing again can become a random access pattern which can lead to
 * more cache misses.
 *
 * ## Design & Implementation
 *
 * Allocator is a cheaply copyable, `std::allocator`-compatible type used for the containers.
 * Similar to `std::pmr::polymorphic_allocator`, it holds a pointer to a memory resource.
 *
 * MemoryResource is an immobile object that actually allocates, holds and manages memory. Currently
 * it is only able to provide optimized alloc/free for a single fixed allocation size. Only allocations
 * that match this size will be provided from the preallocated pools of memory; all other requests
 * simply use `::operator new()`. Using node_allocator with a standard container types like std::list,
 * std::unordered_map requires knowing node sizes of those containers which are non-standard
 * implementation details. The \ref memusage::NodeSize trait and \ref node_allocator::Factory class can
 * be used to help with this.
 *
 * Node size is determined by memusage::NodeSize and verified to work in various alignment scenarios
 * in `node_allocator_tests/test_allocations_are_used`.
 *
 * ## Further Links
 *
 * @see CppCon 2017: Bob Steagall “How to Write a Custom Allocator”
 *   https://www.youtube.com/watch?v=kSWfushlvB8
 * @see C++Now 2018: Arthur O'Dwyer “An Allocator is a Handle to a Heap”
 *   https://www.youtube.com/watch?v=0MdSJsCTRkY
 * @see AllocatorAwareContainer: Introduction and pitfalls of propagate_on_container_XXX defaults
 *   https://www.foonathan.net/2015/10/allocatorawarecontainer-propagation-pitfalls/
 */
namespace node_allocator {

/**
 * Allocator that's usable for node-based containers like std::unordered_map or std::list.
 *
 * The allocator is stateful, and can be cheaply copied. Its state is an immobile MemoryResource,
 * which actually does all the allocation/deallocations. So this class is just a simple wrapper that
 * conforms to the required STL interface to be usable for the node-based containers.
 */
template <typename T, size_t ALLOCATION_SIZE_BYTES>
class Allocator
{
    template <typename U, size_t AS>
    friend class Allocator;

    template <typename X, typename Y, size_t AS>
    friend bool operator==(const Allocator<X, AS>& a, const Allocator<Y, AS>& b) noexcept;

public:
    using value_type = T;

    /**
     * The allocator is stateful so we can't use the compile time `is_always_equal` optimization and
     * have to use the runtime operator==.
     */
    using is_always_equal = std::false_type;

    /**
     * Move assignment should be a fast operation. In the case of a = std::move(b), we want
     * a to be able to use b's allocator, otherwise all elements would have to be recreated with a's
     * old allocator.
     */
    using propagate_on_container_move_assignment = std::true_type;

    /**
     * The default for propagate_on_container_swap is std::false_type. This is bad,because swapping
     * two containers with unequal allocators but not propagating the allocator is undefined
     * behavior! Obviously, we want so swap the allocator as well, so we set that to true.
     *
     * see https://www.foonathan.net/2015/10/allocatorawarecontainer-propagation-pitfalls/
     */
    using propagate_on_container_swap = std::true_type; // to avoid the undefined behavior

    /**
     * Move and swap have to propagate the allocator, so for consistency we do the same for copy
     * assignment.
     */
    using propagate_on_container_copy_assignment = std::true_type;

    /**
     * Construct a new Allocator object which will delegate all allocations/deallocations to the
     * memory resource.
     */
    explicit Allocator(MemoryResource<ALLOCATION_SIZE_BYTES>* memory_resource) noexcept
        : m_memory_resource(memory_resource)
    {
    }

    /**
     * Conversion constructor for rebinding. All Allocators use the same memory_resource.
     *
     * The rebound type is determined with the `struct rebind`. The rebind is necessary feature
     * for standard allocators, and it happens when the allocator shall be used for different types.
     * E.g. for std::unordered_map this happens for allocation of the internally used nodes, for the
     * underlying index array, and possibly for some control structure. In fact the type required by
     * the standard `std::pair<const Key, Value>` type is never actually used for any allocation
     * (which is therefore a stupid requirement, but that's just C++ being C++).
     */
    template <typename U>
    Allocator(const Allocator<U, ALLOCATION_SIZE_BYTES>& other) noexcept
        : m_memory_resource(other.m_memory_resource)
    {
    }

    /**
     * From the standard: rebind is only optional (provided by std::allocator_traits) if this
     * allocator is a template of the form SomeAllocator<T, Args>, where Args is zero or more
     * additional template type parameters.
     *
     * Since we use a size_t as additional argument, it's *not* optional.
     * @see https://en.cppreference.com/w/cpp/named_req/Allocator#cite_note-2
     */
    template <typename U>
    struct rebind {
        using other = Allocator<U, ALLOCATION_SIZE_BYTES>;
    };

    /**
     * Allocates n entries of the given type.
     */
    T* allocate(size_t n)
    {
        if constexpr (ALLOCATION_SIZE_BYTES == detail::RequiredAllocationSizeBytes<T>()) {
            if (n != 1) {
                // pool is not used so forward to operator new.
                return static_cast<T*>(::operator new(n * sizeof(T)));
            }

            // Forward all allocations to the memory_resource
            return m_memory_resource->template Allocate<T>();
        } else {
            // pool is not used so forward to operator new.
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
    }

    /**
     * Deallocates n entries of the given type.
     */
    void deallocate(T* p, size_t n)
    {
        if constexpr (ALLOCATION_SIZE_BYTES == detail::RequiredAllocationSizeBytes<T>()) {
            if (n != 1) {
                // allocation didn't happen with the pool
                ::operator delete(p);
                return;
            }
            m_memory_resource->template Deallocate<T>(p);
        } else {
            // allocation didn't happen with the pool
            ::operator delete(p);
        }
    }

private:
    //! Stateful allocator, where the state is a simple pointer that can be cheaply copied.
    MemoryResource<ALLOCATION_SIZE_BYTES>* m_memory_resource;
};


/**
 * Since Allocator is stateful, comparison with another one only returns true if it uses the same
 * memory_resource.
 */
template <typename T, typename U, size_t CS>
bool operator==(const Allocator<T, CS>& a, const Allocator<U, CS>& b) noexcept
{
    // "Equality of an allocator is determined through the ability of allocating memory with one
    // allocator and deallocating it with another." - Jonathan Müller
    // See https://www.foonathan.net/2015/10/allocatorawarecontainer-propagation-pitfalls/
    //
    // For us that is the case when both allocators use the same memory resource.
    return a.m_memory_resource == b.m_memory_resource;
}

template <typename T, typename U, size_t CS>
bool operator!=(const Allocator<T, CS>& a, const Allocator<U, CS>& b) noexcept
{
    return !(a == b);
}

} // namespace node_allocator

#endif // BITCOIN_SUPPORT_ALLOCATORS_NODE_ALLOCATOR_ALLOCATOR_H
