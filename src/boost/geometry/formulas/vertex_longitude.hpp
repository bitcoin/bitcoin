// Boost.Geometry

// Copyright (c) 2016-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_MAXIMUM_LONGITUDE_HPP
#define BOOST_GEOMETRY_FORMULAS_MAXIMUM_LONGITUDE_HPP


#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/formulas/spherical.hpp>
#include <boost/geometry/formulas/flattening.hpp>

#include <boost/math/special_functions/hypot.hpp>

namespace boost { namespace geometry { namespace formula
{

/*!
\brief Algorithm to compute the vertex longitude of a geodesic segment. Vertex is
a point on the geodesic that maximizes (or minimizes) the latitude. The algorithm
is given the vertex latitude.
*/

//Classes for spesific CS

template <typename CT>
class vertex_longitude_on_sphere
{

public:

    template <typename T>
    static inline CT apply(T const& lat1, //segment point 1
                           T const& lat2, //segment point 2
                           T const& lat3, //vertex latitude
                           T const& sin_l12,
                           T const& cos_l12) //lon1 -lon2
    {
        //https://en.wikipedia.org/wiki/Great-circle_navigation#Finding_way-points
        CT const A = sin(lat1) * cos(lat2) * cos(lat3) * sin_l12;
        CT const B = sin(lat1) * cos(lat2) * cos(lat3) * cos_l12
                - cos(lat1) * sin(lat2) * cos(lat3);
        CT lon = atan2(B, A);
        return lon + math::pi<CT>();
    }
};

template <typename CT>
class vertex_longitude_on_spheroid
{
    template<typename T>
    static inline void normalize(T& x, T& y)
    {
        T h = boost::math::hypot(x, y);
        x /= h;
        y /= h;
    }

public:

    template <typename T, typename Spheroid>
    static inline CT apply(T const& lat1, //segment point 1
                           T const& lat2, //segment point 2
                           T const& lat3, //vertex latitude
                           T& alp1,
                           Spheroid const& spheroid)
    {
        // We assume that segment points lay on different side w.r.t.
        // the vertex

        // Constants
        CT const c0 = 0;
        CT const c2 = 2;
        CT const half_pi = math::pi<CT>() / c2;
        if (math::equals(lat1, half_pi)
                || math::equals(lat2, half_pi)
                || math::equals(lat1, -half_pi)
                || math::equals(lat2, -half_pi))
        {
            // one segment point is the pole
            return c0;
        }

        // More constants
        CT const f = flattening<CT>(spheroid);
        CT const pi = math::pi<CT>();
        CT const c1 = 1;
        CT const cminus1 = -1;

        // First, compute longitude on auxiliary sphere

        CT const one_minus_f = c1 - f;
        CT const bet1 = atan(one_minus_f * tan(lat1));
        CT const bet2 = atan(one_minus_f * tan(lat2));
        CT const bet3 = atan(one_minus_f * tan(lat3));

        CT cos_bet1 = cos(bet1);
        CT cos_bet2 = cos(bet2);
        CT const sin_bet1 = sin(bet1);
        CT const sin_bet2 = sin(bet2);
        CT const sin_bet3 = sin(bet3);

        CT omg12 = 0;

        if (bet1 < c0)
        {
            cos_bet1 *= cminus1;
            omg12 += pi;
        }
        if (bet2 < c0)
        {
            cos_bet2 *= cminus1;
            omg12 += pi;
        }

        CT const sin_alp1 = sin(alp1);
        CT const cos_alp1 = math::sqrt(c1 - math::sqr(sin_alp1));

        CT const norm = math::sqrt(math::sqr(cos_alp1) + math::sqr(sin_alp1 * sin_bet1));
        CT const sin_alp0 = sin(atan2(sin_alp1 * cos_bet1, norm));

        BOOST_ASSERT(cos_bet2 != c0);
        CT const sin_alp2 = sin_alp1 * cos_bet1 / cos_bet2;

        CT const cos_alp0 = math::sqrt(c1 - math::sqr(sin_alp0));
        CT const cos_alp2 = math::sqrt(c1 - math::sqr(sin_alp2));

        CT const sig1 = atan2(sin_bet1, cos_alp1 * cos_bet1);
        CT const sig2 = atan2(sin_bet2, -cos_alp2 * cos_bet2); //lat3 is a vertex

        CT const cos_sig1 = cos(sig1);
        CT const sin_sig1 = math::sqrt(c1 - math::sqr(cos_sig1));

        CT const cos_sig2 = cos(sig2);
        CT const sin_sig2 = math::sqrt(c1 - math::sqr(cos_sig2));

        CT const omg1 = atan2(sin_alp0 * sin_sig1, cos_sig1);
        CT const omg2 = atan2(sin_alp0 * sin_sig2, cos_sig2);

        omg12 += omg1 - omg2;

        CT const sin_omg12 = sin(omg12);
        CT const cos_omg12 = cos(omg12);

        CT omg13 = geometry::formula::vertex_longitude_on_sphere<CT>
                ::apply(bet1, bet2, bet3, sin_omg12, cos_omg12);

        if (lat1 * lat2 < c0)//different hemispheres
        {
            if ((lat2 - lat1) * lat3  > c0)// ascending segment
            {
                omg13 = pi - omg13;
            }
        }

        // Second, compute the ellipsoidal longitude

        CT const e2 = f * (c2 - f);
        CT const ep = math::sqrt(e2 / (c1 - e2));
        CT const k2 = math::sqr(ep * cos_alp0);
        CT const sqrt_k2_plus_one = math::sqrt(c1 + k2);
        CT const eps = (sqrt_k2_plus_one - c1) / (sqrt_k2_plus_one + c1);
        CT const eps2 = eps * eps;
        CT const n = f / (c2 - f);

        // sig3 is the length from equator to the vertex
        CT sig3;
        if(sin_bet3 > c0)
        {
            sig3 = half_pi;
        } else {
            sig3 = -half_pi;
        }
        CT const cos_sig3 = 0;
        CT const sin_sig3 = 1;

        CT sig13 = sig3 - sig1;
        if (sig13 > pi)
        {
            sig13 -= 2 * pi;
        }

        // Order 2 approximation
        CT const c1over2 = 0.5;
        CT const c1over4 = 0.25;
        CT const c1over8 = 0.125;
        CT const c1over16 = 0.0625;
        CT const c4 = 4;
        CT const c8 = 8;

        CT const A3 = 1 - (c1over2 - c1over2 * n) * eps - c1over4 * eps2;
        CT const C31 = (c1over4 - c1over4 * n) * eps + c1over8 * eps2;
        CT const C32 = c1over16 * eps2;

        CT const sin2_sig3 = c2 * cos_sig3 * sin_sig3;
        CT const sin4_sig3 = sin_sig3 * (-c4 * cos_sig3
                                         + c8 * cos_sig3 * cos_sig3 * cos_sig3);
        CT const sin2_sig1 = c2 * cos_sig1 * sin_sig1;
        CT const sin4_sig1 = sin_sig1 * (-c4 * cos_sig1
                                         + c8 * cos_sig1 * cos_sig1 * cos_sig1);
        CT const I3 = A3 * (sig13
                            + C31 * (sin2_sig3 - sin2_sig1)
                            + C32 * (sin4_sig3 - sin4_sig1));

        CT const sign = bet3 >= c0
                      ? c1
                      : cminus1;
        
        CT const dlon_max = omg13 - sign * f * sin_alp0 * I3;

        return dlon_max;
    }
};

//CS_tag dispatching

template <typename CT, typename CS_Tag>
struct compute_vertex_lon
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this coordinate system.",
        CT, CS_Tag);
};

template <typename CT>
struct compute_vertex_lon<CT, spherical_equatorial_tag>
{
    template <typename Strategy>
    static inline CT apply(CT const& lat1,
                           CT const& lat2,
                           CT const& vertex_lat,
                           CT const& sin_l12,
                           CT const& cos_l12,
                           CT,
                           Strategy)
    {
        return vertex_longitude_on_sphere<CT>
                ::apply(lat1,
                        lat2,
                        vertex_lat,
                        sin_l12,
                        cos_l12);
    }
};

template <typename CT>
struct compute_vertex_lon<CT, geographic_tag>
{
    template <typename Strategy>
    static inline CT apply(CT const& lat1,
                           CT const& lat2,
                           CT const& vertex_lat,
                           CT,
                           CT,
                           CT& alp1,
                           Strategy const& azimuth_strategy)
    {
        return vertex_longitude_on_spheroid<CT>
                ::apply(lat1,
                        lat2,
                        vertex_lat,
                        alp1,
                        azimuth_strategy.model());
    }
};

// Vertex longitude interface
// Assume that lon1 < lon2 and vertex_lat is the latitude of the vertex

template <typename CT, typename CS_Tag>
class vertex_longitude
{
public :
    template <typename Strategy>
    static inline CT apply(CT& lon1,
                           CT& lat1,
                           CT& lon2,
                           CT& lat2,
                           CT const& vertex_lat,
                           CT& alp1,
                           Strategy const& azimuth_strategy)
    {
        CT const c0 = 0;
        CT pi = math::pi<CT>();

        //Vertex is a segment's point
        if (math::equals(vertex_lat, lat1))
        {
            return lon1;
        }
        if (math::equals(vertex_lat, lat2))
        {
            return lon2;
        }

        //Segment lay on meridian
        if (math::equals(lon1, lon2))
        {
            return (std::max)(lat1, lat2);
        }
        BOOST_ASSERT(lon1 < lon2);

        CT dlon = compute_vertex_lon<CT, CS_Tag>::apply(lat1, lat2,
                                                        vertex_lat,
                                                        sin(lon1 - lon2),
                                                        cos(lon1 - lon2),
                                                        alp1,
                                                        azimuth_strategy);

        CT vertex_lon = std::fmod(lon1 + dlon, 2 * pi);

        if (vertex_lat < c0)
        {
            vertex_lon -= pi;
        }

        if (std::abs(lon1 - lon2) > pi)
        {
            vertex_lon -= pi;
        }

        return vertex_lon;
    }
};

template <typename CT>
class vertex_longitude<CT, cartesian_tag>
{
public :
    template <typename Strategy>
    static inline CT apply(CT& /*lon1*/,
                           CT& /*lat1*/,
                           CT& lon2,
                           CT& /*lat2*/,
                           CT const& /*vertex_lat*/,
                           CT& /*alp1*/,
                           Strategy const& /*azimuth_strategy*/)
    {
        return lon2;
    }
};

}}} // namespace boost::geometry::formula
#endif // BOOST_GEOMETRY_FORMULAS_MAXIMUM_LONGITUDE_HPP

