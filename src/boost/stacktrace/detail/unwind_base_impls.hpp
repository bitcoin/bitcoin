// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_UNWIND_BASE_IMPLS_HPP
#define BOOST_STACKTRACE_DETAIL_UNWIND_BASE_IMPLS_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/stacktrace/frame.hpp>

namespace boost { namespace stacktrace { namespace detail {

struct to_string_using_nothing {
    std::string res;

    void prepare_function_name(const void* addr) {
        res = boost::stacktrace::frame(addr).name();
    }

    bool prepare_source_location(const void* /*addr*/) const BOOST_NOEXCEPT {
        return false;
    }
};

template <class Base> class to_string_impl_base;
typedef to_string_impl_base<to_string_using_nothing> to_string_impl;

inline std::string name_impl(const void* /*addr*/) {
    return std::string();
}

} // namespace detail

std::string frame::source_file() const {
    return std::string();
}

std::size_t frame::source_line() const {
    return 0;
}

}} // namespace boost::stacktrace

#endif // BOOST_STACKTRACE_DETAIL_UNWIND_BASE_IMPLS_HPP
