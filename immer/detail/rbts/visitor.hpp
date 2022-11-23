//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>

#include <tuple>
#include <utility>

namespace immer {
namespace detail {
namespace rbts {

template <typename Deriv>
struct visitor_base
{
    template <typename... Args>
    static decltype(auto) visit_node(Args&&... args)
    {
        IMMER_UNREACHABLE;
    }

    template <typename... Args>
    static decltype(auto) visit_relaxed(Args&&... args)
    {
        return Deriv::visit_inner(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static decltype(auto) visit_regular(Args&&... args)
    {
        return Deriv::visit_inner(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static decltype(auto) visit_inner(Args&&... args)
    {
        return Deriv::visit_node(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static decltype(auto) visit_leaf(Args&&... args)
    {
        return Deriv::visit_node(std::forward<Args>(args)...);
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
