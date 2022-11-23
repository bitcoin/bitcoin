//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/invoke.hpp>
#include <scm/detail/function_args.hpp>
#include <scm/detail/convert.hpp>

namespace scm {
namespace detail {
// this anonymous namespace should help avoiding registration clashes
// among translation units.
namespace {

template <typename Tag, typename Fn>
auto finalizer_wrapper_impl(Fn fn, pack<>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] { invoke(fn_); };
}
template <typename Tag, typename Fn, typename T1>
auto finalizer_wrapper_impl(Fn fn, pack<T1>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (SCM a1) { invoke(fn_, to_cpp<T1>(a1)); };
}
template <typename Tag, typename Fn, typename T1, typename T2>
auto finalizer_wrapper_impl(Fn fn, pack<T1, T2>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (SCM a1, SCM a2) {
        invoke(fn_, to_cpp<T1>(a1), to_cpp<T2>(a2));
    };
}
template <typename Tag, typename Fn, typename T1, typename T2, typename T3>
auto finalizer_wrapper_impl(Fn fn, pack<T1, T2, T3>)
{
    check_call_once<Tag, Fn>();
    static const Fn fn_ = fn;
    return [] (SCM a1, SCM a2, SCM a3) {
        invoke(fn_, to_cpp<T1>(a1), to_cpp<T2>(a2), to_cpp<T3>(a3));
    };
}

template <typename Tag, typename Fn>
auto finalizer_wrapper(Fn fn)
{
    return finalizer_wrapper_impl<Tag>(fn, function_args_t<Fn>{});
}

} // anonymous namespace
} // namespace detail
} // namespace scm
