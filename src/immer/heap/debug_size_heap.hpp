//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cassert>
#include <cstddef>
#include <immer/config.hpp>
#include <immer/heap/identity_heap.hpp>

namespace immer {

#if IMMER_ENABLE_DEBUG_SIZE_HEAP

/*!
 * A heap that in debug mode ensures that the sizes for allocation and
 * deallocation do match.
 */
template <typename Base>
struct debug_size_heap
{
#if defined(__MINGW32__) && !defined(__MINGW64__)
    // There is a bug in MinGW 32bit: https://sourceforge.net/p/mingw-w64/bugs/778/
    // It causes different versions of std::max_align_t to be defined, depending on inclusion order of stddef.h
    // and stdint.h. As we have no control over the inclusion order here (as it might be set in stone by the outside
    // world), we can't easily pin it to one of both versions of std::max_align_t. This means, we have to hardcode
    // extra_size for MinGW 32bit builds until the mentioned bug is fixed.
    constexpr static auto extra_size = 8;
#else
    constexpr static auto extra_size = sizeof(
        std::aligned_storage_t<sizeof(std::size_t),
                               alignof(std::max_align_t)>);
#endif

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = (std::size_t*) Base::allocate(size + extra_size, tags...);
        new (p) std::size_t{ size };
        return ((char*)p) + extra_size;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        auto p = (std::size_t*) (((char*) data) - extra_size);
        assert(*p == size);
        Base::deallocate(size + extra_size, p, tags...);
    }
};

#else // IMMER_ENABLE_DEBUG_SIZE_HEAP

template <typename Base>
using debug_size_heap = identity_heap<Base>;

#endif // !IMMER_ENABLE_DEBUG_SIZE_HEAP

} // namespace immer
