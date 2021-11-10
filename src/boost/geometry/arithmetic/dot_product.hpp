// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_DOT_PRODUCT_HPP
#define BOOST_GEOMETRY_ARITHMETIC_DOT_PRODUCT_HPP


#include <cstddef>

#include <boost/concept/requires.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename P1, typename P2, std::size_t Dimension, std::size_t DimensionCount>
struct dot_product_maker
{
    typedef typename select_coordinate_type<P1, P2>::type coordinate_type;

    static constexpr coordinate_type apply(P1 const& p1, P2 const& p2)
    {
        return get<Dimension>(p1) * get<Dimension>(p2)
            + dot_product_maker<P1, P2, Dimension+1, DimensionCount>::apply(p1, p2);
    }
};

template <typename P1, typename P2, std::size_t DimensionCount>
struct dot_product_maker<P1, P2, DimensionCount, DimensionCount>
{
    typedef typename select_coordinate_type<P1, P2>::type coordinate_type;

    static constexpr coordinate_type apply(P1 const& p1, P2 const& p2)
    {
        return get<DimensionCount>(p1) * get<DimensionCount>(p2);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
    \brief Computes the dot product (or scalar product) of 2 vectors (points).
    \ingroup arithmetic
    \tparam Point1 \tparam_point
    \tparam Point2 \tparam_point
    \param p1 first point
    \param p2 second point
    \return the dot product
    
    \qbk{[heading Examples]}
    \qbk{[dot_product] [dot_product_output]}

 */
template <typename Point1, typename Point2>
// workaround for VS2015
#if !defined(_MSC_VER) || (_MSC_VER >= 1910)
constexpr
#endif
inline typename select_coordinate_type<Point1, Point2>::type dot_product(
        Point1 const& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point1>) );
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point2>) );

    return detail::dot_product_maker
        <
            Point1, Point2,
            0, dimension<Point1>::type::value - 1
        >::apply(p1, p2);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ARITHMETIC_DOT_PRODUCT_HPP
