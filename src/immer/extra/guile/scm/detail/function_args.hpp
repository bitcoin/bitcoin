//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/pack.hpp>
#include <boost/callable_traits/args.hpp>

namespace scm {
namespace detail {

template <typename Fn>
using function_args_t = boost::callable_traits::args_t<Fn, pack>;

} // namespace detail
} // namespace scm
