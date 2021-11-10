// Boost.Geometry

// Copyright (c) 2017-2018 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_MERIDIAN_INVERSE_HPP
#define BOOST_GEOMETRY_FORMULAS_MERIDIAN_INVERSE_HPP

#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/meridian_segment.hpp>

namespace boost { namespace geometry { namespace formula
{

/*!
\brief Compute the arc length of an ellipse.
*/

template <typename CT, unsigned int Order = 1>
class meridian_inverse
{

public :

    struct result
    {
        result()
            : distance(0)
            , meridian(false)
        {}

        CT distance;
        bool meridian;
    };

    template <typename T>
    static bool meridian_not_crossing_pole(T lat1, T lat2, CT diff)
    {
        CT half_pi = math::pi<CT>()/CT(2);
        return math::equals(diff, CT(0)) ||
                    (math::equals(lat2, half_pi) && math::equals(lat1, -half_pi));
    }

    static bool meridian_crossing_pole(CT diff)
    {
        return math::equals(math::abs(diff), math::pi<CT>());
    }


    template <typename T, typename Spheroid>
    static CT meridian_not_crossing_pole_dist(T lat1, T lat2, Spheroid const& spheroid)
    {
        return math::abs(apply(lat2, spheroid) - apply(lat1, spheroid));
    }

    template <typename T, typename Spheroid>
    static CT meridian_crossing_pole_dist(T lat1, T lat2, Spheroid const& spheroid)
    {
        CT c0 = 0;
        CT half_pi = math::pi<CT>()/CT(2);
        CT lat_sign = 1;
        if (lat1+lat2 < c0)
        {
            lat_sign = CT(-1);
        }
        return math::abs(lat_sign * CT(2) * apply(half_pi, spheroid)
                         - apply(lat1, spheroid) - apply(lat2, spheroid));
    }

    template <typename T, typename Spheroid>
    static result apply(T lon1, T lat1, T lon2, T lat2, Spheroid const& spheroid)
    {
        result res;

        CT diff = geometry::math::longitude_distance_signed<geometry::radian>(lon1, lon2);

        if (lat1 > lat2)
        {
            std::swap(lat1, lat2);
        }

        if ( meridian_not_crossing_pole(lat1, lat2, diff) )
        {
            res.distance = meridian_not_crossing_pole_dist(lat1, lat2, spheroid);
            res.meridian = true;
        }
        else if ( meridian_crossing_pole(diff) )
        {
            res.distance = meridian_crossing_pole_dist(lat1, lat2, spheroid);
            res.meridian = true;
        }
        return res;
    }

    // Distance computation on meridians using series approximations
    // to elliptic integrals. Formula to compute distance from lattitude 0 to lat
    // https://en.wikipedia.org/wiki/Meridian_arc
    // latitudes are assumed to be in radians and in [-pi/2,pi/2]
    template <typename T, typename Spheroid>
    static CT apply(T lat, Spheroid const& spheroid)
    {
        CT const a = get_radius<0>(spheroid);
        CT const f = formula::flattening<CT>(spheroid);
        CT n = f / (CT(2) - f);
        CT M = a/(1+n);
        CT C0 = 1;

        if (Order == 0)
        {
           return M * C0 * lat;
        }

        CT C2 = -1.5 * n;

        if (Order == 1)
        {
            return M * (C0 * lat + C2 * sin(2*lat));
        }

        CT n2 = n * n;
        C0 += .25 * n2;
        CT C4 = 0.9375 * n2;

        if (Order == 2)
        {
            return M * (C0 * lat + C2 * sin(2*lat) + C4 * sin(4*lat));
        }

        CT n3 = n2 * n;
        C2 += 0.1875 * n3;
        CT C6 = -0.729166667 * n3;

        if (Order == 3)
        {
            return M * (C0 * lat + C2 * sin(2*lat) + C4 * sin(4*lat)
                      + C6 * sin(6*lat));
        }

        CT n4 = n2 * n2;
        C4 -= 0.234375 * n4;
        CT C8 = 0.615234375 * n4;

        if (Order == 4)
        {
            return M * (C0 * lat + C2 * sin(2*lat) + C4 * sin(4*lat)
                      + C6 * sin(6*lat) + C8 * sin(8*lat));
        }

        CT n5 = n4 * n;
        C6 += 0.227864583 * n5;
        CT C10 = -0.54140625 * n5;

        // Order 5 or higher
        return M * (C0 * lat + C2 * sin(2*lat) + C4 * sin(4*lat)
                  + C6 * sin(6*lat) + C8 * sin(8*lat) + C10 * sin(10*lat));

    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_MERIDIAN_INVERSE_HPP
