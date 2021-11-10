// Boost.Geometry

// Copyright (c) 2016-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_MAXIMUM_LATITUDE_HPP
#define BOOST_GEOMETRY_FORMULAS_MAXIMUM_LATITUDE_HPP


#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/spherical.hpp>


namespace boost { namespace geometry { namespace formula
{

/*!
\brief Algorithm to compute the vertex latitude of a geodesic segment. Vertex is
a point on the geodesic that maximizes (or minimizes) the latitude.
\author See
    [Wood96] Wood - Vertex Latitudes on Ellipsoid Geodesics, SIAM Rev., 38(4),
             637–644, 1996
*/

template <typename CT>
class vertex_latitude_on_sphere
{

public:
    template<typename T1, typename T2>
    static inline CT apply(T1 const& lat1,
                           T2 const& alp1)
    {
        return std::acos( math::abs(cos(lat1) * sin(alp1)) );
    }
};

template <typename CT>
class vertex_latitude_on_spheroid
{

public:
/*
 * formula based on paper
 *   [Wood96] Wood - Vertex Latitudes on Ellipsoid Geodesics, SIAM Rev., 38(4),
 *            637–644, 1996
    template <typename T1, typename T2, typename Spheroid>
    static inline CT apply(T1 const& lat1,
                           T2 const& alp1,
                           Spheroid const& spheroid)
    {
        CT const f = formula::flattening<CT>(spheroid);

        CT const e2 = f * (CT(2) - f);
        CT const sin_alp1 = sin(alp1);
        CT const sin2_lat1 = math::sqr(sin(lat1));
        CT const cos2_lat1 = CT(1) - sin2_lat1;

        CT const e2_sin2 = CT(1) - e2 * sin2_lat1;
        CT const cos2_sin2 = cos2_lat1 * math::sqr(sin_alp1);
        CT const vertex_lat = std::asin( math::sqrt((e2_sin2 - cos2_sin2)
                                                    / (e2_sin2 - e2 * cos2_sin2)));
        return vertex_lat;
    }
*/

    // simpler formula based on Clairaut relation for spheroids
    template <typename T1, typename T2, typename Spheroid>
    static inline CT apply(T1 const& lat1,
                           T2 const& alp1,
                           Spheroid const& spheroid)
    {
        CT const f = formula::flattening<CT>(spheroid);

        CT const one_minus_f = (CT(1) - f);

        //get the reduced latitude
        CT const bet1 = atan( one_minus_f * tan(lat1) );

        //apply Clairaut relation
        CT const betv =  vertex_latitude_on_sphere<CT>::apply(bet1, alp1);

        //return the spheroid latitude
        return atan( tan(betv) / one_minus_f );
    }

    /*
    template <typename T>
    inline static void sign_adjustment(CT lat1, CT lat2, CT vertex_lat, T& vrt_result)
    {
        // signbit returns a non-zero value (true) if the sign is negative;
        // and zero (false) otherwise.
        bool sign = std::signbit(std::abs(lat1) > std::abs(lat2) ? lat1 : lat2);

        vrt_result.north = sign ? std::max(lat1, lat2) : vertex_lat;
        vrt_result.south = sign ? vertex_lat * CT(-1) : std::min(lat1, lat2);
    }

    template <typename T>
    inline static bool vertex_on_segment(CT alp1, CT alp2, CT lat1, CT lat2, T& vrt_result)
    {
        CT const half_pi = math::pi<CT>() / CT(2);

        // if the segment does not contain the vertex of the geodesic
        // then return the endpoint of max (min) latitude
        if ((alp1 < half_pi && alp2 < half_pi)
                || (alp1 > half_pi && alp2 > half_pi))
        {
            vrt_result.north = std::max(lat1, lat2);
            vrt_result.south = std::min(lat1, lat2);
            return false;
        }
        return true;
    }
    */
};


template <typename CT, typename CS_Tag>
struct vertex_latitude
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this coordinate system.",
        CT, CS_Tag);
};

template <typename CT>
struct vertex_latitude<CT, spherical_equatorial_tag>
        : vertex_latitude_on_sphere<CT>
{};

template <typename CT>
struct vertex_latitude<CT, geographic_tag>
        : vertex_latitude_on_spheroid<CT>
{};


}}} // namespace boost::geometry::formula

#endif // BOOST_GEOMETRY_FORMULAS_MAXIMUM_LATITUDE_HPP
