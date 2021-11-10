// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_IO_HPP
#define BOOST_PFR_DETAIL_IO_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <iosfwd>       // stream operators
#include <iomanip>

#if defined(__has_include)
#   if __has_include(<string_view>) && BOOST_PFR_USE_CPP17
#       include <string_view>
#   endif
#endif

namespace boost { namespace pfr { namespace detail {

inline auto quoted_helper(const std::string& s) noexcept {
    return std::quoted(s);
}

#if defined(__has_include)
#   if __has_include(<string_view>) && BOOST_PFR_USE_CPP17
template <class CharT, class Traits>
inline auto quoted_helper(std::basic_string_view<CharT, Traits> s) noexcept {
    return std::quoted(s);
}
#   endif
#endif

inline auto quoted_helper(std::string& s) noexcept {
    return std::quoted(s);
}

template <class T>
inline decltype(auto) quoted_helper(T&& v) noexcept {
    return std::forward<T>(v);
}

template <std::size_t I, std::size_t N>
struct print_impl {
    template <class Stream, class T>
    static void print (Stream& out, const T& value) {
        if (!!I) out << ", ";
        out << detail::quoted_helper(boost::pfr::detail::sequence_tuple::get<I>(value));
        print_impl<I + 1, N>::print(out, value);
    }
};

template <std::size_t I>
struct print_impl<I, I> {
    template <class Stream, class T> static void print (Stream&, const T&) noexcept {}
};


template <std::size_t I, std::size_t N>
struct read_impl {
    template <class Stream, class T>
    static void read (Stream& in, const T& value) {
        char ignore = {};
        if (!!I) {
            in >> ignore;
            if (ignore != ',') in.setstate(Stream::failbit);
            in >> ignore;
            if (ignore != ' ')  in.setstate(Stream::failbit);
        }
        in >> detail::quoted_helper( boost::pfr::detail::sequence_tuple::get<I>(value) );
        read_impl<I + 1, N>::read(in, value);
    }
};

template <std::size_t I>
struct read_impl<I, I> {
    template <class Stream, class T> static void read (Stream&, const T&) {}
};

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_IO_HPP
