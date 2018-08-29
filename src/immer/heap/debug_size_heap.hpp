//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/heap/identity_heap.hpp>
#include <cassert>

namespace immer {

#if IMMER_ENABLE_DEBUG_SIZE_HEAP

/*!
 * A heap that in debug mode ensures that the sizes for allocation and
 * deallocation do match.
 */
template <typename Base>
struct debug_size_heap
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = (std::size_t*) Base::allocate(size + sizeof(std::size_t), tags...);
        *p = size;
        return p + 1;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        auto p = ((std::size_t*) data) - 1;
        assert(*p == size);
        Base::deallocate(size + sizeof(std::size_t), p, tags...);
    }
};

#else // IMMER_ENABLE_DEBUG_SIZE_HEAP

template <typename Base>
using debug_size_heap = identity_heap<Base>;

#endif // !IMMER_ENABLE_DEBUG_SIZE_HEAP

} // namespace immer
