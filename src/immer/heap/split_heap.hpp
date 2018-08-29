//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <atomic>
#include <cassert>

namespace immer {

/*!
 * Adaptor that uses `SmallHeap` for allocations that are smaller or
 * equal to `Size` and `BigHeap` otherwise.
 */
template <std::size_t Size, typename SmallHeap, typename BigHeap>
struct split_heap
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        return size <= Size
            ? SmallHeap::allocate(size, tags...)
            : BigHeap::allocate(size, tags...);
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        if (size <= Size)
            SmallHeap::deallocate(size, data, tags...);
        else
            BigHeap::deallocate(size, data, tags...);
    }
};

} // namespace immer
