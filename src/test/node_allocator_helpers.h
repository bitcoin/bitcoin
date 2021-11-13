#ifndef BITCOIN_TEST_NODE_ALLOCATOR_HELPERS_H
#define BITCOIN_TEST_NODE_ALLOCATOR_HELPERS_H

#include <cstddef>
#include <utility>

/**
 * Wrapper around std::hash, but without noexcept. Useful for testing unordered containers
 * because they potentially behave differently with/without a noexcept hash.
 */
template <typename T>
struct NotNoexceptHash {
    size_t operator()(const T& x) const /* not noexcept */
    {
        return std::hash<T>{}(x);
    }
};

/**
 * Generic struct with customizeable size and alignment.
 */
template <size_t ALIGNMENT, size_t SIZE>
struct alignas(ALIGNMENT) AlignedSize {
    char data[SIZE];
};

#endif // BITCOIN_TEST_NODE_ALLOCATOR_HELPERS_H
