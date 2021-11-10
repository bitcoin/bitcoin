// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_DETAIL_IS_STRING_HPP
#define BOOST_CONVERT_DETAIL_IS_STRING_HPP

#include <boost/convert/detail/range.hpp>

namespace boost { namespace cnv
{
    namespace detail
    {
        template<typename T, bool is_range_class> struct is_string : std::false_type {};

        template<typename T> struct is_string<T*, false>
        {
            static bool BOOST_CONSTEXPR_OR_CONST value = cnv::is_char<T>::value;
        };
        template <typename T, std::size_t N> struct is_string<T [N], false>
        {
            static bool BOOST_CONSTEXPR_OR_CONST value = cnv::is_char<T>::value;
        };
        template<typename T> struct is_string<T, /*is_range_class=*/true>
        {
            static bool BOOST_CONSTEXPR_OR_CONST value = cnv::is_char<typename T::value_type>::value;
        };
    }
    template<typename T>
    struct is_string : detail::is_string<
        typename std::remove_const<T>::type,
        std::is_class<T>::value && cnv::is_range<T>::value>
    {};
}}

#endif // BOOST_CONVERT_DETAIL_IS_STRING_HPP
