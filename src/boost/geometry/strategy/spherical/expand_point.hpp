// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_POINT_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_POINT_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/is_inverse_spheroidal_coordinates.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/algorithms/detail/normalize.hpp>
#include <boost/geometry/algorithms/detail/envelope/transform_units.hpp>

#include <boost/geometry/strategy/expand.hpp>
#include <boost/geometry/strategy/cartesian/expand_point.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace expand
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// implementation for the spherical and geographic coordinate systems
template <std::size_t DimensionCount, bool IsEquatorial>
struct point_loop_on_spheroid
{
    template <typename Box, typename Point>
    static inline void apply(Box& box, Point const& point)
    {
        typedef typename point_type<Box>::type box_point_type;
        typedef typename coordinate_type<Box>::type box_coordinate_type;
        typedef typename geometry::detail::cs_angular_units<Box>::type units_type;

        typedef math::detail::constants_on_spheroid
            <
                box_coordinate_type,
                units_type
            > constants;

        // normalize input point and input box
        Point p_normalized;
        strategy::normalize::spherical_point::apply(point, p_normalized);

        // transform input point to be of the same type as the box point
        box_point_type box_point;
        geometry::detail::envelope::transform_units(p_normalized, box_point);

        if (is_inverse_spheroidal_coordinates(box))
        {
            geometry::set_from_radian<min_corner, 0>(box, geometry::get_as_radian<0>(p_normalized));
            geometry::set_from_radian<min_corner, 1>(box, geometry::get_as_radian<1>(p_normalized));
            geometry::set_from_radian<max_corner, 0>(box, geometry::get_as_radian<0>(p_normalized));
            geometry::set_from_radian<max_corner, 1>(box, geometry::get_as_radian<1>(p_normalized));

        } else {

            strategy::normalize::spherical_box::apply(box, box);

            box_coordinate_type p_lon = geometry::get<0>(box_point);
            box_coordinate_type p_lat = geometry::get<1>(box_point);

            typename coordinate_type<Box>::type
                    b_lon_min = geometry::get<min_corner, 0>(box),
                    b_lat_min = geometry::get<min_corner, 1>(box),
                    b_lon_max = geometry::get<max_corner, 0>(box),
                    b_lat_max = geometry::get<max_corner, 1>(box);

            if (math::is_latitude_pole<units_type, IsEquatorial>(p_lat))
            {
                // the point of expansion is the either the north or the
                // south pole; the only important coordinate here is the
                // pole's latitude, as the longitude can be anything;
                // we, thus, take into account the point's latitude only and return
                geometry::set<min_corner, 1>(box, (std::min)(p_lat, b_lat_min));
                geometry::set<max_corner, 1>(box, (std::max)(p_lat, b_lat_max));
                return;
            }

            if (math::equals(b_lat_min, b_lat_max)
                    && math::is_latitude_pole<units_type, IsEquatorial>(b_lat_min))
            {
                // the box degenerates to either the north or the south pole;
                // the only important coordinate here is the pole's latitude,
                // as the longitude can be anything;
                // we thus take into account the box's latitude only and return
                geometry::set<min_corner, 0>(box, p_lon);
                geometry::set<min_corner, 1>(box, (std::min)(p_lat, b_lat_min));
                geometry::set<max_corner, 0>(box, p_lon);
                geometry::set<max_corner, 1>(box, (std::max)(p_lat, b_lat_max));
                return;
            }

            // update latitudes
            b_lat_min = (std::min)(b_lat_min, p_lat);
            b_lat_max = (std::max)(b_lat_max, p_lat);

            // update longitudes
            if (math::smaller(p_lon, b_lon_min))
            {
                box_coordinate_type p_lon_shifted = p_lon + constants::period();

                if (math::larger(p_lon_shifted, b_lon_max))
                {
                    // here we could check using: ! math::larger(.., ..)
                    if (math::smaller(b_lon_min - p_lon, p_lon_shifted - b_lon_max))
                    {
                        b_lon_min = p_lon;
                    }
                    else
                    {
                        b_lon_max = p_lon_shifted;
                    }
                }
            }
            else if (math::larger(p_lon, b_lon_max))
            {
                // in this case, and since p_lon is normalized in the range
                // (-180, 180], we must have that b_lon_max <= 180
                if (b_lon_min < 0
                        && math::larger(p_lon - b_lon_max,
                                        constants::period() - p_lon + b_lon_min))
                {
                    b_lon_min = p_lon;
                    b_lon_max += constants::period();
                }
                else
                {
                    b_lon_max = p_lon;
                }
            }

            geometry::set<min_corner, 0>(box, b_lon_min);
            geometry::set<min_corner, 1>(box, b_lat_min);
            geometry::set<max_corner, 0>(box, b_lon_max);
            geometry::set<max_corner, 1>(box, b_lat_max);
        }

        point_loop
            <
                2, DimensionCount
            >::apply(box, point);
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


struct spherical_point
{
    template <typename Box, typename Point>
    static void apply(Box & box, Point const& point)
    {
        expand::detail::point_loop_on_spheroid
            <
                dimension<Point>::value,
                ! std::is_same<typename cs_tag<Point>::type, spherical_polar_tag>::value
            >::apply(box, point);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<point_tag, spherical_equatorial_tag, CalculationType>
{
    typedef spherical_point type;
};

template <typename CalculationType>
struct default_strategy<point_tag, spherical_polar_tag, CalculationType>
{
    typedef spherical_point type;
};

template <typename CalculationType>
struct default_strategy<point_tag, geographic_tag, CalculationType>
{
    typedef spherical_point type;
};


} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::expand

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_POINT_HPP
