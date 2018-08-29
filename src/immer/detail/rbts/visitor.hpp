//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <tuple>
#include <utility>

namespace immer {
namespace detail {
namespace rbts {

struct visitor_tag {};

template <typename... Fns>
using fn_visitor = std::tuple<visitor_tag, Fns...>;

template <typename... Fns>
auto make_visitor(Fns&& ...fns)
{
    return std::make_tuple(visitor_tag{}, std::forward<Fns>(fns)...);
}

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_relaxed(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_regular(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<2>(v)(v, std::forward<Args>(args)...); }

template <typename FnR, typename FnI, typename FnL, typename... Args>
decltype(auto) visit_leaf(fn_visitor<FnR, FnI, FnL> v, Args&& ...args)
{ return std::get<3>(v)(v, std::forward<Args>(args)...); }

template <typename FnI, typename FnL, typename... Args>
decltype(auto) visit_inner(fn_visitor<FnI, FnL> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename FnI, typename FnL, typename... Args>
decltype(auto) visit_leaf(fn_visitor<FnI, FnL> v, Args&& ...args)
{ return std::get<2>(v)(v, std::forward<Args>(args)...); }

template <typename Fn, typename... Args>
decltype(auto) visit_node(fn_visitor<Fn> v, Args&& ...args)
{ return std::get<1>(v)(v, std::forward<Args>(args)...); }

template <typename Visitor, typename... Args>
decltype(auto) visit_relaxed(Visitor&& v, Args&& ...args)
{
    return visit_inner(std::forward<Visitor>(v),
                       std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_regular(Visitor&& v, Args&& ...args)
{
    return visit_inner(std::forward<Visitor>(v),
                       std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_inner(Visitor&& v, Args&& ...args)
{
    return visit_node(std::forward<Visitor>(v),
                      std::forward<Args>(args)...);
}

template <typename Visitor, typename... Args>
decltype(auto) visit_leaf(Visitor&& v, Args&& ...args)
{
    return visit_node(std::forward<Visitor>(v),
                      std::forward<Args>(args)...);
}

} // namespace rbts
} // namespace detail
} // namespace immer
