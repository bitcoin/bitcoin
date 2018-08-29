//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdlib>

namespace immer {

/*!
 * A heap that simply passes on to the parent heap.
 */
template <typename Base>
struct identity_heap : Base
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        return Base::allocate(size, tags...);
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags... tags)
    {
        Base::deallocate(size, data, tags...);
    }
};

} // namespace immer
