// Boost.Geometry

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_AZIMUTH_HPP
#define BOOST_GEOMETRY_ALGORITHMS_AZIMUTH_HPP


#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/azimuth/cartesian.hpp>
#include <boost/geometry/strategies/azimuth/geographic.hpp>
#include <boost/geometry/strategies/azimuth/spherical.hpp>

#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{
       
} // namespace detail
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1, typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type
>
struct azimuth : not_implemented<Tag1, Tag2>
{};

template <typename Point1, typename Point2>
struct azimuth<Point1, Point2, point_tag, point_tag>
{
    template <typename Strategy>
    static auto apply(Point1 const& p1, Point2 const& p2, Strategy const& strategy)
    {
        typedef typename decltype(strategy.azimuth())::template result_type
            <
                typename coordinate_type<Point1>::type,
                typename coordinate_type<Point2>::type
            >::type calc_t;

        calc_t result = 0;
        calc_t const x1 = geometry::get_as_radian<0>(p1);
        calc_t const y1 = geometry::get_as_radian<1>(p1);
        calc_t const x2 = geometry::get_as_radian<0>(p2);
        calc_t const y2 = geometry::get_as_radian<1>(p2);

        strategy.azimuth().apply(x1, y1, x2, y2, result);

        // NOTE: It is not clear which units we should use for the result.
        //   For now radians are always returned but a user could expect
        //   e.g. something like this:
        /*
        bool const both_degree = std::is_same
                <
                    typename detail::cs_angular_units<Point1>::type,
                    geometry::degree
                >::value
            && std::is_same
                <
                    typename detail::cs_angular_units<Point2>::type,
                    geometry::degree
                >::value;
        if (both_degree)
        {
            result *= math::r2d<calc_t>();
        }
        */

        return result;
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct azimuth
{
    template <typename P1, typename P2>
    static auto apply(P1 const& p1, P2 const& p2, Strategy const& strategy)
    {
        return dispatch::azimuth<P1, P2>::apply(p1, p2, strategy);
    }
};

template <typename Strategy>
struct azimuth<Strategy, false>
{
    template <typename P1, typename P2>
    static auto apply(P1 const& p1, P2 const& p2, Strategy const& strategy)
    {
        using strategies::azimuth::services::strategy_converter;
        return dispatch::azimuth
            <
                P1, P2
            >::apply(p1, p2, strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct azimuth<default_strategy, false>
{
    template <typename P1, typename P2>
    static auto apply(P1 const& p1, P2 const& p2, default_strategy)
    {
        typedef typename strategies::azimuth::services::default_strategy
            <
                P1, P2
            >::type strategy_type;

        return dispatch::azimuth<P1, P2>::apply(p1, p2, strategy_type());
    }
};


} // namespace resolve_strategy


namespace resolve_variant
{
} // namespace resolve_variant


/*!
\brief Calculate azimuth of a segment defined by a pair of points.
\ingroup azimuth
\tparam Point1 Type of the first point of a segment.
\tparam Point2 Type of the second point of a segment.
\param point1 First point of a segment.
\param point2 Second point of a segment.
\return Azimuth in radians.

\qbk{[include reference/algorithms/azimuth.qbk]}

\qbk{
[heading Example]
[azimuth]
[azimuth_output]
}
*/
template <typename Point1, typename Point2>
inline auto azimuth(Point1 const& point1, Point2 const& point2)
{
    concepts::check<Point1 const>();
    concepts::check<Point2 const>();
    
    return resolve_strategy::azimuth
            <
                default_strategy
            >::apply(point1, point2, default_strategy());
}


/*!
\brief Calculate azimuth of a segment defined by a pair of points.
\ingroup azimuth
\tparam Point1 Type of the first point of a segment.
\tparam Point2 Type of the second point of a segment.
\tparam Strategy Type of an umbrella strategy defining azimuth strategy.
\param point1 First point of a segment.
\param point2 Second point of a segment.
\param strategy Umbrella strategy defining azimuth strategy.
\return Azimuth in radians.

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/azimuth.qbk]}

\qbk{
[heading Example]
[azimuth_strategy]
[azimuth_strategy_output]
}
*/
template <typename Point1, typename Point2, typename Strategy>
inline auto azimuth(Point1 const& point1, Point2 const& point2, Strategy const& strategy)
{
    concepts::check<Point1 const>();
    concepts::check<Point2 const>();

    return resolve_strategy::azimuth<Strategy>::apply(point1, point2, strategy);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_AZIMUTH_HPP
