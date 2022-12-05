//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/convert.hpp>

namespace scm {
namespace detail {

template <typename T>
struct convert_wrapper_type
{
    static T to_cpp(SCM v) { return T{v}; }
    static SCM to_scm(T v) { return v.get(); }
};

struct wrapper
{
    wrapper() = default;
    wrapper(SCM hdl) : handle_{hdl} {}
    SCM get() const { return handle_; }
    operator SCM () const { return handle_; }

    bool operator==(wrapper other) { return handle_ == other.handle_; }
    bool operator!=(wrapper other) { return handle_ != other.handle_; }

protected:
    SCM handle_ = SCM_UNSPECIFIED;
};

} // namespace detail

struct val : detail::wrapper
{
    using base_t = detail::wrapper;
    using base_t::base_t;

    template <typename T,
              typename = std::enable_if_t<
                  (!std::is_same<std::decay_t<T>, val>{} &&
                   !std::is_same<std::decay_t<T>, SCM>{})>>
    val(T&& x)
        : base_t(detail::to_scm(std::forward<T>(x)))
    {}

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<T, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator T() const { return detail::to_cpp<T>(handle_); }

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<T&, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator T& () const { return detail::to_cpp<T>(handle_); }

    template <typename T,
              typename = std::enable_if_t<
                  std::is_same<const T&, decltype(detail::to_cpp<T>(SCM{}))>{}>>
    operator const T& () const { return detail::to_cpp<T>(handle_); }

    val operator() () const
    { return val{scm_call_0(get())}; }
    val operator() (val a0) const
    { return val{scm_call_1(get(), a0)}; }
    val operator() (val a0, val a1) const
    { return val{scm_call_2(get(), a0, a1)}; }
    val operator() (val a0, val a1, val a3) const
    { return val{scm_call_3(get(), a0, a1, a3)}; }
};

} // namespace scm

#define SCM_DECLARE_WRAPPER_TYPE(cpp_name__)                            \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__>                                          \
        : convert_wrapper_type<cpp_name__> {};                          \
    }} /* namespace scm::detail */                                      \
    /**/

SCM_DECLARE_WRAPPER_TYPE(val);
