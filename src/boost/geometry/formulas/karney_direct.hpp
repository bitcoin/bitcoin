// Boost.Geometry

// Copyright (c) 2018 Adeel Ahmad, Islamabad, Pakistan.

// Contributed and/or modified by Adeel Ahmad,
//   as part of Google Summer of Code 2018 program.

// This file was modified by Oracle on 2018-2020.
// Modifications copyright (c) 2018-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from GeographicLib, https://geographiclib.sourceforge.io
// GeographicLib is originally written by Charles Karney.

// Author: Charles Karney (2008-2017)

// Last updated version of GeographicLib: 1.49

// Original copyright notice:

// Copyright (c) Charles Karney (2008-2017) <charles@karney.com> and licensed
// under the MIT/X11 License. For more information, see
// https://geographiclib.sourceforge.io

#ifndef BOOST_GEOMETRY_FORMULAS_KARNEY_DIRECT_HPP
#define BOOST_GEOMETRY_FORMULAS_KARNEY_DIRECT_HPP


#include <boost/array.hpp>

#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/result_direct.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/series_expansion.hpp>


namespace boost { namespace geometry { namespace formula
{

namespace se = series_expansion;

/*!
\brief The solution of the direct problem of geodesics on latlong coordinates,
       after Karney (2011).
\author See
- Charles F.F Karney, Algorithms for geodesics, 2011
https://arxiv.org/pdf/1109.4448.pdf
*/
template <
    typename CT,
    bool EnableCoordinates = true,
    bool EnableReverseAzimuth = false,
    bool EnableReducedLength = false,
    bool EnableGeodesicScale = false,
    size_t SeriesOrder = 8
>
class karney_direct
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

        CT lon1 = lo1;
        CT const lat1 = la1;

        Azi azi12 = azimuth12;
        math::normalize_azimuth<degree, Azi>(azi12);

        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;

        CT const b = CT(get_radius<2>(spheroid));
        CT const f = formula::flattening<CT>(spheroid);
        CT const one_minus_f = c1 - f;
        CT const two_minus_f = c2 - f;

        CT const n = f / two_minus_f;
        CT const e2 = f * two_minus_f;
        CT const ep2 = e2 / math::sqr(one_minus_f);

        CT sin_alpha1, cos_alpha1;
        math::sin_cos_degrees<CT>(azi12, sin_alpha1, cos_alpha1);

        // Find the reduced latitude.
        CT sin_beta1, cos_beta1;
        math::sin_cos_degrees<CT>(lat1, sin_beta1, cos_beta1);
        sin_beta1 *= one_minus_f;

        math::normalize_unit_vector<CT>(sin_beta1, cos_beta1);

        cos_beta1 = (std::max)(c0, cos_beta1);

        // Obtain alpha 0 by solving the spherical triangle.
        CT const sin_alpha0 = sin_alpha1 * cos_beta1;
        CT const cos_alpha0 = boost::math::hypot(cos_alpha1, sin_alpha1 * sin_beta1);

        CT const k2 = math::sqr(cos_alpha0) * ep2;

        CT const epsilon = k2 / (c2 * (c1 + math::sqrt(c1 + k2)) + k2);

        // Find the coefficients for A1 by computing the
        // series expansion using Horner scehme.
        CT const expansion_A1 = se::evaluate_A1<SeriesOrder>(epsilon);

        // Index zero element of coeffs_C1 is unused.
        se::coeffs_C1<SeriesOrder, CT> const coeffs_C1(epsilon);

        // Tau is an integration variable.
        CT const tau12 = distance / (b * (c1 + expansion_A1));

        CT const sin_tau12 = sin(tau12);
        CT const cos_tau12 = cos(tau12);

        CT sin_sigma1 = sin_beta1;
        CT sin_omega1 = sin_alpha0 * sin_beta1;

        CT cos_sigma1, cos_omega1;
        cos_sigma1 = cos_omega1 = sin_beta1 != c0 || cos_alpha1 != c0 ? cos_beta1 * cos_alpha1 : c1;
        math::normalize_unit_vector<CT>(sin_sigma1, cos_sigma1);

        CT const B11 = se::sin_cos_series(sin_sigma1, cos_sigma1, coeffs_C1);
        CT const sin_B11 = sin(B11);
        CT const cos_B11 = cos(B11);

        CT const sin_tau1 = sin_sigma1 * cos_B11 + cos_sigma1 * sin_B11;
        CT const cos_tau1 = cos_sigma1 * cos_B11 - sin_sigma1 * sin_B11;

        // Index zero element of coeffs_C1p is unused.
        se::coeffs_C1p<SeriesOrder, CT> const coeffs_C1p(epsilon);

        CT const B12 = - se::sin_cos_series
                             (sin_tau1 * cos_tau12 + cos_tau1 * sin_tau12,
                              cos_tau1 * cos_tau12 - sin_tau1 * sin_tau12,
                              coeffs_C1p);

        CT const sigma12 = tau12 - (B12 - B11);
        CT const sin_sigma12 = sin(sigma12);
        CT const cos_sigma12 = cos(sigma12);

        CT const sin_sigma2 = sin_sigma1 * cos_sigma12 + cos_sigma1 * sin_sigma12;
        CT const cos_sigma2 = cos_sigma1 * cos_sigma12 - sin_sigma1 * sin_sigma12;

        if (BOOST_GEOMETRY_CONDITION(CalcRevAzimuth))
        {
            CT const sin_alpha2 = sin_alpha0;
            CT const cos_alpha2 = cos_alpha0 * cos_sigma2;

            result.reverse_azimuth = atan2(sin_alpha2, cos_alpha2);

            // Convert the angle to radians.
            result.reverse_azimuth /= math::d2r<CT>();
        }

        if (BOOST_GEOMETRY_CONDITION(CalcCoordinates))
        {
            // Find the latitude at the second point.
            CT const sin_beta2 = cos_alpha0 * sin_sigma2;
            CT const cos_beta2 = boost::math::hypot(sin_alpha0, cos_alpha0 * cos_sigma2);

            result.lat2 = atan2(sin_beta2, one_minus_f * cos_beta2);

            // Convert the coordinate to radians.
            result.lat2 /= math::d2r<CT>();

            // Find the longitude at the second point.
            CT const sin_omega2 = sin_alpha0 * sin_sigma2;
            CT const cos_omega2 = cos_sigma2;

            CT const omega12 = atan2(sin_omega2 * cos_omega1 - cos_omega2 * sin_omega1,
                                     cos_omega2 * cos_omega1 + sin_omega2 * sin_omega1);

            se::coeffs_A3<SeriesOrder, CT> const coeffs_A3(n);

            CT const A3 = math::horner_evaluate(epsilon, coeffs_A3.begin(), coeffs_A3.end());
            CT const A3c = -f * sin_alpha0 * A3;

            se::coeffs_C3<SeriesOrder, CT> const coeffs_C3(n, epsilon);

            CT const B31 = se::sin_cos_series(sin_sigma1, cos_sigma1, coeffs_C3);

            CT const lam12 = omega12 + A3c *
                             (sigma12 + (se::sin_cos_series
                                             (sin_sigma2,
                                              cos_sigma2,
                                              coeffs_C3) - B31));

            // Convert to radians to get the
            // longitudinal difference.
            CT lon12 = lam12 / math::d2r<CT>();

            // Add the longitude at first point to the longitudinal
            // difference and normalize the result.
            math::normalize_longitude<degree, CT>(lon1);
            math::normalize_longitude<degree, CT>(lon12);

            result.lon2 = lon1 + lon12;

            // For longitudes close to the antimeridian the result can be out
            // of range. Therefore normalize.
            // In other formulas this has to be done at the end because
            // otherwise differential quantities are calculated incorrectly.
            // But here it's ok since result.lon2 is not used after this point.
            math::normalize_longitude<degree, CT>(result.lon2);
        }

        if (BOOST_GEOMETRY_CONDITION(CalcQuantities))
        {
            // Evaluate the coefficients for C2.
            // Index zero element of coeffs_C2 is unused.
            se::coeffs_C2<SeriesOrder, CT> const coeffs_C2(epsilon);

            CT const B21 = se::sin_cos_series(sin_sigma1, cos_sigma1, coeffs_C2);
            CT const B22 = se::sin_cos_series(sin_sigma2, cos_sigma2, coeffs_C2);

            // Find the coefficients for A2 by computing the
            // series expansion using Horner scehme.
            CT const expansion_A2 = se::evaluate_A2<SeriesOrder>(epsilon);

            CT const AB1 = (c1 + expansion_A1) * (B12 - B11);
            CT const AB2 = (c1 + expansion_A2) * (B22 - B21);
            CT const J12 = (expansion_A1 - expansion_A2) * sigma12 + (AB1 - AB2);

            CT const dn1 = math::sqrt(c1 + ep2 * math::sqr(sin_beta1));
            CT const dn2 = math::sqrt(c1 + k2 * math::sqr(sin_sigma2));

            // Find the reduced length.
            result.reduced_length = b * ((dn2 * (cos_sigma1 * sin_sigma2) -
                                          dn1 * (sin_sigma1 * cos_sigma2)) -
                                          cos_sigma1 * cos_sigma2 * J12);

            // Find the geodesic scale.
            CT const t = k2 * (sin_sigma2 - sin_sigma1) *
                              (sin_sigma2 + sin_sigma1) / (dn1 + dn2);

            result.geodesic_scale = cos_sigma12 +
                                    (t * sin_sigma2 - cos_sigma2 * J12) *
                                    sin_sigma1 / dn1;
        }

        return result;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_KARNEY_DIRECT_HPP
