// Boost.Geometry

// Copyright (c) 2018 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_MERIDIAN_DIRECT_HPP
#define BOOST_GEOMETRY_FORMULAS_MERIDIAN_DIRECT_HPP

#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/formulas/differential_quantities.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/meridian_inverse.hpp>
#include <boost/geometry/formulas/quarter_meridian.hpp>
#include <boost/geometry/formulas/result_direct.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry { namespace formula
{

/*!
\brief Compute the direct geodesic problem on a meridian
*/

template <
    typename CT,
    bool EnableCoordinates = true,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false,
    unsigned int Order = 4
>
class meridian_direct
{
    static const bool CalcQuantities = EnableReducedLength || EnableGeodesicScale;
    static const bool CalcRevAzimuth = EnableReverseAzimuth || CalcQuantities;
    static const bool CalcCoordinates = EnableCoordinates || CalcRevAzimuth;

public:
    typedef result_direct<CT> result_type;

    template <typename T, typename Dist, typename Spheroid>
    static inline result_type apply(T const& lo1,
                                    T const& la1,
                                    Dist const& distance,
                                    bool north,
                                    Spheroid const& spheroid)
    {
        result_type result;

        CT const half_pi = math::half_pi<CT>();
        CT const pi = math::pi<CT>();
        CT const one_and_a_half_pi = pi + half_pi;
        CT const c0 = 0;

        CT azimuth = north ? c0 : pi;

        if (BOOST_GEOMETRY_CONDITION(CalcCoordinates))
        {
            CT s0 = meridian_inverse<CT, Order>::apply(la1, spheroid);
            int signed_distance = north ? distance : -distance;
            result.lon2 = lo1;
            result.lat2 = apply(s0 + signed_distance, spheroid);
        }

        if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
        {
            result.reverse_azimuth = azimuth;


            if (result.lat2 > half_pi &&
                result.lat2 < one_and_a_half_pi)
            {
                result.reverse_azimuth =  pi;
            }
            else if (result.lat2 < -half_pi &&
                     result.lat2 >  -one_and_a_half_pi)
            {
                result.reverse_azimuth =  c0;
            }

        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            CT const b = CT(get_radius<2>(spheroid));
            CT const f = formula::flattening<CT>(spheroid);

            boost::geometry::math::normalize_spheroidal_coordinates
                <
                    boost::geometry::radian,
                    double
                >(result.lon2, result.lat2);

            typedef differential_quantities
            <
                CT,
                EnableReducedLength,
                EnableGeodesicScale,
                Order
            > quantities;
            quantities::apply(lo1, la1, result.lon2, result.lat2,
                              azimuth, result.reverse_azimuth,
                              b, f,
                              result.reduced_length, result.geodesic_scale);
        }
        return result;
    }

    // https://en.wikipedia.org/wiki/Meridian_arc#The_inverse_meridian_problem_for_the_ellipsoid
    // latitudes are assumed to be in radians and in [-pi/2,pi/2]
    template <typename T, typename Spheroid>
    static CT apply(T m, Spheroid const& spheroid)
    {
        CT const f = formula::flattening<CT>(spheroid);
        CT n = f / (CT(2) - f);
        CT mp = formula::quarter_meridian<CT>(spheroid);
        CT mu = geometry::math::pi<CT>()/CT(2) * m / mp;

        if (Order == 0)
        {
            return mu;
        }

        CT H2 = 1.5 * n;

        if (Order == 1)
        {
            return mu + H2 * sin(2*mu);
        }

        CT n2 = n * n;
        CT H4 = 1.3125 * n2;

        if (Order == 2)
        {
            return mu + H2 * sin(2*mu) + H4 * sin(4*mu);
        }

        CT n3 = n2 * n;
        H2 -= 0.84375 * n3;
        CT H6 = 1.572916667 * n3;

        if (Order == 3)
        {
            return mu + H2 * sin(2*mu) + H4 * sin(4*mu) + H6 * sin(6*mu);
        }

        CT n4 = n2 * n2;
        H4 -= 1.71875 * n4;
        CT H8 = 2.142578125 * n4;

        // Order 4 or higher
        return mu + H2 * sin(2*mu) + H4 * sin(4*mu) + H6 * sin(6*mu) + H8 * sin(8*mu);
    }
};

}}} // namespace boost::geometry::formula

#endif // BOOST_GEOMETRY_FORMULAS_MERIDIAN_DIRECT_HPP
