//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstddef>
#include <memory>

namespace immer {

/*!
 * A heap that uses `operator new` and `operator delete`.
 */
struct cpp_heap
{
    /*!
     * Returns a pointer to a memory region of size `size`, if the
     * allocation was successful, and throws otherwise.
     */
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return ::operator new(size);
    }

    /*!
     * Releases a memory region `data` that was previously returned by
     * `allocate`.  One must not use nor deallocate again a memory
     * region that once it has been deallocated.
     */
    static void deallocate(std::size_t size, void* data)
    {
        ::operator delete(data);
    }
};

} // namespace immer
