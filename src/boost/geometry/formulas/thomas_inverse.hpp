// Boost.Geometry

// Copyright (c) 2015-2018 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_THOMAS_INVERSE_HPP
#define BOOST_GEOMETRY_FORMULAS_THOMAS_INVERSE_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/formulas/differential_quantities.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/result_inverse.hpp>


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The solution of the inverse problem of geodesics on latlong coordinates,
       Forsyth-Andoyer-Lambert type approximation with second order terms.
\author See
    - Technical Report: PAUL D. THOMAS, MATHEMATICAL MODELS FOR NAVIGATION SYSTEMS, 1965
      http://www.dtic.mil/docs/citations/AD0627893
    - Technical Report: PAUL D. THOMAS, SPHEROIDAL GEODESICS, REFERENCE SYSTEMS, AND LOCAL GEOMETRY, 1970
      http://www.dtic.mil/docs/citations/AD0703541
*/
template <
    typename CT,
    bool EnableDistance,
    bool EnableAzimuth,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false
>
class thomas_inverse
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

        // coordinates in radians

        if ( math::equals(lon1, lon2) && math::equals(lat1, lat2) )
        {
            return result;
        }

        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;
        CT const c4 = 4;

        CT const pi_half = math::pi<CT>() / c2;
        CT const f = formula::flattening<CT>(spheroid);
        CT const one_minus_f = c1 - f;

//        CT const tan_theta1 = one_minus_f * tan(lat1);
//        CT const tan_theta2 = one_minus_f * tan(lat2);
//        CT const theta1 = atan(tan_theta1);
//        CT const theta2 = atan(tan_theta2);

        CT const theta1 = math::equals(lat1, pi_half) ? lat1 :
                          math::equals(lat1, -pi_half) ? lat1 :
                          atan(one_minus_f * tan(lat1));
        CT const theta2 = math::equals(lat2, pi_half) ? lat2 :
                          math::equals(lat2, -pi_half) ? lat2 :
                          atan(one_minus_f * tan(lat2));

        CT const theta_m = (theta1 + theta2) / c2;
        CT const d_theta_m = (theta2 - theta1) / c2;
        CT const d_lambda = lon2 - lon1;
        CT const d_lambda_m = d_lambda / c2;

        CT const sin_theta_m = sin(theta_m);
        CT const cos_theta_m = cos(theta_m);
        CT const sin_d_theta_m = sin(d_theta_m);
        CT const cos_d_theta_m = cos(d_theta_m);
        CT const sin2_theta_m = math::sqr(sin_theta_m);
        CT const cos2_theta_m = math::sqr(cos_theta_m);
        CT const sin2_d_theta_m = math::sqr(sin_d_theta_m);
        CT const cos2_d_theta_m = math::sqr(cos_d_theta_m);
        CT const sin_d_lambda_m = sin(d_lambda_m);
        CT const sin2_d_lambda_m = math::sqr(sin_d_lambda_m);

        CT const H = cos2_theta_m - sin2_d_theta_m;
        CT const L = sin2_d_theta_m + H * sin2_d_lambda_m;
        CT const cos_d = c1 - c2 * L;
        CT const d = acos(cos_d);
        CT const sin_d = sin(d);

        CT const one_minus_L = c1 - L;

        if ( math::equals(sin_d, c0)
          || math::equals(L, c0)
          || math::equals(one_minus_L, c0) )
        {
            return result;
        }

        CT const U = c2 * sin2_theta_m * cos2_d_theta_m / one_minus_L;
        CT const V = c2 * sin2_d_theta_m * cos2_theta_m / L;
        CT const X = U + V;
        CT const Y = U - V;
        CT const T = d / sin_d;
        CT const D = c4 * math::sqr(T);
        CT const E = c2 * cos_d;
        CT const A = D * E;
        CT const B = c2 * D;
        CT const C = T - (A - E) / c2;

        CT const f_sqr = math::sqr(f);
        CT const f_sqr_per_64 = f_sqr / CT(64);
    
        if ( BOOST_GEOMETRY_CONDITION(EnableDistance) )
        {
            CT const n1 = X * (A + C*X);
            CT const n2 = Y * (B + E*Y);
            CT const n3 = D*X*Y;

            CT const delta1d = f * (T*X-Y) / c4;
            CT const delta2d = f_sqr_per_64 * (n1 - n2 + n3);

            CT const a = get_radius<0>(spheroid);

            //result.distance = a * sin_d * (T - delta1d);
            result.distance = a * sin_d * (T - delta1d + delta2d);
        }
    
        if ( BOOST_GEOMETRY_CONDITION(CalcAzimuths) )
        {
            // NOTE: if both cos_latX == 0 then below we'd have 0 * INF
            // it's a situation when the endpoints are on the poles +-90 deg
            // in this case the azimuth could either be 0 or +-pi
            // but above always 0 is returned

            CT const F = c2*Y-E*(c4-X);
            CT const M = CT(32)*T-(CT(20)*T-A)*X-(B+c4)*Y;
            CT const G = f*T/c2 + f_sqr_per_64 * M;
            
            // TODO:
            // If d_lambda is close to 90 or -90 deg then tan(d_lambda) is big
            // and F is small. The result is not accurate.
            // In the edge case the result may be 2 orders of magnitude less
            // accurate than Andoyer's.
            CT const tan_d_lambda = tan(d_lambda);
            CT const Q = -(F*G*tan_d_lambda) / c4;
            CT const d_lambda_m_p = (d_lambda + Q) / c2;
            CT const tan_d_lambda_m_p = tan(d_lambda_m_p);

            CT const v = atan2(cos_d_theta_m, sin_theta_m * tan_d_lambda_m_p);
            CT const u = atan2(-sin_d_theta_m, cos_theta_m * tan_d_lambda_m_p);

            CT const pi = math::pi<CT>();

            if (BOOST_GEOMETRY_CONDITION(CalcFwdAzimuth))
            {
                CT alpha1 = v + u;
                if (alpha1 > pi)
                {
                    alpha1 -= c2 * pi;
                }

                result.azimuth = alpha1;
            }

            if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
            {
                CT alpha2 = pi - (v - u);
                if (alpha2 > pi)
                {
                    alpha2 -= c2 * pi;
                }

                result.reverse_azimuth = alpha2;
            }
        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            typedef differential_quantities<CT, EnableReducedLength, EnableGeodesicScale, 2> quantities;
            quantities::apply(lon1, lat1, lon2, lat2,
                              result.azimuth, result.reverse_azimuth,
                              get_radius<2>(spheroid), f,
                              result.reduced_length, result.geodesic_scale);
        }

        return result;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_THOMAS_INVERSE_HPP
