#ifndef BOOST_ENDIAN_DETAIL_IS_SCOPED_ENUM_HPP_INCLUDED
#define BOOST_ENDIAN_DETAIL_IS_SCOPED_ENUM_HPP_INCLUDED

// Copyright 2020 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost
{
namespace endian
{
namespace detail
{

template<class T> struct negation: boost::integral_constant<bool, !T::value> {};

template<class T> struct is_scoped_enum:
    boost::conditional<
        boost::is_enum<T>::value,
        negation< boost::is_convertible<T, int> >,
        boost::false_type
    >::type
{
};

} // namespace detail
} // namespace endian
} // namespace boost

#endif  // BOOST_ENDIAN_DETAIL_IS_SCOPED_ENUM_HPP_INCLUDED
