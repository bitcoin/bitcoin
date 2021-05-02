// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H
#define XBIT_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H

#include <support/cleanse.h>

#include <memory>
#include <vector>

template <typename T>
struct zero_after_free_allocator : public std::allocator<T> {
    // MSVC8 default copy constructor is broken
    typedef std::allocator<T> base;
    typedef typename base::size_type size_type;
    typedef typename base::difference_type difference_type;
    typedef typename base::pointer pointer;
    typedef typename base::const_pointer const_pointer;
    typedef typename base::reference reference;
    typedef typename base::const_reference const_reference;
    typedef typename base::value_type value_type;
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

// Byte-vector that clears its contents before deletion.
typedef std::vector<char, zero_after_free_allocator<char> > CSerializeData;

#endif // XBIT_SUPPORT_ALLOCATORS_ZEROAFTERFREE_H
