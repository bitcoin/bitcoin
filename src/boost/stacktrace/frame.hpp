// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_FRAME_HPP
#define BOOST_STACKTRACE_FRAME_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <iosfwd>
#include <string>

#include <boost/core/explicit_operator_bool.hpp>

#include <boost/stacktrace/safe_dump_to.hpp> // boost::stacktrace::detail::native_frame_ptr_t

#include <boost/stacktrace/detail/frame_decl.hpp>
#include <boost/stacktrace/detail/push_options.h>

namespace boost { namespace stacktrace {

/// Comparison operators that provide platform dependant ordering and have O(1) complexity; are Async-Handler-Safe.
BOOST_CONSTEXPR inline bool operator< (const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return lhs.address() < rhs.address(); }
BOOST_CONSTEXPR inline bool operator> (const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return rhs < lhs; }
BOOST_CONSTEXPR inline bool operator<=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs > rhs); }
BOOST_CONSTEXPR inline bool operator>=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs < rhs); }
BOOST_CONSTEXPR inline bool operator==(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return lhs.address() == rhs.address(); }
BOOST_CONSTEXPR inline bool operator!=(const frame& lhs, const frame& rhs) BOOST_NOEXCEPT { return !(lhs == rhs); }

/// Fast hashing support, O(1) complexity; Async-Handler-Safe.
inline std::size_t hash_value(const frame& f) BOOST_NOEXCEPT {
    return reinterpret_cast<std::size_t>(f.address());
}

/// Outputs stacktrace::frame in a human readable format to string; unsafe to use in async handlers.
BOOST_STACKTRACE_FUNCTION std::string to_string(const frame& f);

/// Outputs stacktrace::frame in a human readable format to output stream; unsafe to use in async handlers.
template <class CharT, class TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, const frame& f) {
    return os << boost::stacktrace::to_string(f);
}

}} // namespace boost::stacktrace

/// @cond

#include <boost/stacktrace/detail/pop_options.h>

#ifndef BOOST_STACKTRACE_LINK
#   if defined(BOOST_STACKTRACE_USE_NOOP)
#       include <boost/stacktrace/detail/frame_noop.ipp>
#   elif defined(BOOST_MSVC) || defined(BOOST_STACKTRACE_USE_WINDBG) || defined(BOOST_STACKTRACE_USE_WINDBG_CACHED)
#       include <boost/stacktrace/detail/frame_msvc.ipp>
#   else
#       include <boost/stacktrace/detail/frame_unwind.ipp>
#   endif
#endif
/// @endcond


#endif // BOOST_STACKTRACE_FRAME_HPP
