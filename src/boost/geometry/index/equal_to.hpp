// Boost.Geometry Index
//
// Copyright (c) 2011-2016 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP
#define BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP

#include <boost/geometry/algorithms/detail/equals/interface.hpp>
#include <boost/geometry/index/indexable.hpp>

namespace boost { namespace geometry { namespace index { namespace detail
{

template <typename Geometry,
          typename Tag = typename geometry::tag<Geometry>::type>
struct equals
{
    template <typename Strategy>
    inline static bool apply(Geometry const& g1, Geometry const& g2, Strategy const&)
    {
        return geometry::equals(g1, g2);
    }
};

template <typename Geometry>
struct equals<Geometry, point_tag>
{
    inline static bool apply(Geometry const& g1, Geometry const& g2, default_strategy const&)
    {
        return geometry::equals(g1, g2);
    }

    template <typename Strategy>
    inline static bool apply(Geometry const& g1, Geometry const& g2, Strategy const& s)
    {
        return geometry::equals(g1, g2, s);
    }
};

template <typename Geometry>
struct equals<Geometry, box_tag>
{
    inline static bool apply(Geometry const& g1, Geometry const& g2, default_strategy const&)
    {
        return geometry::equals(g1, g2);
    }

    template <typename Strategy>
    inline static bool apply(Geometry const& g1, Geometry const& g2, Strategy const& s)
    {
        return geometry::equals(g1, g2, s);
    }
};

template <typename Geometry>
struct equals<Geometry, segment_tag>
{
    inline static bool apply(Geometry const& g1, Geometry const& g2, default_strategy const&)
    {
        return geometry::equals(g1, g2);
    }

    template <typename Strategy>
    inline static bool apply(Geometry const& g1, Geometry const& g2, Strategy const& s)
    {
        return geometry::equals(g1, g2, s);
    }
};


template <typename Geometry, typename Tag>
struct equals<Geometry *, Tag>
{
    template <typename Strategy>
    inline static bool apply(const Geometry * g1, const Geometry * g2, Strategy const&)
    {
        return g1 == g2;
    }
};

template <typename T>
struct equals<T, void>
{
    template <typename Strategy>
    inline static bool apply(T const& v1, T const& v2, Strategy const&)
    {
        return v1 == v2;
    }
};

template <typename T>
struct equals<T *, void>
{
    template <typename Strategy>
    inline static bool apply(const T * v1, const T * v2, Strategy const&)
    {
        return v1 == v2;
    }
};

template <typename Tuple, size_t I, size_t N>
struct tuple_equals
{
    template <typename Strategy>
    inline static bool apply(Tuple const& t1, Tuple const& t2, Strategy const& strategy)
    {
        typedef typename boost::tuples::element<I, Tuple>::type T;

        return equals<T>::apply(boost::get<I>(t1), boost::get<I>(t2), strategy)
            && tuple_equals<Tuple, I + 1, N>::apply(t1, t2, strategy);
    }
};

template <typename Tuple, size_t I>
struct tuple_equals<Tuple, I, I>
{
    template <typename Strategy>
    inline static bool apply(Tuple const&, Tuple const&, Strategy const&)
    {
        return true;
    }
};

// TODO: Consider this: Since equal_to<> is using geometry::equals() it's possible that
//       two compared Indexables are not exactly the same! They will be spatially equal
//       but not strictly equal. Consider 2 Segments with reversed order of points.
//       Therefore it's possible that during the Value removal different value will be
//       removed than the one that was passed.

/*!
\brief The function object comparing Values.

It compares Geometries using geometry::equals() function. Other types are compared using operator==.
The default version handles Values which are Indexables.
This template is also specialized for std::pair<T1, T2> and boost::tuple<...>.

\tparam Value       The type of objects which are compared by this function object.
\tparam IsIndexable If true, Values are compared using boost::geometry::equals() functions.
*/
template <typename Value,
          bool IsIndexable = is_indexable<Value>::value>
struct equal_to
{
    /*! \brief The type of result returned by function object. */
    typedef bool result_type;
    
    /*!
    \brief Compare values. If Value is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    template <typename Strategy>
    inline bool operator()(Value const& l, Value const& r, Strategy const& strategy) const
    {
        return detail::equals<Value>::apply(l, r, strategy);
    }
};

/*!
\brief The function object comparing Values.

This specialization compares values of type std::pair<T1, T2>.
It compares pairs' first values, then second values.

\tparam T1       The first type.
\tparam T2       The second type.
*/
template <typename T1, typename T2>
struct equal_to<std::pair<T1, T2>, false>
{
    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If pair<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    template <typename Strategy>
    inline bool operator()(std::pair<T1, T2> const& l, std::pair<T1, T2> const& r,
                           Strategy const& strategy) const
    {
        return detail::equals<T1>::apply(l.first, r.first, strategy)
            && detail::equals<T2>::apply(l.second, r.second, strategy);
    }
};

/*!
\brief The function object comparing Values.

This specialization compares values of type boost::tuple<...>.
It compares all members of the tuple from the first one to the last one.
*/
template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
struct equal_to<boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>, false>
{
    typedef boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> value_type;

    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If tuple<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    template <typename Strategy>
    inline bool operator()(value_type const& l, value_type const& r,
                           Strategy const& strategy) const
    {
        return detail::tuple_equals<
            value_type, 0, boost::tuples::length<value_type>::value
        >::apply(l, r, strategy);
    }
};

}}}} // namespace boost::geometry::index::detail

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#include <tuple>

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename Tuple, size_t I, size_t N>
struct std_tuple_equals
{
    template <typename Strategy>
    inline static bool apply(Tuple const& t1, Tuple const& t2, Strategy const& strategy)
    {
        typedef typename std::tuple_element<I, Tuple>::type T;

        return equals<T>::apply(std::get<I>(t1), std::get<I>(t2), strategy)
            && std_tuple_equals<Tuple, I + 1, N>::apply(t1, t2, strategy);
    }
};

template <typename Tuple, size_t I>
struct std_tuple_equals<Tuple, I, I>
{
    template <typename Strategy>
    inline static bool apply(Tuple const&, Tuple const&, Strategy const&)
    {
        return true;
    }
};

/*!
\brief The function object comparing Values.

This specialization compares values of type std::tuple<Args...>.
It's defined if the compiler supports tuples and variadic templates.
It compares all members of the tuple from the first one to the last one.
*/
template <typename ...Args>
struct equal_to<std::tuple<Args...>, false>
{
    typedef std::tuple<Args...> value_type;

    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If tuple<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    template <typename Strategy>
    bool operator()(value_type const& l, value_type const& r, Strategy const& strategy) const
    {
        return detail::std_tuple_equals<
            value_type, 0, std::tuple_size<value_type>::value
        >::apply(l, r, strategy);
    }
};

}}}} // namespace boost::geometry::index::detail

#endif // !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

namespace boost { namespace geometry { namespace index {

/*!
\brief The function object comparing Values.

The default version handles Values which are Indexables, std::pair<T1, T2>, boost::tuple<...>
and std::tuple<...> if STD tuples and variadic templates are supported.
All members are compared from left to right, Geometries using boost::geometry::equals() function,
other types using operator==.

\tparam Value       The type of objects which are compared by this function object.
*/
template <typename Value>
struct equal_to
    : detail::equal_to<Value>
{
    /*! \brief The type of result returned by function object. */
    typedef typename detail::equal_to<Value>::result_type result_type;
    
    /*!
    \brief Compare Values.
    
    \param l First value.
    \param r Second value.
    \return true if Values are equal.
    */
    inline bool operator()(Value const& l, Value const& r) const
    {
        return detail::equal_to<Value>::operator()(l, r, default_strategy());
    }

    template <typename Strategy>
    inline bool operator()(Value const& l, Value const& r, Strategy const& strategy) const
    {
        return detail::equal_to<Value>::operator()(l, r, strategy);
    }
};

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP
