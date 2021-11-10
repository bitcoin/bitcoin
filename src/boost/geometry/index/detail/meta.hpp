// Boost.Geometry Index
//
// Copyright (c) 2011-2019 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_META_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_META_HPP

#include <type_traits>

//#include <boost/range/value_type.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

//template <typename T, typename V, bool IsRange = range::detail::is_range<T>::value>
//struct is_range_of_convertible_values_impl
//    : std::is_convertible<typename ::boost::range_value<T>::type, V>
//{};
//
//template <typename T, typename V>
//struct is_range_of_convertible_values_impl<T, V, false>
//    : std::integral_constant<bool, false>
//{};
//
//template <typename T, typename V>
//struct is_range_of_convertible_values
//    : is_range_of_convertible_values_impl<T, V>
//{};

// Implemented this way in order to prevent instantiation of all type traits at
// once because some of them are causing problems with gcc 4.6 namely
// is_convertible<bg::model::segment<>, std::pair<bg::model::segment<>, T> >
// because segment<> is derived from pair<> and pair<> has copy ctor taking
// other pair<> of any types the compiler tries to instantiate ctor of
// pair<segment, T> taking pair<point, point> which results in instantiation of
// segment's ctor taking a point which results in compilation error.
// This is probably compiler's bug.
template <typename T, typename Value, typename Indexable, typename ResultType, int Ver>
struct convertible_type_impl
{
    typedef ResultType type;
};

template <typename T, typename Value, typename Indexable>
struct convertible_type_impl<T, Value, Indexable, void, 0>
{
    typedef std::conditional_t
        <
            std::is_convertible<T, Indexable>::value,
            Indexable,
            void
        > result_type;

    typedef typename convertible_type_impl
        <
            T, Value, Indexable, result_type, 1
        >::type type;
};

template <typename T, typename Value, typename Indexable>
struct convertible_type_impl<T, Value, Indexable, void, 1>
{
    typedef std::conditional_t
        <
            std::is_convertible<T, Value>::value,
            Value,
            void
        > type;
};

template <typename T, typename Value, typename Indexable>
struct convertible_type
{
    typedef std::conditional_t
        <
            std::is_same<T, Value>::value,
            Value,
            std::conditional_t
                <
                    std::is_same<T, Indexable>::value,
                    Indexable,
                    void
                >
        > result_type;

    typedef typename convertible_type_impl
        <
            T, Value, Indexable, result_type, 0
        >::type type;
};

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_META_HPP
