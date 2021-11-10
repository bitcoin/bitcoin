// Boost.Geometry

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2018 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014, 2016, 2017.
// Modifications copyright (c) 2014-2017 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_VINCENTY_INVERSE_HPP
#define BOOST_GEOMETRY_FORMULAS_VINCENTY_INVERSE_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/formulas/differential_quantities.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/result_inverse.hpp>


#ifndef BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS
#define BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS 1000
#endif


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The solution of the inverse problem of geodesics on latlong coordinates, after Vincenty, 1975
\author See
    - http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf
    - http://www.icsm.gov.au/gda/gda-v_2.4.pdf
\author Adapted from various implementations to get it close to the original document
    - http://www.movable-type.co.uk/scripts/LatLongVincenty.html
    - http://exogen.case.edu/projects/geopy/source/geopy.distance.html
    - http://futureboy.homeip.net/fsp/colorize.fsp?fileName=navigation.frink

*/
template <
    typename CT,
    bool EnableDistance,
    bool EnableAzimuth,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false
>
struct vincenty_inverse
{
    static const bool CalcQuantities = EnableReducedLength || EnableGeodesicScale;
    static const bool CalcAzimuths = EnableAzimuth || EnableReverseAzimuth || CalcQuantities;
    static const bool CalcFwdAzimuth = EnableAzimuth || CalcQuantities;
    static const bool CalcRevAzimuth = EnableReverseAzimuth || CalcQuantities;

public:
    typedef result_inverse<CT> result_type;

    template <typename T1, typename T2, typename Spheroid>
    static inline result_type apply(T1 const& lon1,
                                    T1 const& lat1,
                                    T2 const& lon2,
                                    T2 const& lat2,
                                    Spheroid const& spheroid)
    {
        result_type result;

        if (math::equals(lat1, lat2) && math::equals(lon1, lon2))
        {
            return result;
        }

        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;
        CT const c3 = 3;
        CT const c4 = 4;
        CT const c16 = 16;
        CT const c_e_12 = CT(1e-12);

        CT const pi = geometry::math::pi<CT>();
        CT const two_pi = c2 * pi;

        // lambda: difference in longitude on an auxiliary sphere
        CT L = lon2 - lon1;
        CT lambda = L;

        if (L < -pi) L += two_pi;
        if (L > pi) L -= two_pi;

        CT const radius_a = CT(get_radius<0>(spheroid));
        CT const radius_b = CT(get_radius<2>(spheroid));
        CT const f = formula::flattening<CT>(spheroid);

        // U: reduced latitude, defined by tan U = (1-f) tan phi
        CT const one_min_f = c1 - f;
        CT const tan_U1 = one_min_f * tan(lat1); // above (1)
        CT const tan_U2 = one_min_f * tan(lat2); // above (1)

        // calculate sin U and cos U using trigonometric identities
        CT const temp_den_U1 = math::sqrt(c1 + math::sqr(tan_U1));
        CT const temp_den_U2 = math::sqrt(c1 + math::sqr(tan_U2));
        // cos = 1 / sqrt(1 + tan^2)
        CT const cos_U1 = c1 / temp_den_U1;
        CT const cos_U2 = c1 / temp_den_U2;
        // sin = tan / sqrt(1 + tan^2)
        // sin = tan * cos
        CT const sin_U1 = tan_U1 * cos_U1;
        CT const sin_U2 = tan_U2 * cos_U2;

        // calculate sin U and cos U directly
        //CT const U1 = atan(tan_U1);
        //CT const U2 = atan(tan_U2);
        //cos_U1 = cos(U1);
        //cos_U2 = cos(U2);
        //sin_U1 = tan_U1 * cos_U1; // sin(U1);
        //sin_U2 = tan_U2 * cos_U2; // sin(U2);

        CT previous_lambda;
        CT sin_lambda;
        CT cos_lambda;
        CT sin_sigma;
        CT sin_alpha;
        CT cos2_alpha;
        CT cos_2sigma_m;
        CT cos2_2sigma_m;
        CT sigma;

        int counter = 0; // robustness

        do
        {
            previous_lambda = lambda; // (13)
            sin_lambda = sin(lambda);
            cos_lambda = cos(lambda);
            sin_sigma = math::sqrt(math::sqr(cos_U2 * sin_lambda) + math::sqr(cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lambda)); // (14)
            CT cos_sigma = sin_U1 * sin_U2 + cos_U1 * cos_U2 * cos_lambda; // (15)
            sin_alpha = cos_U1 * cos_U2 * sin_lambda / sin_sigma; // (17)
            cos2_alpha = c1 - math::sqr(sin_alpha);
            cos_2sigma_m = math::equals(cos2_alpha, c0) ? c0 : cos_sigma - c2 * sin_U1 * sin_U2 / cos2_alpha; // (18)
            cos2_2sigma_m = math::sqr(cos_2sigma_m);

            CT C = f/c16 * cos2_alpha * (c4 + f * (c4 - c3 * cos2_alpha)); // (10)
            sigma = atan2(sin_sigma, cos_sigma); // (16)
            lambda = L + (c1 - C) * f * sin_alpha *
                (sigma + C * sin_sigma * (cos_2sigma_m + C * cos_sigma * (-c1 + c2 * cos2_2sigma_m))); // (11)

            ++counter; // robustness

        } while ( geometry::math::abs(previous_lambda - lambda) > c_e_12
               && geometry::math::abs(lambda) < pi
               && counter < BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS ); // robustness
    
        if ( BOOST_GEOMETRY_CONDITION(EnableDistance) )
        {
            // Some types cannot divide by doubles
            CT const c6 = 6;
            CT const c47 = 47;
            CT const c74 = 74;
            CT const c128 = 128;
            CT const c256 = 256;
            CT const c175 = 175;
            CT const c320 = 320;
            CT const c768 = 768;
            CT const c1024 = 1024;
            CT const c4096 = 4096;
            CT const c16384 = 16384;

            //CT sqr_u = cos2_alpha * (math::sqr(radius_a) - math::sqr(radius_b)) / math::sqr(radius_b); // above (1)
            CT sqr_u = cos2_alpha * ( math::sqr(radius_a / radius_b) - c1 ); // above (1)

            CT A = c1 + sqr_u/c16384 * (c4096 + sqr_u * (-c768 + sqr_u * (c320 - c175 * sqr_u))); // (3)
            CT B = sqr_u/c1024 * (c256 + sqr_u * ( -c128 + sqr_u * (c74 - c47 * sqr_u))); // (4)
            CT const cos_sigma = cos(sigma);
            CT const sin2_sigma = math::sqr(sin_sigma);
            CT delta_sigma = B * sin_sigma * (cos_2sigma_m + (B/c4) * (cos_sigma* (-c1 + c2 * cos2_2sigma_m)
                - (B/c6) * cos_2sigma_m * (-c3 + c4 * sin2_sigma) * (-c3 + c4 * cos2_2sigma_m))); // (6)

            result.distance = radius_b * A * (sigma - delta_sigma); // (19)
        }
    
        if ( BOOST_GEOMETRY_CONDITION(CalcAzimuths) )
        {
            if (BOOST_GEOMETRY_CONDITION(CalcFwdAzimuth))
            {
                result.azimuth = atan2(cos_U2 * sin_lambda, cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lambda); // (20)
            }

            if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
            {
                result.reverse_azimuth = atan2(cos_U1 * sin_lambda, -sin_U1 * cos_U2 + cos_U1 * sin_U2 * cos_lambda); // (21)
            }
        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            typedef differential_quantities<CT, EnableReducedLength, EnableGeodesicScale, 2> quantities;
            quantities::apply(lon1, lat1, lon2, lat2,
                              result.azimuth, result.reverse_azimuth,
                              radius_b, f,
                              result.reduced_length, result.geodesic_scale);
        }

        return result;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_VINCENTY_INVERSE_HPP
