// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_BOX_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_BOX_HPP


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/srs/sphere.hpp>
#include <boost/geometry/strategies/spherical/get_radius.hpp>
#include <boost/geometry/strategy/area.hpp>
#include <boost/geometry/util/normalize_spheroidal_box_coordinates.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

// https://math.stackexchange.com/questions/131735/surface-element-in-spherical-coordinates
// http://www.cs.cmu.edu/afs/cs/academic/class/16823-s16/www/pdfs/appearance-modeling-3.pdf
// https://www.astronomyclub.xyz/celestial-sphere-2/solid-angle-on-the-celestial-sphere.html
// https://mathworld.wolfram.com/SolidAngle.html
// https://en.wikipedia.org/wiki/Spherical_coordinate_system
// Note that the equations used in the above articles are spherical polar coordinates.
// We use spherical equatorial, so the equation is different:
// assume(y_max > y_min);
// assume(x_max > x_min);
// /* because of polar to equatorial conversion */
// sin(%pi / 2 - y);
// O: r ^ 2 * cos(y);
// S: integrate(integrate(O, y, y_min, y_max), x, x_min, x_max);
template
<
    typename RadiusTypeOrSphere = double,
    typename CalculationType = void
>
class spherical_box
{
    typedef typename strategy_detail::get_radius
        <
            RadiusTypeOrSphere
        >::type radius_type;

public:
    template <typename Box>
    struct result_type
        : strategy::area::detail::result_type
            <
                Box,
                CalculationType
            >
    {};

    // For consistency with other strategies the radius is set to 1
    inline spherical_box()
        : m_radius(1.0)
    {}

    template <typename RadiusOrSphere>
    explicit inline spherical_box(RadiusOrSphere const& radius_or_sphere)
        : m_radius(strategy_detail::get_radius
                    <
                        RadiusOrSphere
                    >::apply(radius_or_sphere))
    {}
    
    template <typename Box>
    inline auto apply(Box const& box) const
    {
        typedef typename result_type<Box>::type return_type;

        return_type x_min = get_as_radian<min_corner, 0>(box); // lon
        return_type y_min = get_as_radian<min_corner, 1>(box); // lat
        return_type x_max = get_as_radian<max_corner, 0>(box);
        return_type y_max = get_as_radian<max_corner, 1>(box);

        if (x_min == x_max || y_max == y_min)
        {
            return return_type(0);
        }

        math::normalize_spheroidal_box_coordinates<radian>(x_min, y_min, x_max, y_max);

        return (x_max - x_min)
             * (sin(y_max) - sin(y_min))
             * return_type(m_radius * m_radius);
    }

    srs::sphere<radius_type> model() const
    {
        return srs::sphere<radius_type>(m_radius);
    }

private:
    radius_type m_radius;
};


}} // namespace strategy::area


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_BOX_HPP
