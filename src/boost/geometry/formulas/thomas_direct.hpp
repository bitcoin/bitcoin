// Boost.Geometry

// Copyright (c) 2016-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_THOMAS_DIRECT_HPP
#define BOOST_GEOMETRY_FORMULAS_THOMAS_DIRECT_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

#include <boost/geometry/formulas/differential_quantities.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/result_direct.hpp>


namespace boost { namespace geometry { namespace formula
{


/*!
\brief The solution of the direct problem of geodesics on latlong coordinates,
       Forsyth-Andoyer-Lambert type approximation with first/second order terms.
\author See
    - Technical Report: PAUL D. THOMAS, MATHEMATICAL MODELS FOR NAVIGATION SYSTEMS, 1965
      http://www.dtic.mil/docs/citations/AD0627893
    - Technical Report: PAUL D. THOMAS, SPHEROIDAL GEODESICS, REFERENCE SYSTEMS, AND LOCAL GEOMETRY, 1970
      http://www.dtic.mil/docs/citations/AD0703541
*/
template <
    typename CT,
    bool SecondOrder = true,
    bool EnableCoordinates = true,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false
>
class thomas_direct
{
    static const bool CalcQuantities = EnableReducedLength || EnableGeodesicScale;
    static const bool CalcCoordinates = EnableCoordinates || CalcQuantities;
    static const bool CalcRevAzimuth = EnableReverseAzimuth || CalcCoordinates || CalcQuantities;

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

        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;
        CT const c4 = 4;

        CT const a = CT(get_radius<0>(spheroid));
        CT const b = CT(get_radius<2>(spheroid));
        CT const f = formula::flattening<CT>(spheroid);
        CT const one_minus_f = c1 - f;

        CT const pi = math::pi<CT>();
        CT const pi_half = pi / c2;

        BOOST_GEOMETRY_ASSERT(-pi <= azimuth12 && azimuth12 <= pi);

        // keep azimuth small - experiments show low accuracy
        // if the azimuth is closer to (+-)180 deg.
        CT azi12_alt = azimuth12;
        CT lat1_alt = lat1;
        bool alter_result = vflip_if_south(lat1, azimuth12, lat1_alt, azi12_alt);

        CT const theta1 = math::equals(lat1_alt, pi_half) ? lat1_alt :
                          math::equals(lat1_alt, -pi_half) ? lat1_alt :
                          atan(one_minus_f * tan(lat1_alt));
        CT const sin_theta1 = sin(theta1);
        CT const cos_theta1 = cos(theta1);

        CT const sin_a12 = sin(azi12_alt);
        CT const cos_a12 = cos(azi12_alt);

        CT const M = cos_theta1 * sin_a12; // cos_theta0
        CT const theta0 = acos(M);
        CT const sin_theta0 = sin(theta0);

        CT const N = cos_theta1 * cos_a12;
        CT const C1 = f * M; // lower-case c1 in the technical report
        CT const C2 = f * (c1 - math::sqr(M)) / c4; // lower-case c2 in the technical report
        CT D = 0;
        CT P = 0;
        if ( BOOST_GEOMETRY_CONDITION(SecondOrder) )
        {
            D = (c1 - C2) * (c1 - C2 - C1 * M);
            P = C2 * (c1 + C1 * M / c2) / D;
        }
        else
        {
            D = c1 - c2 * C2 - C1 * M;
            P = C2 / D;
        }
        // special case for equator:
        // sin_theta0 = 0 <=> lat1 = 0 ^ |azimuth12| = pi/2
        // NOTE: in this case it doesn't matter what's the value of cos_sigma1 because
        //       theta1=0, theta0=0, M=1|-1, C2=0 so X=0 and Y=0 so d_sigma=d
        //       cos_a12=0 so N=0, therefore
        //       lat2=0, azi21=pi/2|-pi/2
        //       d_eta = atan2(sin_d_sigma, cos_d_sigma)
        //       H = C1 * d_sigma
        CT const cos_sigma1 = math::equals(sin_theta0, c0)
                                ? c1
                                : normalized1_1(sin_theta1 / sin_theta0);
        CT const sigma1 = acos(cos_sigma1);
        CT const d = distance / (a * D);
        CT const u = 2 * (sigma1 - d);
        CT const cos_d = cos(d);
        CT const sin_d = sin(d);
        CT const cos_u = cos(u);
        CT const sin_u = sin(u);

        CT const W = c1 - c2 * P * cos_u;
        CT const V = cos_u * cos_d - sin_u * sin_d;
        CT const Y = c2 * P * V * W * sin_d;
        CT X = 0;
        CT d_sigma = d - Y;
        if ( BOOST_GEOMETRY_CONDITION(SecondOrder) )
        {
            X = math::sqr(C2) * sin_d * cos_d * (2 * math::sqr(V) - c1);
            d_sigma += X;
        }
        CT const sin_d_sigma = sin(d_sigma);
        CT const cos_d_sigma = cos(d_sigma);

        if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
        {
            result.reverse_azimuth = atan2(M, N * cos_d_sigma - sin_theta1 * sin_d_sigma);

            if (alter_result)
            {
                vflip_rev_azi(result.reverse_azimuth, azimuth12);
            }
        }

        if (BOOST_GEOMETRY_CONDITION(CalcCoordinates))
        {
            CT const S_sigma = c2 * sigma1 - d_sigma;
            CT cos_S_sigma = 0;
            CT H = C1 * d_sigma;
            if ( BOOST_GEOMETRY_CONDITION(SecondOrder) )
            {
                cos_S_sigma = cos(S_sigma);
                H = H * (c1 - C2) - C1 * C2 * sin_d_sigma * cos_S_sigma;
            }
            CT const d_eta = atan2(sin_d_sigma * sin_a12, cos_theta1 * cos_d_sigma - sin_theta1 * sin_d_sigma * cos_a12);
            CT const d_lambda = d_eta - H;

            result.lon2 = lon1 + d_lambda;

            if (! math::equals(M, c0))
            {
                CT const sin_a21 = sin(result.reverse_azimuth);
                CT const tan_theta2 = (sin_theta1 * cos_d_sigma + N * sin_d_sigma) * sin_a21 / M;
                result.lat2 = atan(tan_theta2 / one_minus_f);
            }
            else
            {
                CT const sigma2 = S_sigma - sigma1;
                //theta2 = asin(cos(sigma2)) <=> sin_theta0 = 1
                // NOTE: cos(sigma2) defines the sign of tan_theta2
                CT const tan_theta2 = cos(sigma2) / math::abs(sin(sigma2));
                result.lat2 = atan(tan_theta2 / one_minus_f);
            }

            if (alter_result)
            {
                result.lat2 = -result.lat2;
            }
        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            typedef differential_quantities<CT, EnableReducedLength, EnableGeodesicScale, 2> quantities;
            quantities::apply(lon1, lat1, result.lon2, result.lat2,
                              azimuth12, result.reverse_azimuth,
                              b, f,
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

private:
    static inline bool vflip_if_south(CT const& lat1, CT const& azi12, CT & lat1_alt, CT & azi12_alt)
    {
        CT const c2 = 2;
        CT const pi = math::pi<CT>();
        CT const pi_half = pi / c2;

        if (azi12 > pi_half)
        {
            azi12_alt = pi - azi12;
            lat1_alt = -lat1;
            return true;
        }
        else if (azi12 < -pi_half)
        {
            azi12_alt = -pi - azi12;
            lat1_alt = -lat1;
            return true;
        }

        return false;
    }

    static inline void vflip_rev_azi(CT & rev_azi, CT const& azimuth12)
    {
        CT const c0 = 0;
        CT const pi = math::pi<CT>();

        if (rev_azi == c0)
        {
            rev_azi = azimuth12 >= 0 ? pi : -pi;
        }
        else if (rev_azi > c0)
        {
            rev_azi = pi - rev_azi;
        }
        else
        {
            rev_azi = -pi - rev_azi;
        }
    }

    static inline CT normalized1_1(CT const& value)
    {
        CT const c1 = 1;
        return value > c1 ? c1 :
               value < -c1 ? -c1 :
               value;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_THOMAS_DIRECT_HPP
