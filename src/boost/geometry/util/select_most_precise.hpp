// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_SELECT_MOST_PRECISE_HPP
#define BOOST_GEOMETRY_UTIL_SELECT_MOST_PRECISE_HPP


#include <type_traits>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL

namespace detail { namespace select_most_precise
{


// 0 - void
// 1 - integral
// 2 - floating point
// 3 - non-fundamental
template <typename T>
struct type_priority
    : std::conditional_t
        <
            std::is_void<T>::value,
            std::integral_constant<int, 0>,
            std::conditional_t
                <
                    std::is_fundamental<T>::value,
                    std::conditional_t
                        <
                            std::is_floating_point<T>::value,
                            std::integral_constant<int, 2>,
                            std::integral_constant<int, 1>
                        >,
                    std::integral_constant<int, 3>
                >
        >
{};


template <typename T>
struct type_size
    : std::integral_constant<std::size_t, sizeof(T)>
{};

template <>
struct type_size<void>
    : std::integral_constant<std::size_t, 0>
{};


}} // namespace detail::select_most_precise
#endif // DOXYGEN_NO_DETAIL


/*!
    \brief Meta-function to select the most accurate type for
        calculations
    \ingroup utility
    \details select_most_precise classes, compares types on compile time.
    For example, if an addition must be done with a double and an integer, the
        result must be a double.
    If both types are integer, the result can be an integer.
    \note It is different from the "promote" class, already in boost. That
        class promotes e.g. a (one) float to a double. This class selects a
        type from two types. It takes the most accurate, but does not promote
        afterwards.
    \note If the input is a non-fundamental type, it might be a calculation
        type such as a GMP-value or another high precision value. Therefore,
        if one is non-fundamental, that one is chosen.
    \note If both types are non-fundamental, the result is indeterminate and
        currently the first one is chosen.
*/
template <typename ...Types>
struct select_most_precise
{
    typedef void type;
};

template <typename T>
struct select_most_precise<T>
{
    typedef T type;
};

template <typename T1, typename T2>
struct select_most_precise<T1, T2>
{
    static const int priority1 = detail::select_most_precise::type_priority<T1>::value;
    static const int priority2 = detail::select_most_precise::type_priority<T2>::value;
    static const std::size_t size1 = detail::select_most_precise::type_size<T1>::value;
    static const std::size_t size2 = detail::select_most_precise::type_size<T2>::value;

    typedef std::conditional_t
        <
            (priority1 > priority2),
            T1,
            std::conditional_t
                <
                    (priority2 > priority1),
                    T2,
                    std::conditional_t // priority1 == priority2
                        <
                            (priority1 == 0 || priority1 == 3), // both void or non-fundamental
                            T1,
                            std::conditional_t // both fundamental
                                <
                                    (size2 > size1),
                                    T2,
                                    T1
                                >
                        >
                >
        > type;
};

template <typename T1, typename T2, typename ...Types>
struct select_most_precise<T1, T2, Types...>
{
    typedef typename select_most_precise
        <
            typename select_most_precise<T1, T2>::type,
            typename select_most_precise<Types...>::type
        >::type type;
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_SELECT_MOST_PRECISE_HPP
