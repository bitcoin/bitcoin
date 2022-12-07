//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/util.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

#include <libguile.h>

namespace scm {
namespace detail {

template <typename T, typename Enable=void>
struct convert;

template <typename T>
auto to_scm(T&& v)
    -> SCM_DECLTYPE_RETURN(
        convert<std::decay_t<T>>::to_scm(std::forward<T>(v)));

template <typename T>
auto to_cpp(SCM v)
    -> SCM_DECLTYPE_RETURN(
        convert<std::decay_t<T>>::to_cpp(v));

} // namespace detail
} // namespace scm

#define SCM_DECLARE_NUMERIC_TYPE(cpp_name__, scm_name__)                \
    namespace scm {                                                     \
    namespace detail {                                                  \
    template <>                                                         \
    struct convert<cpp_name__> {                                        \
        static cpp_name__ to_cpp(SCM v) { return scm_to_ ## scm_name__(v); } \
        static SCM to_scm(cpp_name__ v) { return scm_from_ ## scm_name__(v); } \
    };                                                                  \
    }} /* namespace scm::detail */                                      \
    /**/

SCM_DECLARE_NUMERIC_TYPE(float,         double);
SCM_DECLARE_NUMERIC_TYPE(double,        double);
SCM_DECLARE_NUMERIC_TYPE(std::int8_t,   int8);
SCM_DECLARE_NUMERIC_TYPE(std::int16_t,  int16);
SCM_DECLARE_NUMERIC_TYPE(std::int32_t,  int32);
SCM_DECLARE_NUMERIC_TYPE(std::int64_t,  int64);
SCM_DECLARE_NUMERIC_TYPE(std::uint8_t,  uint8);
SCM_DECLARE_NUMERIC_TYPE(std::uint16_t, uint16);
SCM_DECLARE_NUMERIC_TYPE(std::uint32_t, uint32);
SCM_DECLARE_NUMERIC_TYPE(std::uint64_t, uint64);
