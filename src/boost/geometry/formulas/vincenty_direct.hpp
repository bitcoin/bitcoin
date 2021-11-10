// Boost.Geometry

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_VINCENTY_DIRECT_HPP
#define BOOST_GEOMETRY_FORMULAS_VINCENTY_DIRECT_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

#include <boost/geometry/formulas/differential_quantities.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/result_direct.hpp>


#ifndef BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS
#define BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS 1000
#endif


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The solution of the direct problem of geodesics on latlong coordinates, after Vincenty, 1975
\author See
    - http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf
    - http://www.icsm.gov.au/gda/gdav2.3.pdf
\author Adapted from various implementations to get it close to the original document
    - http://www.movable-type.co.uk/scripts/LatLongVincenty.html
    - http://exogen.case.edu/projects/geopy/source/geopy.distance.html
    - http://futureboy.homeip.net/fsp/colorize.fsp?fileName=navigation.frink

*/
template <
    typename CT,
    bool EnableCoordinates = true,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false
>
class vincenty_direct
{
    static const bool CalcQuantities = EnableReducedLength || EnableGeodesicScale;
    static const bool CalcCoordinates = EnableCoordinates || CalcQuantities;
    static const bool CalcRevAzimuth = EnableReverseAzimuth || CalcQuantities;

public:
    typedef result_direct<CT> result_type;

    template <typename T, typename Dist, typename Azi, typename Spheroid>
    static inline result_type apply(T const& lo1,
                                    T const& la1,
                                    Dist const& distance,
                                    Azi const& azimuth12,
                                    Spheroid const& spheroid)
    {
        result_type result;

        CT const lon1 = lo1;
        CT const lat1 = la1;

        CT const radius_a = CT(get_radius<0>(spheroid));
        CT const radius_b = CT(get_radius<2>(spheroid));
        CT const flattening = formula::flattening<CT>(spheroid);

        CT const sin_azimuth12 = sin(azimuth12);
        CT const cos_azimuth12 = cos(azimuth12);

        // U: reduced latitude, defined by tan U = (1-f) tan phi
        CT const one_min_f = CT(1) - flattening;
        CT const tan_U1 = one_min_f * tan(lat1);
        CT const sigma1 = atan2(tan_U1, cos_azimuth12); // (1)

        // may be calculated from tan using 1 sqrt()
        CT const U1 = atan(tan_U1);
        CT const sin_U1 = sin(U1);
        CT const cos_U1 = cos(U1);

        CT const sin_alpha = cos_U1 * sin_azimuth12; // (2)
        CT const sin_alpha_sqr = math::sqr(sin_alpha);
        CT const cos_alpha_sqr = CT(1) - sin_alpha_sqr;

        CT const b_sqr = radius_b * radius_b;
        CT const u_sqr = cos_alpha_sqr * (radius_a * radius_a - b_sqr) / b_sqr;
        CT const A = CT(1) + (u_sqr/CT(16384)) * (CT(4096) + u_sqr*(CT(-768) + u_sqr*(CT(320) - u_sqr*CT(175)))); // (3)
        CT const B = (u_sqr/CT(1024))*(CT(256) + u_sqr*(CT(-128) + u_sqr*(CT(74) - u_sqr*CT(47)))); // (4)

        CT s_div_bA = distance / (radius_b * A);
        CT sigma = s_div_bA; // (7)

        CT previous_sigma;
        CT sin_sigma;
        CT cos_sigma;
        CT cos_2sigma_m;
        CT cos_2sigma_m_sqr;

        int counter = 0; // robustness

        do
        {
            previous_sigma = sigma;

            CT const two_sigma_m = CT(2) * sigma1 + sigma; // (5)

            sin_sigma = sin(sigma);
            cos_sigma = cos(sigma);
            CT const sin_sigma_sqr = math::sqr(sin_sigma);
            cos_2sigma_m = cos(two_sigma_m);
            cos_2sigma_m_sqr = math::sqr(cos_2sigma_m);

            CT const delta_sigma = B * sin_sigma * (cos_2sigma_m
                                        + (B/CT(4)) * ( cos_sigma * (CT(-1) + CT(2)*cos_2sigma_m_sqr)
                                            - (B/CT(6) * cos_2sigma_m * (CT(-3)+CT(4)*sin_sigma_sqr) * (CT(-3)+CT(4)*cos_2sigma_m_sqr)) )); // (6)

            sigma = s_div_bA + delta_sigma; // (7)

            ++counter; // robustness

        } while ( geometry::math::abs(previous_sigma - sigma) > CT(1e-12)
               //&& geometry::math::abs(sigma) < pi
               && counter < BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS ); // robustness

        if (BOOST_GEOMETRY_CONDITION(CalcCoordinates))
        {
            result.lat2
                = atan2( sin_U1 * cos_sigma + cos_U1 * sin_sigma * cos_azimuth12,
                         one_min_f * math::sqrt(sin_alpha_sqr + math::sqr(sin_U1 * sin_sigma - cos_U1 * cos_sigma * cos_azimuth12))); // (8)
            
            CT const lambda = atan2( sin_sigma * sin_azimuth12,
                                     cos_U1 * cos_sigma - sin_U1 * sin_sigma * cos_azimuth12); // (9)
            CT const C = (flattening/CT(16)) * cos_alpha_sqr * ( CT(4) + flattening * ( CT(4) - CT(3) * cos_alpha_sqr ) ); // (10)
            CT const L = lambda - (CT(1) - C) * flattening * sin_alpha
                            * ( sigma + C * sin_sigma * ( cos_2sigma_m + C * cos_sigma * ( CT(-1) + CT(2) * cos_2sigma_m_sqr ) ) ); // (11)

            result.lon2 = lon1 + L;
        }

        if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
        {
            result.reverse_azimuth
                = atan2(sin_alpha, -sin_U1 * sin_sigma + cos_U1 * cos_sigma * cos_azimuth12); // (12)
        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            typedef differential_quantities<CT, EnableReducedLength, EnableGeodesicScale, 2> quantities;
            quantities::apply(lon1, lat1, result.lon2, result.lat2,
                              azimuth12, result.reverse_azimuth,
                              radius_b, flattening,
                              result.reduced_length, result.geodesic_scale);
        }

        if (BOOST_GEOMETRY_CONDITION(CalcCoordinates))
        {
            // For longitudes close to the antimeridian the result can be out
            // of range. Therefore normalize.
            // It has to be done at the end because otherwise differential
            // quantities are calculated incorrectly.
            math::detail::normalize_angle_cond<radian>(result.lon2);
        }

        return result;
    }

};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_VINCENTY_DIRECT_HPP
