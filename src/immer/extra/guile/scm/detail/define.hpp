//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#ifndef SCM_AUTO_EXPORT
#define SCM_AUTO_EXPORT 1
#endif

#include <scm/detail/subr_wrapper.hpp>
#include <scm/list.hpp>

namespace scm {
namespace detail {

template <typename Tag, typename Fn>
static void define_impl(const std::string& name, Fn fn)
{
    using args_t = function_args_t<Fn>;
    constexpr auto args_size = pack_size_v<args_t>;
    constexpr auto has_rest  = std::is_same<pack_last_t<args_t>, scm::args>{};
    constexpr auto arg_count = args_size - has_rest;
    auto subr = (scm_t_subr) +subr_wrapper_aux<Tag>(fn, args_t{});
    scm_c_define_gsubr(name.c_str(), arg_count, 0, has_rest, subr);
#if SCM_AUTO_EXPORT
    scm_c_export(name.c_str());
#endif
}

} // namespace detail
} // namespace scm
