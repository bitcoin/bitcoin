// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_TO_HEX_ARRAY_HPP
#define BOOST_STACKTRACE_DETAIL_TO_HEX_ARRAY_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/array.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/make_unsigned.hpp>

namespace boost { namespace stacktrace { namespace detail {

BOOST_STATIC_CONSTEXPR char to_hex_array_bytes[] = "0123456789ABCDEF";

template <class T>
inline boost::array<char, 2 + sizeof(void*) * 2 + 1> to_hex_array(T addr) BOOST_NOEXCEPT {
    boost::array<char, 2 + sizeof(void*) * 2 + 1> ret = {"0x"};
    ret.back() = '\0';
    BOOST_STATIC_ASSERT_MSG(!boost::is_pointer<T>::value, "");

    const std::size_t s = sizeof(T);

    char* out = ret.data() + s * 2 + 1;

    for (std::size_t i = 0; i < s; ++i) {
        const unsigned char tmp_addr = (addr & 0xFFu);
        *out = to_hex_array_bytes[tmp_addr & 0xF];
        -- out;
        *out = to_hex_array_bytes[tmp_addr >> 4];
        -- out;
        addr >>= 8;
    }

    return ret;
}

inline boost::array<char, 2 + sizeof(void*) * 2 + 1> to_hex_array(const void* addr) BOOST_NOEXCEPT {
    return to_hex_array(
        reinterpret_cast< boost::make_unsigned<std::ptrdiff_t>::type >(addr)
    );
}

}}} // namespace boost::stacktrace::detail

#endif // BOOST_STACKTRACE_DETAIL_TO_HEX_ARRAY_HPP
