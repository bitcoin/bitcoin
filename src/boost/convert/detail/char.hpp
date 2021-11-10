// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_DETAIL_IS_CHAR_HPP
#define BOOST_CONVERT_DETAIL_IS_CHAR_HPP

#include <boost/convert/detail/config.hpp>
#include <cctype>
#include <cwctype>

namespace boost { namespace cnv
{
    using  char_type = char;
    using uchar_type = unsigned char;
    using wchar_type = wchar_t;

    namespace detail
    {
        template<typename> struct is_char             : std::false_type {};
        template<>         struct is_char< char_type> : std:: true_type {};
        template<>         struct is_char<wchar_type> : std:: true_type {};
    }
    template <typename T> struct is_char : detail::is_char<typename boost::remove_const<T>::type> {};

    template<typename char_type> inline bool      is_space(char_type);
    template<typename char_type> inline char_type to_upper(char_type);

    template<> inline bool       is_space ( char_type c) { return bool(std::isspace(static_cast<uchar_type>(c))); }
    template<> inline bool       is_space (uchar_type c) { return bool(std::isspace(c)); }
    template<> inline bool       is_space (wchar_type c) { return bool(std::iswspace(c)); }
    template<> inline  char_type to_upper ( char_type c) { return std::toupper(static_cast<uchar_type>(c)); }
    template<> inline uchar_type to_upper (uchar_type c) { return std::toupper(c); }
    template<> inline wchar_type to_upper (wchar_type c) { return std::towupper(c); }
}}

#endif // BOOST_CONVERT_DETAIL_IS_CHAR_HPP

