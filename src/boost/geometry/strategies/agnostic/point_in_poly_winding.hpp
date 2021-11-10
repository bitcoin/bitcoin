// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/cartesian/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/spherical/point_in_poly_winding.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template
<
    typename Point,
    typename PointOfSegment,
    typename CalculationType,
    typename CSTag = typename tag_cast
                        <
                            typename cs_tag<Point>::type,
                            spherical_tag
                        >::type
>
struct winding_base_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this coordinate system.",
        Point, PointOfSegment, CSTag);
};

template <typename Point, typename PointOfSegment, typename CalculationType>
struct winding_base_type<Point, PointOfSegment, CalculationType, cartesian_tag>
{
    typedef within::cartesian_winding<void, void, CalculationType> type;
};

template <typename Point, typename PointOfSegment, typename CalculationType>
struct winding_base_type<Point, PointOfSegment, CalculationType, spherical_tag>
{
    typedef within::detail::spherical_winding_base
        <
            typename strategy::side::services::default_strategy
                <
                    typename cs_tag<Point>::type
                >::type,
            CalculationType
        > type;
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Within detection using winding rule. Side strategy used internally is
       choosen based on Point's coordinate system.
\ingroup strategies
\tparam Point \tparam_point
\tparam PointOfSegment \tparam_segment_point
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    typename Point,
    typename PointOfSegment = Point,
    typename CalculationType = void
>
class winding
    : public within::detail::winding_base_type
        <
            Point, PointOfSegment, CalculationType
        >::type
{
    typedef typename within::detail::winding_base_type
        <
            Point, PointOfSegment, CalculationType
        >::type base_t;

public:
    winding() {}

    template <typename Model>
    explicit winding(Model const& model)
        : base_t(model)
    {}
};


}} // namespace strategy::within


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP
