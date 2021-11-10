// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_STACKTRACE_FWD_HPP
#define BOOST_STACKTRACE_STACKTRACE_FWD_HPP

#include <cstddef>
#include <memory>

/// @file stacktrace_fwd.hpp This header contains only forward declarations of
/// boost::stacktrace::frame, boost::stacktrace::basic_stacktrace, boost::stacktrace::stacktrace
/// and does not include any other Boost headers.

/// @cond
namespace boost { namespace stacktrace {

class frame;
template <class Allocator = std::allocator<frame> > class basic_stacktrace;
typedef basic_stacktrace<> stacktrace;

}} // namespace boost::stacktrace
/// @endcond


#endif // BOOST_STACKTRACE_STACKTRACE_FWD_HPP
