//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdio>

namespace immer {

/*!
 * Appends a default constructed extra object of type `T` at the
 * *before* the requested region.
 *
 * @tparam T Type of the appended data.
 * @tparam Base Type of the parent heap.
 */
template <typename T, typename Base>
struct with_data : Base
{
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = base_t::allocate(size + sizeof(T), tags...);
        return new (p) T{} + 1;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* p, Tags... tags)
    {
        auto dp = static_cast<T*>(p) - 1;
        dp->~T();
        base_t::deallocate(size + sizeof(T), dp, tags...);
    }
};

} // namespace immer
