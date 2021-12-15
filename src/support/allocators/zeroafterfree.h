// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H
#define BITCOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H

#include <support/cleanse.h>

#include <memory>
#include <vector>

template <typename T>
struct zero_after_free_allocator : public std::allocator<T> {
    using base = std::allocator<T>;
    using traits = std::allocator_traits<base>;
    using size_type = typename traits::size_type;
    using difference_type = typename traits::difference_type;
    using pointer = typename traits::pointer;
    using const_pointer = typename traits::const_pointer;
    using value_type = typename traits::value_type;
    zero_after_free_allocator() noexcept {}
    zero_after_free_allocator(const zero_after_free_allocator& a) noexcept : base(a) {}
    template <typename U>
    zero_after_free_allocator(const zero_after_free_allocator<U>& a) noexcept : base(a)
    {
    }
    ~zero_after_free_allocator() noexcept {}
    template <typename _Other>
    struct rebind {
        typedef zero_after_free_allocator<_Other> other;
    };

    void deallocate(T* p, std::size_t n)
    {
        if (p != nullptr)
            memory_cleanse(p, sizeof(T) * n);
        std::allocator<T>::deallocate(p, n);
    }
};

/** Byte-vector that clears its contents before deletion. */
using SerializeData = std::vector<uint8_t, zero_after_free_allocator<uint8_t>>;

#endif // BITCOIN_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H
