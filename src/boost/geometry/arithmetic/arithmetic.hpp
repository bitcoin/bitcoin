// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2021.
// Modifications copyright (c) 2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP
#define BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP

#include <functional>

#include <boost/call_traits.hpp>
#include <boost/concept/requires.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/util/algorithm.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Point>
struct param
{
    typedef typename boost::call_traits
        <
            typename coordinate_type<Point>::type
        >::param_type type;
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

/*!
    \brief Adds the same value to each coordinate of a point
    \ingroup arithmetic
    \details
    \tparam Point \tparam_point
    \param p point
    \param value value to add
 */
template <typename Point>
inline void add_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::for_each_dimension<Point>([&](auto index)
    {
        set<index>(p, get<index>(p) + value);
    });
}

/*!
    \brief Adds a point to another
    \ingroup arithmetic
    \details The coordinates of the second point will be added to those of the first point.
             The second point is not modified.
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void add_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    detail::for_each_dimension<Point1>([&](auto index)
    {
        using calc_t = typename select_coordinate_type<Point1, Point2>::type;
        set<index>(p1, calc_t(get<index>(p1)) + calc_t(get<index>(p2)));
    });
}

/*!
    \brief Subtracts the same value to each coordinate of a point
    \ingroup arithmetic
    \details
    \tparam Point \tparam_point
    \param p point
    \param value value to subtract
 */
template <typename Point>
inline void subtract_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::for_each_dimension<Point>([&](auto index)
    {
        set<index>(p, get<index>(p) - value);
    });
}

/*!
    \brief Subtracts a point to another
    \ingroup arithmetic
    \details The coordinates of the second point will be subtracted to those of the first point.
             The second point is not modified.
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void subtract_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    detail::for_each_dimension<Point1>([&](auto index)
    {
        using calc_t = typename select_coordinate_type<Point1, Point2>::type;
        set<index>(p1, calc_t(get<index>(p1)) - calc_t(get<index>(p2)));
    });
}

/*!
    \brief Multiplies each coordinate of a point by the same value
    \ingroup arithmetic
    \details
    \tparam Point \tparam_point
    \param p point
    \param value value to multiply by
 */
template <typename Point>
inline void multiply_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::for_each_dimension<Point>([&](auto index)
    {
        set<index>(p, get<index>(p) * value);
    });
}

/*!
    \brief Multiplies a point by another
    \ingroup arithmetic
    \details The coordinates of the first point will be multiplied by those of the second point.
             The second point is not modified.
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
    \note This is *not* a dot, cross or wedge product. It is a mere field-by-field multiplication.
 */
template <typename Point1, typename Point2>
inline void multiply_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    detail::for_each_dimension<Point1>([&](auto index)
    {
        using calc_t = typename select_coordinate_type<Point1, Point2>::type;
        set<index>(p1, calc_t(get<index>(p1)) * calc_t(get<index>(p2)));
    });
}

/*!
    \brief Divides each coordinate of the same point by a value
    \ingroup arithmetic
    \details
    \tparam Point \tparam_point
    \param p point
    \param value value to divide by
 */
template <typename Point>
inline void divide_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::for_each_dimension<Point>([&](auto index)
    {
        set<index>(p, get<index>(p) / value);
    });
}

/*!
    \brief Divides a point by another
    \ingroup arithmetic
    \details The coordinates of the first point will be divided by those of the second point.
             The second point is not modified.
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void divide_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    detail::for_each_dimension<Point1>([&](auto index)
    {
        using calc_t = typename select_coordinate_type<Point1, Point2>::type;
        set<index>(p1, calc_t(get<index>(p1)) / calc_t(get<index>(p2)));
    });
}

/*!
    \brief Assign each coordinate of a point the same value
    \ingroup arithmetic
    \details
    \tparam Point \tparam_point
    \param p point
    \param value value to assign
 */
template <typename Point>
inline void assign_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::for_each_dimension<Point>([&](auto index)
    {
        set<index>(p, value);
    });
}

/*!
    \brief Assign a point with another
    \ingroup arithmetic
    \details The coordinates of the first point will be assigned those of the second point.
             The second point is not modified.
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void assign_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    detail::for_each_dimension<Point1>([&](auto index)
    {
        set<index>(p1, get<index>(p2));
    });
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP
