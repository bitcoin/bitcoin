//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/define.hpp>
#include <string>

namespace scm {
namespace detail {

template <typename Tag, int Seq=0>
struct definer
{
    using this_t = definer;
    using next_t = definer<Tag, Seq + 1>;

    std::string group_name_ = {};

    definer() = default;
    definer(definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    definer(definer<Tag, Seq2>)
    {}

    template <typename Fn>
    next_t define(std::string name, Fn fn) &&
    {
        define_impl<this_t>(name, fn);
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        define_impl<this_t>("make", fn);
        return { std::move(*this) };
    }
};

template <typename Tag, int Seq=0>
struct group_definer
{
    using this_t = group_definer;
    using next_t = group_definer<Tag, Seq + 1>;

    std::string group_name_ = {};

    group_definer(std::string name)
        : group_name_{std::move(name)} {}

    group_definer(group_definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    group_definer(group_definer<Tag, Seq2>)
    {}

    template <typename Fn>
    next_t define(std::string name, Fn fn) &&
    {
        define_impl<this_t>(group_name_ + "-" + name, fn);
        return { std::move(*this) };
    }
};

} // namespace detail

template <typename Tag=void>
detail::definer<Tag> group()
{
    return {};
}

template <typename Tag=void>
detail::group_definer<Tag> group(std::string name)
{
    return { std::move(name) };
}

} // namespace scm
