//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>

#include <cassert>
#include <cstdlib>
#include <exception>
#include <memory>

namespace immer {

/*!
 * A heap that uses `std::malloc` and `std::free` to manage memory.
 */
struct malloc_heap
{
    /*!
     * Returns a pointer to a memory region of size `size`, if the
     * allocation was successful and throws `std::bad_alloc` otherwise.
     */
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        auto p = std::malloc(size);
        if (IMMER_UNLIKELY(!p))
            IMMER_THROW(std::bad_alloc{});
        return p;
    }

    /*!
     * Releases a memory region `data` that was previously returned by
     * `allocate`.  One must not use nor deallocate again a memory
     * region that once it has been deallocated.
     */
    static void deallocate(std::size_t, void* data) { std::free(data); }
};

} // namespace immer
