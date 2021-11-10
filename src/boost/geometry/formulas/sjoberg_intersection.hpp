// Boost.Geometry

// Copyright (c) 2016-2019 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_SJOBERG_INTERSECTION_HPP
#define BOOST_GEOMETRY_FORMULAS_SJOBERG_INTERSECTION_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/spherical.hpp>


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The intersection of two great circles as proposed by Sjoberg.
\see See
    - [Sjoberg02] Lars E. Sjoberg, Intersections on the sphere and ellipsoid, 2002
      http://link.springer.com/article/10.1007/s00190-001-0230-9
*/
template <typename CT>
struct sjoberg_intersection_spherical_02
{
    // TODO: if it will be used as standalone formula
    //       support segments on equator and endpoints on poles

    static inline bool apply(CT const& lon1, CT const& lat1, CT const& lon_a2, CT const& lat_a2,
                             CT const& lon2, CT const& lat2, CT const& lon_b2, CT const& lat_b2,
                             CT & lon, CT & lat)
    {
        CT tan_lat = 0;
        bool res = apply_alt(lon1, lat1, lon_a2, lat_a2,
                             lon2, lat2, lon_b2, lat_b2,
                             lon, tan_lat);

        if (res)
        {
            lat = atan(tan_lat);
        }

        return res;
    }

    static inline bool apply_alt(CT const& lon1, CT const& lat1, CT const& lon_a2, CT const& lat_a2,
                                 CT const& lon2, CT const& lat2, CT const& lon_b2, CT const& lat_b2,
                                 CT & lon, CT & tan_lat)
    {
        CT const cos_lon1 = cos(lon1);
        CT const sin_lon1 = sin(lon1);
        CT const cos_lon2 = cos(lon2);
        CT const sin_lon2 = sin(lon2);
        CT const sin_lat1 = sin(lat1);
        CT const sin_lat2 = sin(lat2);
        CT const cos_lat1 = cos(lat1);
        CT const cos_lat2 = cos(lat2);

        CT const tan_lat_a2 = tan(lat_a2);
        CT const tan_lat_b2 = tan(lat_b2);
        
        return apply(lon1, lon_a2, lon2, lon_b2,
                     sin_lon1, cos_lon1, sin_lat1, cos_lat1,
                     sin_lon2, cos_lon2, sin_lat2, cos_lat2,
                     tan_lat_a2, tan_lat_b2,
                     lon, tan_lat);
    }

private:
    static inline bool apply(CT const& lon1, CT const& lon_a2, CT const& lon2, CT const& lon_b2,
                             CT const& sin_lon1, CT const& cos_lon1, CT const& sin_lat1, CT const& cos_lat1,
                             CT const& sin_lon2, CT const& cos_lon2, CT const& sin_lat2, CT const& cos_lat2,
                             CT const& tan_lat_a2, CT const& tan_lat_b2,
                             CT & lon, CT & tan_lat)
    {
        // NOTE:
        // cos_lat_ = 0 <=> segment on equator
        // tan_alpha_ = 0 <=> segment vertical

        CT const tan_lat1 = sin_lat1 / cos_lat1; //tan(lat1);
        CT const tan_lat2 = sin_lat2 / cos_lat2; //tan(lat2);

        CT const dlon1 = lon_a2 - lon1;
        CT const sin_dlon1 = sin(dlon1);
        CT const dlon2 = lon_b2 - lon2;
        CT const sin_dlon2 = sin(dlon2);

        CT const cos_dlon1 = cos(dlon1);
        CT const cos_dlon2 = cos(dlon2);

        CT const tan_alpha1_x = cos_lat1 * tan_lat_a2 - sin_lat1 * cos_dlon1;
        CT const tan_alpha2_x = cos_lat2 * tan_lat_b2 - sin_lat2 * cos_dlon2;
                
        CT const c0 = 0;
        bool const is_vertical1 = math::equals(sin_dlon1, c0) || math::equals(tan_alpha1_x, c0);
        bool const is_vertical2 = math::equals(sin_dlon2, c0) || math::equals(tan_alpha2_x, c0);

        CT tan_alpha1 = 0;
        CT tan_alpha2 = 0;

        if (is_vertical1 && is_vertical2)
        {
            // circles intersect at one of the poles or are collinear
            return false;
        }
        else if (is_vertical1)
        {
            tan_alpha2 = sin_dlon2 / tan_alpha2_x;

            lon = lon1;
        }
        else if (is_vertical2)
        {
            tan_alpha1 = sin_dlon1 / tan_alpha1_x;

            lon = lon2;
        }
        else
        {
            tan_alpha1 = sin_dlon1 / tan_alpha1_x;
            tan_alpha2 = sin_dlon2 / tan_alpha2_x;
        
            CT const T1 = tan_alpha1 * cos_lat1;
            CT const T2 = tan_alpha2 * cos_lat2;
            CT const T1T2 = T1*T2;
            CT const tan_lon_y = T1 * sin_lon2 - T2 * sin_lon1 + T1T2 * (tan_lat1 * cos_lon1 - tan_lat2 * cos_lon2);
            CT const tan_lon_x = T1 * cos_lon2 - T2 * cos_lon1 - T1T2 * (tan_lat1 * sin_lon1 - tan_lat2 * sin_lon2);

            lon = atan2(tan_lon_y, tan_lon_x);
        }

        // choose closer result
        CT const pi = math::pi<CT>();
        CT const lon_2 = lon > c0 ? lon - pi : lon + pi;
        CT const lon_dist1 = (std::max)((std::min)(math::longitude_difference<radian>(lon1, lon),
                                                   math::longitude_difference<radian>(lon_a2, lon)),
                                        (std::min)(math::longitude_difference<radian>(lon2, lon),
                                                   math::longitude_difference<radian>(lon_b2, lon)));
        CT const lon_dist2 = (std::max)((std::min)(math::longitude_difference<radian>(lon1, lon_2),
                                                   math::longitude_difference<radian>(lon_a2, lon_2)),
                                        (std::min)(math::longitude_difference<radian>(lon2, lon_2),
                                                   math::longitude_difference<radian>(lon_b2, lon_2)));        
        if (lon_dist2 < lon_dist1)
        {
            lon = lon_2;
        }
        
        CT const sin_lon = sin(lon);
        CT const cos_lon = cos(lon);

        if (math::abs(tan_alpha1) >= math::abs(tan_alpha2)) // pick less vertical segment
        {
            CT const sin_dlon_1 = sin_lon * cos_lon1 - cos_lon * sin_lon1;
            CT const cos_dlon_1 = cos_lon * cos_lon1 + sin_lon * sin_lon1;
            CT const lat_y_1 = sin_dlon_1 + tan_alpha1 * sin_lat1 * cos_dlon_1;
            CT const lat_x_1 = tan_alpha1 * cos_lat1;
            tan_lat = lat_y_1 / lat_x_1;
        }
        else
        {
            CT const sin_dlon_2 = sin_lon * cos_lon2 - cos_lon * sin_lon2;
            CT const cos_dlon_2 = cos_lon * cos_lon2 + sin_lon * sin_lon2;
            CT const lat_y_2 = sin_dlon_2 + tan_alpha2 * sin_lat2 * cos_dlon_2;
            CT const lat_x_2 = tan_alpha2 * cos_lat2;
            tan_lat = lat_y_2 / lat_x_2;
        }
        
        return true;
    }
};


/*! Approximation of dLambda_j [Sjoberg07], expanded into taylor series in e^2
    Maxima script:
    dLI_j(c_j, sinB_j, sinB) := integrate(1 / (sqrt(1 - c_j ^ 2 - x ^ 2)*(1 + sqrt(1 - e2*(1 - x ^ 2)))), x, sinB_j, sinB);
    dL_j(c_j, B_j, B) := -e2 * c_j * dLI_j(c_j, B_j, B);
    S: taylor(dLI_j(c_j, sinB_j, sinB), e2, 0, 3);
    assume(c_j < 1);
    assume(c_j > 0);
    L1: factor(integrate(sqrt(-x ^ 2 - c_j ^ 2 + 1) / (x ^ 2 + c_j ^ 2 - 1), x));
    L2: factor(integrate(((x ^ 2 - 1)*sqrt(-x ^ 2 - c_j ^ 2 + 1)) / (x ^ 2 + c_j ^ 2 - 1), x));
    L3: factor(integrate(((x ^ 4 - 2 * x ^ 2 + 1)*sqrt(-x ^ 2 - c_j ^ 2 + 1)) / (x ^ 2 + c_j ^ 2 - 1), x));
    L4: factor(integrate(((x ^ 6 - 3 * x ^ 4 + 3 * x ^ 2 - 1)*sqrt(-x ^ 2 - c_j ^ 2 + 1)) / (x ^ 2 + c_j ^ 2 - 1), x));

\see See
    - [Sjoberg07] Lars E. Sjoberg, Geodetic intersection on the ellipsoid, 2007
      http://link.springer.com/article/10.1007/s00190-007-0204-7
*/
template <unsigned int Order, typename CT>
inline CT sjoberg_d_lambda_e_sqr(CT const& sin_betaj, CT const& sin_beta,
                                 CT const& Cj, CT const& sqrt_1_Cj_sqr,
                                 CT const& e_sqr)
{
    using math::detail::bounded;

    if (Order == 0)
    {
        return 0;
    }

    CT const c1 = 1;
    CT const c2 = 2;
        
    CT const asin_B = asin(bounded(sin_beta / sqrt_1_Cj_sqr, -c1, c1));
    CT const asin_Bj = asin(sin_betaj / sqrt_1_Cj_sqr);
    CT const L0 = (asin_B - asin_Bj) / c2;

    if (Order == 1)
    {
        return -Cj * e_sqr * L0;
    }

    CT const c0 = 0;
    CT const c16 = 16;

    CT const X = sin_beta;
    CT const Xj = sin_betaj;
    CT const X_sqr = math::sqr(X);
    CT const Xj_sqr = math::sqr(Xj);
    CT const Cj_sqr = math::sqr(Cj);
    CT const Cj_sqr_plus_one = Cj_sqr + c1;
    CT const one_minus_Cj_sqr = c1 - Cj_sqr;
    CT const sqrt_Y = math::sqrt(bounded(-X_sqr + one_minus_Cj_sqr, c0));
    CT const sqrt_Yj = math::sqrt(-Xj_sqr + one_minus_Cj_sqr);
    CT const L1 = (Cj_sqr_plus_one * (asin_B - asin_Bj) + X * sqrt_Y - Xj * sqrt_Yj) / c16;

    if (Order == 2)
    {
        return -Cj * e_sqr * (L0 + e_sqr * L1);
    }

    CT const c3 = 3;
    CT const c5 = 5;
    CT const c128 = 128;

    CT const E = Cj_sqr * (c3 * Cj_sqr + c2) + c3;
    CT const F = X * (-c2 * X_sqr + c3 * Cj_sqr + c5);
    CT const Fj = Xj * (-c2 * Xj_sqr + c3 * Cj_sqr + c5);
    CT const L2 = (E * (asin_B - asin_Bj) + F * sqrt_Y - Fj * sqrt_Yj) / c128;

    if (Order == 3)
    {
        return -Cj * e_sqr * (L0 + e_sqr * (L1 + e_sqr * L2));
    }

    CT const c8 = 8;
    CT const c9 = 9;
    CT const c10 = 10;
    CT const c15 = 15;
    CT const c24 = 24;
    CT const c26 = 26;
    CT const c33 = 33;
    CT const c6144 = 6144;

    CT const G = Cj_sqr * (Cj_sqr * (Cj_sqr * c15 + c9) + c9) + c15;
    CT const H = -c10 * Cj_sqr - c26;
    CT const I = Cj_sqr * (Cj_sqr * c15 + c24) + c33;
    CT const J = X_sqr * (X * (c8 * X_sqr + H)) + X * I;
    CT const Jj = Xj_sqr * (Xj * (c8 * Xj_sqr + H)) + Xj * I;
    CT const L3 = (G * (asin_B - asin_Bj) + J * sqrt_Y - Jj * sqrt_Yj) / c6144;

    // Order 4 and higher
    return -Cj * e_sqr * (L0 + e_sqr * (L1 + e_sqr * (L2 + e_sqr * L3)));
}

/*!
\brief The representation of geodesic as proposed by Sjoberg.
\see See
    - [Sjoberg07] Lars E. Sjoberg, Geodetic intersection on the ellipsoid, 2007
      http://link.springer.com/article/10.1007/s00190-007-0204-7
    - [Sjoberg12] Lars E. Sjoberg, Solutions to the ellipsoidal Clairaut constant
      and the inverse geodetic problem by numerical integration, 2012
      https://www.degruyter.com/view/j/jogs.2012.2.issue-3/v10156-011-0037-4/v10156-011-0037-4.xml
*/
template <typename CT, unsigned int Order>
class sjoberg_geodesic
{
    sjoberg_geodesic() {}

    static int sign_C(CT const& alphaj)
    {
        CT const c0 = 0;
        CT const c2 = 2;
        CT const pi = math::pi<CT>();
        CT const pi_half = pi / c2;

        return (pi_half < alphaj && alphaj < pi) || (-pi_half < alphaj && alphaj < c0) ? -1 : 1;
    }

public:
    sjoberg_geodesic(CT const& lon, CT const& lat, CT const& alpha, CT const& f)
        : lonj(lon)
        , latj(lat)
        , alphaj(alpha)
    {
        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;
        //CT const pi = math::pi<CT>();
        //CT const pi_half = pi / c2;

        one_minus_f = c1 - f;
        e_sqr = f * (c2 - f);

        tan_latj = tan(lat);
        tan_betaj = one_minus_f * tan_latj;
        betaj = atan(tan_betaj);
        sin_betaj = sin(betaj);

        cos_betaj = cos(betaj);
        sin_alphaj = sin(alphaj);
        // Clairaut constant (lower-case in the paper)
        Cj = sign_C(alphaj) * cos_betaj * sin_alphaj;
        Cj_sqr = math::sqr(Cj);
        sqrt_1_Cj_sqr = math::sqrt(c1 - Cj_sqr);

        sign_lon_diff = alphaj >= 0 ? 1 : -1; // || alphaj == -pi ?
        //sign_lon_diff = 1;

        is_on_equator = math::equals(sqrt_1_Cj_sqr, c0);
        is_Cj_zero = math::equals(Cj, c0);

        t0j = c0;
        asin_tj_t0j = c0;

        if (! is_Cj_zero)
        {
            t0j = sqrt_1_Cj_sqr / Cj;
        }

        if (! is_on_equator)
        {
            //asin_tj_t0j = asin(tan_betaj / t0j);
            asin_tj_t0j = asin(tan_betaj * Cj / sqrt_1_Cj_sqr);
        }
    }

    struct vertex_data
    {
        //CT beta0j;
        CT sin_beta0j;
        CT dL0j;
        CT lon0j;
    };

    vertex_data get_vertex_data() const
    {
        CT const c2 = 2;
        CT const pi = math::pi<CT>();
        CT const pi_half = pi / c2;

        vertex_data res;

        if (! is_Cj_zero)
        {
            //res.beta0j = atan(t0j);
            //res.sin_beta0j = sin(res.beta0j);
            res.sin_beta0j = math::sign(t0j) * sqrt_1_Cj_sqr;
            res.dL0j = d_lambda(res.sin_beta0j);
            res.lon0j = lonj + sign_lon_diff * (pi_half - asin_tj_t0j + res.dL0j);
        }
        else
        {
            //res.beta0j = pi_half;
            //res.sin_beta0j = betaj >= 0 ? 1 : -1;
            res.sin_beta0j = 1;
            res.dL0j = 0;
            res.lon0j = lonj;
        }

        return res;
    }

    bool is_sin_beta_ok(CT const& sin_beta) const
    {
        CT const c1 = 1;
        return math::abs(sin_beta / sqrt_1_Cj_sqr) <= c1;
    }

    bool k_diff(CT const& sin_beta,
                CT & delta_k) const
    {
        if (is_Cj_zero)
        {
            delta_k = 0;
            return true;
        }

        // beta out of bounds and not close
        if (! (is_sin_beta_ok(sin_beta)
                || math::equals(math::abs(sin_beta), sqrt_1_Cj_sqr)) )
        {
            return false;
        }
        
        // NOTE: beta may be slightly out of bounds here but d_lambda handles that
        CT const dLj = d_lambda(sin_beta);
        delta_k = sign_lon_diff * (/*asin_t_t0j*/ - asin_tj_t0j + dLj);

        return true;
    }

    bool lon_diff(CT const& sin_beta, CT const& t,
                  CT & delta_lon) const
    {
        using math::detail::bounded;
        CT const c1 = 1;

        if (is_Cj_zero)
        {
            delta_lon = 0;
            return true;
        }

        CT delta_k = 0;
        if (! k_diff(sin_beta, delta_k))
        {
            return false;
        }

        CT const t_t0j = t / t0j;
        // NOTE: t may be slightly out of bounds here
        CT const asin_t_t0j = asin(bounded(t_t0j, -c1, c1));
        delta_lon = sign_lon_diff * asin_t_t0j + delta_k;

        return true;
    }

    bool k_diffs(CT const& sin_beta, vertex_data const& vd,
                 CT & delta_k_before, CT & delta_k_behind,
                 bool check_sin_beta = true) const
    {
        CT const pi = math::pi<CT>();

        if (is_Cj_zero)
        {
            delta_k_before = 0;
            delta_k_behind = sign_lon_diff * pi;
            return true;
        }
        
        // beta out of bounds and not close
        if (check_sin_beta
            && ! (is_sin_beta_ok(sin_beta)
                    || math::equals(math::abs(sin_beta), sqrt_1_Cj_sqr)) )
        {
            return false;
        }
        
        // NOTE: beta may be slightly out of bounds here but d_lambda handles that
        CT const dLj = d_lambda(sin_beta);
        delta_k_before = sign_lon_diff * (/*asin_t_t0j*/ - asin_tj_t0j + dLj);

        // This version require no additional dLj calculation
        delta_k_behind = sign_lon_diff * (pi /*- asin_t_t0j*/ - asin_tj_t0j + vd.dL0j + (vd.dL0j - dLj));

        // [Sjoberg12]
        //CT const dL101 = d_lambda(sin_betaj, vd.sin_beta0j);
        // WARNING: the following call might not work if beta was OoB because only the second argument is bounded
        //CT const dL_01 = d_lambda(sin_beta, vd.sin_beta0j);
        //delta_k_behind = sign_lon_diff * (pi /*- asin_t_t0j*/ - asin_tj_t0j + dL101 + dL_01);

        return true;
    }

    bool lon_diffs(CT const& sin_beta, CT const& t, vertex_data const& vd,
                   CT & delta_lon_before, CT & delta_lon_behind) const
    {
        using math::detail::bounded;
        CT const c1 = 1;
        CT const pi = math::pi<CT>();

        if (is_Cj_zero)
        {
            delta_lon_before = 0;
            delta_lon_behind = sign_lon_diff * pi;
            return true;
        }

        CT delta_k_before = 0, delta_k_behind = 0;
        if (! k_diffs(sin_beta, vd, delta_k_before, delta_k_behind))
        {
            return false;
        }

        CT const t_t0j = t / t0j;
        // NOTE: t may be slightly out of bounds here
        CT const asin_t_t0j = asin(bounded(t_t0j, -c1, c1));
        CT const sign_asin_t_t0j = sign_lon_diff * asin_t_t0j;
        delta_lon_before = sign_asin_t_t0j + delta_k_before;
        delta_lon_behind = -sign_asin_t_t0j + delta_k_behind;

        return true;
    }

    bool lon(CT const& sin_beta, CT const& t, vertex_data const& vd,
             CT & lon_before, CT & lon_behind) const
    {
        using math::detail::bounded;
        CT const c1 = 1;
        CT const pi = math::pi<CT>();

        if (is_Cj_zero)
        {
            lon_before = lonj;
            lon_behind = lonj + sign_lon_diff * pi;
            return true;
        }

        if (! (is_sin_beta_ok(sin_beta)
                || math::equals(math::abs(sin_beta), sqrt_1_Cj_sqr)) )
        {
            return false;
        }

        CT const t_t0j = t / t0j;
        CT const asin_t_t0j = asin(bounded(t_t0j, -c1, c1));
        CT const dLj = d_lambda(sin_beta);
        lon_before = lonj + sign_lon_diff * (asin_t_t0j - asin_tj_t0j + dLj);
        lon_behind = vd.lon0j + (vd.lon0j - lon_before);

        return true;
    }


    CT lon(CT const& delta_lon) const
    {
        return lonj + delta_lon;
    }

    CT lat(CT const& t) const
    {
        // t = tan(beta) = (1-f)tan(lat)
        return atan(t / one_minus_f);
    }

    void vertex(CT & lon, CT & lat) const
    {
        lon = get_vertex_data().lon0j;
        if (! is_Cj_zero)
        {
            lat = sjoberg_geodesic::lat(t0j);
        }
        else
        {
            CT const c2 = 2;
            lat = math::pi<CT>() / c2;
        }
    }

    CT lon_of_equator_intersection() const
    {
        CT const c0 = 0;
        CT const dLj = d_lambda(c0);
        CT const asin_tj_t0j = asin(Cj * tan_betaj / sqrt_1_Cj_sqr);
        return lonj - asin_tj_t0j + dLj;
    }

    CT d_lambda(CT const& sin_beta) const
    {
        return sjoberg_d_lambda_e_sqr<Order>(sin_betaj, sin_beta, Cj, sqrt_1_Cj_sqr, e_sqr);
    }

    // [Sjoberg12]
    /*CT d_lambda(CT const& sin_beta1, CT const& sin_beta2) const
    {
        return sjoberg_d_lambda_e_sqr<Order>(sin_beta1, sin_beta2, Cj, sqrt_1_Cj_sqr, e_sqr);
    }*/

    CT lonj;
    CT latj;
    CT alphaj;

    CT one_minus_f;
    CT e_sqr;

    CT tan_latj;
    CT tan_betaj;
    CT betaj;
    CT sin_betaj;
    CT cos_betaj;
    CT sin_alphaj;
    CT Cj;
    CT Cj_sqr;
    CT sqrt_1_Cj_sqr;

    int sign_lon_diff;

    bool is_on_equator;
    bool is_Cj_zero;

    CT t0j;
    CT asin_tj_t0j;
};


/*!
\brief The intersection of two geodesics as proposed by Sjoberg.
\see See
    - [Sjoberg02] Lars E. Sjoberg, Intersections on the sphere and ellipsoid, 2002
      http://link.springer.com/article/10.1007/s00190-001-0230-9
    - [Sjoberg07] Lars E. Sjoberg, Geodetic intersection on the ellipsoid, 2007
      http://link.springer.com/article/10.1007/s00190-007-0204-7
    - [Sjoberg12] Lars E. Sjoberg, Solutions to the ellipsoidal Clairaut constant
      and the inverse geodetic problem by numerical integration, 2012
      https://www.degruyter.com/view/j/jogs.2012.2.issue-3/v10156-011-0037-4/v10156-011-0037-4.xml
*/
template
<
    typename CT,
    template <typename, bool, bool, bool, bool, bool> class Inverse,
    unsigned int Order = 4
>
class sjoberg_intersection
{
    typedef sjoberg_geodesic<CT, Order> geodesic_type;
    typedef Inverse<CT, false, true, false, false, false> inverse_type;
    typedef typename inverse_type::result_type inverse_result;

    static bool const enable_02 = true;
    static int const max_iterations_02 = 10;
    static int const max_iterations_07 = 20;

public:
    template <typename T1, typename T2, typename Spheroid>
    static inline bool apply(T1 const& lona1, T1 const& lata1,
                             T1 const& lona2, T1 const& lata2,
                             T2 const& lonb1, T2 const& latb1,
                             T2 const& lonb2, T2 const& latb2,
                             CT & lon, CT & lat,
                             Spheroid const& spheroid)
    {
        CT const lon_a1 = lona1;
        CT const lat_a1 = lata1;
        CT const lon_a2 = lona2;
        CT const lat_a2 = lata2;
        CT const lon_b1 = lonb1;
        CT const lat_b1 = latb1;
        CT const lon_b2 = lonb2;
        CT const lat_b2 = latb2;

        inverse_result const res1 = inverse_type::apply(lon_a1, lat_a1, lon_a2, lat_a2, spheroid);
        inverse_result const res2 = inverse_type::apply(lon_b1, lat_b1, lon_b2, lat_b2, spheroid);

        return apply(lon_a1, lat_a1, lon_a2, lat_a2, res1.azimuth,
                     lon_b1, lat_b1, lon_b2, lat_b2, res2.azimuth,
                     lon, lat, spheroid);
    }

    // TODO: Currently may not work correctly if one of the endpoints is the pole
    template <typename Spheroid>
    static inline bool apply(CT const& lon_a1, CT const& lat_a1, CT const& lon_a2, CT const& lat_a2, CT const& alpha_a1,
                             CT const& lon_b1, CT const& lat_b1, CT const& lon_b2, CT const& lat_b2, CT const& alpha_b1,
                             CT & lon, CT & lat,
                             Spheroid const& spheroid)
    {
        // coordinates in radians

        CT const c0 = 0;
        CT const c1 = 1;

        CT const f = formula::flattening<CT>(spheroid);
        CT const one_minus_f = c1 - f;

        geodesic_type geod1(lon_a1, lat_a1, alpha_a1, f);
        geodesic_type geod2(lon_b1, lat_b1, alpha_b1, f);

        // Cj = 1 if on equator <=> sqrt_1_Cj_sqr = 0
        // Cj = 0 if vertical <=> sqrt_1_Cj_sqr = 1

        if (geod1.is_on_equator && geod2.is_on_equator)
        {
            return false;
        }
        else if (geod1.is_on_equator)
        {
            lon = geod2.lon_of_equator_intersection();
            lat = c0;
            return true;
        }
        else if (geod2.is_on_equator)
        {
            lon = geod1.lon_of_equator_intersection();
            lat = c0;
            return true;
        }

        // (lon1 - lon2) normalized to (-180, 180]
        CT const lon1_minus_lon2 = math::longitude_distance_signed<radian>(geod2.lonj, geod1.lonj);

        // vertical segments
        if (geod1.is_Cj_zero && geod2.is_Cj_zero)
        {
            CT const pi = math::pi<CT>();

            // the geodesics are parallel, the intersection point cannot be calculated
            if ( math::equals(lon1_minus_lon2, c0)
              || math::equals(lon1_minus_lon2 + (lon1_minus_lon2 < c0 ? pi : -pi), c0) )
            {
                return false;
            }

            lon = c0;

            // the geodesics intersect at one of the poles
            CT const pi_half = pi / CT(2);
            CT const abs_lat_a1 = math::abs(lat_a1);
            CT const abs_lat_a2 = math::abs(lat_a2);
            if (math::equals(abs_lat_a1, abs_lat_a2))
            {
                lat = pi_half;
            }
            else
            {
                // pick the pole closest to one of the points of the first segment
                CT const& closer_lat = abs_lat_a1 > abs_lat_a2 ? lat_a1 : lat_a2;
                lat = closer_lat >= 0 ? pi_half : -pi_half;
            }

            return true;
        }

        CT lon_sph = 0;

        // Starting tan(beta)
        CT t = 0;

        /*if (geod1.is_Cj_zero)
        {
            CT const k_base = lon1_minus_lon2 + geod2.sign_lon_diff * geod2.asin_tj_t0j;
            t = sin(k_base) * geod2.t0j;
            lon_sph = vertical_intersection_longitude(geod1.lonj, lon_b1, lon_b2);
        }
        else if (geod2.is_Cj_zero)
        {
            CT const k_base = lon1_minus_lon2 - geod1.sign_lon_diff * geod1.asin_tj_t0j;
            t = sin(-k_base) * geod1.t0j;
            lon_sph = vertical_intersection_longitude(geod2.lonj, lon_a1, lon_a2);
        }
        else*/
        {
            // TODO: Consider using betas instead of latitudes.
            //       Some function calls might be saved this way.
            CT tan_lat_sph = 0;
            sjoberg_intersection_spherical_02<CT>::apply_alt(lon_a1, lat_a1, lon_a2, lat_a2,
                                                             lon_b1, lat_b1, lon_b2, lat_b2,
                                                             lon_sph, tan_lat_sph);

            // Return for sphere
            if (math::equals(f, c0))
            {
                lon = lon_sph;
                lat = atan(tan_lat_sph);
                return true;
            }

            t = one_minus_f * tan_lat_sph; // tan(beta)
        }        

        // TODO: no need to calculate atan here if reduced latitudes were used
        //       instead of latitudes above, in sjoberg_intersection_spherical_02
        CT const beta = atan(t);

        if (enable_02 && newton_method(geod1, geod2, beta, t, lon1_minus_lon2, lon_sph, lon, lat))
        {
            // TODO: Newton's method may return wrong result in some specific cases
            // Detected for sphere and nearly sphere, e.g. A=6371228, B=6371227
            // and segments s1=(-121 -19,37 8) and s2=(-19 -15,-104 -58)
            // It's unclear if this is a bug or a characteristic of this method
            // so until this is investigated check if the resulting longitude is
            // between endpoints of the segments. It should be since before calling
            // this formula sides of endpoints WRT other segments are checked.
            if ( is_result_longitude_ok(geod1, lon_a1, lon_a2, lon)
              && is_result_longitude_ok(geod2, lon_b1, lon_b2, lon) )
            {
                return true;
            }
        }

        return converge_07(geod1, geod2, beta, t, lon1_minus_lon2, lon_sph, lon, lat);
    }

private:
    static inline bool newton_method(geodesic_type const& geod1, geodesic_type const& geod2, // in
                                     CT beta, CT t, CT const& lon1_minus_lon2, CT const& lon_sph, // in
                                     CT & lon, CT & lat) // out
    {
        CT const c0 = 0;
        CT const c1 = 1;

        CT const e_sqr = geod1.e_sqr;
        
        CT lon1_diff = 0;
        CT lon2_diff = 0;

        // The segment is vertical and intersection point is behind the vertex
        // this method is unable to calculate correct result
        if (geod1.is_Cj_zero && math::abs(geod1.lonj - lon_sph) > math::half_pi<CT>())
            return false;
        if (geod2.is_Cj_zero && math::abs(geod2.lonj - lon_sph) > math::half_pi<CT>())
            return false;

        CT abs_dbeta_last = 0;

        // [Sjoberg02] converges faster than solution in [Sjoberg07]
        // Newton-Raphson method
        for (int i = 0; i < max_iterations_02; ++i)
        {
            CT const cos_beta = cos(beta);

            if (math::equals(cos_beta, c0))
            {
                return false;
            }

            CT const sin_beta = sin(beta);
            CT const cos_beta_sqr = math::sqr(cos_beta);
            CT const G = c1 - e_sqr * cos_beta_sqr;

            CT f1 = 0;
            CT f2 = 0;

            if (!geod1.is_Cj_zero)
            {
                bool is_beta_ok = geod1.lon_diff(sin_beta, t, lon1_diff);

                if (is_beta_ok)
                {
                    CT const H = cos_beta_sqr - geod1.Cj_sqr;
                    if (math::equals(H, c0))
                    {
                        return false;
                    }
                    f1 = geod1.Cj / cos_beta * math::sqrt(G / H);
                }
                else
                {
                    return false;
                }
            }

            if (!geod2.is_Cj_zero)
            {
                bool is_beta_ok = geod2.lon_diff(sin_beta, t, lon2_diff);

                if (is_beta_ok)
                {
                    CT const H = cos_beta_sqr - geod2.Cj_sqr;
                    if (math::equals(H, c0))
                    {
                        // NOTE: This may happen for segment nearly
                        // at the equator. Detected for (radian):
                        // (-0.0872665 -0.0872665, -0.0872665 0.0872665)
                        // x
                        // (0 1.57e-07, -0.392699 1.57e-07)
                        return false;
                    }
                    f2 = geod2.Cj / cos_beta * math::sqrt(G / H);
                }
                else
                {
                    return false;
                }
            }

            // NOTE: Things may go wrong if the IP is near the vertex
            //   1. May converge into the wrong direction (from the other way around).
            //      This happens when the starting point is on the other side than the vertex
            //   2. During converging may "jump" into the other side of the vertex.
            //      In this case sin_beta/sqrt_1_Cj_sqr and t/t0j is not in [-1, 1]
            //   3. f1-f2 may be 0 which means that the intermediate point is on the vertex
            //      In this case it's not possible to check if this is the correct result
            //   4. f1-f2 may also be 0 in other cases, e.g.
            //      geodesics are symmetrical wrt equator and longitude directions are different

            CT const dbeta_denom = f1 - f2;
            //CT const dbeta_denom = math::abs(f1) + math::abs(f2);

            if (math::equals(dbeta_denom, c0))
            {
                return false;
            }            

            // The sign of dbeta is changed WRT [Sjoberg02]
            CT const dbeta = (lon1_minus_lon2 + lon1_diff - lon2_diff) / dbeta_denom;

            CT const abs_dbeta = math::abs(dbeta);
            if (i > 0 && abs_dbeta > abs_dbeta_last)
            {
                // The algorithm is not converging
                // The intersection may be on the other side of the vertex
                return false;
            }
            abs_dbeta_last = abs_dbeta;

            if (math::equals(dbeta, c0))
            {
                // Result found
                break;
            }

            // Because the sign of dbeta is changed WRT [Sjoberg02] dbeta is subtracted here
            beta = beta - dbeta;

            t = tan(beta);
        }

        lat = geod1.lat(t);
        // NOTE: if Cj is 0 then the result is lonj or lonj+180
        lon = ! geod1.is_Cj_zero
                ? geod1.lon(lon1_diff)
                : geod2.lon(lon2_diff);

        return true;
    }

    static inline bool is_result_longitude_ok(geodesic_type const& geod,
                                              CT const& lon1, CT const& lon2, CT const& lon)
    {
        CT const c0 = 0;

        if (geod.is_Cj_zero)
            return true; // don't check vertical segment

        CT dist1p = math::longitude_distance_signed<radian>(lon1, lon);
        CT dist12 = math::longitude_distance_signed<radian>(lon1, lon2);

        if (dist12 < c0)
        {
            dist1p = -dist1p;
            dist12 = -dist12;
        }

        return (c0 <= dist1p && dist1p <= dist12)
            || math::equals(dist1p, c0)
            || math::equals(dist1p, dist12);
    }

    struct geodesics_type
    {
        geodesics_type(geodesic_type const& g1, geodesic_type const& g2)
            : geod1(g1)
            , geod2(g2)
            , vertex1(geod1.get_vertex_data())
            , vertex2(geod2.get_vertex_data())
        {}

        geodesic_type const& geod1;
        geodesic_type const& geod2;
        typename geodesic_type::vertex_data vertex1;
        typename geodesic_type::vertex_data vertex2;
    };

    struct converge_07_result
    {
        converge_07_result()
            : lon1(0), lon2(0), k1_diff(0), k2_diff(0), t1(0), t2(0)
        {}

        CT lon1, lon2;
        CT k1_diff, k2_diff;
        CT t1, t2;
    };

    static inline bool converge_07(geodesic_type const& geod1, geodesic_type const& geod2,
                                   CT beta, CT t,
                                   CT const& lon1_minus_lon2, CT const& lon_sph,
                                   CT & lon, CT & lat)
    {
        //CT const c0 = 0;
        //CT const c1 = 1;
        //CT const c2 = 2;
        //CT const pi = math::pi<CT>();

        geodesics_type geodesics(geod1, geod2);
        converge_07_result result;

        // calculate first pair of longitudes
        if (!converge_07_step_one(CT(sin(beta)), t, lon1_minus_lon2, geodesics, lon_sph, result, false))
        {
            return false;
        }

        int t_direction = 0;

        CT lon_diff_prev = math::longitude_difference<radian>(result.lon1, result.lon2);

        // [Sjoberg07]
        for (int i = 2; i < max_iterations_07; ++i)
        {
            // pick t candidates from previous result based on dir
            CT t_cand1 = result.t1;
            CT t_cand2 = result.t2;
            // if direction is 0 the closer one is the first
            if (t_direction < 0)
            {
                t_cand1 = (std::min)(result.t1, result.t2);
                t_cand2 = (std::max)(result.t1, result.t2);
            }
            else if (t_direction > 0)
            {
                t_cand1 = (std::max)(result.t1, result.t2);
                t_cand2 = (std::min)(result.t1, result.t2);
            }
            else
            {
                t_direction = t_cand1 < t_cand2 ? -1 : 1;
            }

            CT t1 = t;
            CT beta1 = beta;
            // check if the further calculation is needed
            if (converge_07_update(t1, beta1, t_cand1))
            {
                break;
            }
                    
            bool try_t2 = false;
            converge_07_result result_curr;
            if (converge_07_step_one(CT(sin(beta1)), t1, lon1_minus_lon2, geodesics, lon_sph, result_curr))
            {
                CT const lon_diff1 = math::longitude_difference<radian>(result_curr.lon1, result_curr.lon2);
                if (lon_diff_prev > lon_diff1)
                {
                    t = t1;
                    beta = beta1;
                    lon_diff_prev = lon_diff1;
                    result = result_curr;
                }
                else if (t_cand1 != t_cand2)
                {
                    try_t2 = true;
                }
                else
                {
                    // the result is not fully correct but it won't be more accurate
                    break;
                }
            }
            // ! converge_07_step_one
            else
            {
                if (t_cand1 != t_cand2)
                {
                    try_t2 = true;
                }
                else
                {
                    return false;
                }
            }

            
            if (try_t2)
            {
                CT t2 = t;
                CT beta2 = beta;
                // check if the further calculation is needed
                if (converge_07_update(t2, beta2, t_cand2))
                {
                    break;
                }

                if (! converge_07_step_one(CT(sin(beta2)), t2, lon1_minus_lon2, geodesics, lon_sph, result_curr))
                {
                    return false;
                }

                CT const lon_diff2 = math::longitude_difference<radian>(result_curr.lon1, result_curr.lon2);
                if (lon_diff_prev > lon_diff2)
                {
                    t_direction *= -1;
                    t = t2;
                    beta = beta2;
                    lon_diff_prev = lon_diff2;
                    result = result_curr;
                }
                else
                {
                    // the result is not fully correct but it won't be more accurate
                    break;
                }
            }
        }

        lat = geod1.lat(t);
        lon = ! geod1.is_Cj_zero ? result.lon1 : result.lon2;
        math::normalize_longitude<radian>(lon);

        return true;
    }

    static inline bool converge_07_update(CT & t, CT & beta, CT const& t_new)
    {
        CT const c0 = 0;

        CT const beta_new = atan(t_new);
        CT const dbeta = beta_new - beta;
        beta = beta_new;
        t = t_new;

        return math::equals(dbeta, c0);
    }

    static inline CT const& pick_t(CT const& t1, CT const& t2, int direction)
    {
        return direction < 0 ? (std::min)(t1, t2) : (std::max)(t1, t2);
    }

    static inline bool converge_07_step_one(CT const& sin_beta,
                                            CT const& t,
                                            CT const& lon1_minus_lon2,
                                            geodesics_type const& geodesics,
                                            CT const& lon_sph,
                                            converge_07_result & result,
                                            bool check_sin_beta = true)
    {
        bool ok = converge_07_one_geod(sin_beta, t, geodesics.geod1, geodesics.vertex1, lon_sph,
                                       result.lon1, result.k1_diff, check_sin_beta)
               && converge_07_one_geod(sin_beta, t, geodesics.geod2, geodesics.vertex2, lon_sph,
                                       result.lon2, result.k2_diff, check_sin_beta);

        if (!ok)
        {
            return false;
        }

        CT const k = lon1_minus_lon2 + result.k1_diff - result.k2_diff;

        // get 2 possible ts one lesser and one greater than t
        // t1 is the closer one
        calc_ts(t, k, geodesics.geod1, geodesics.geod2, result.t1, result.t2);

        return true;
    }

    static inline bool converge_07_one_geod(CT const& sin_beta, CT const& t,
                                            geodesic_type const& geod,
                                            typename geodesic_type::vertex_data const& vertex,
                                            CT const& lon_sph,
                                            CT & lon, CT & k_diff,
                                            bool check_sin_beta)
    {
        using math::detail::bounded;
        CT const c1 = 1;
        
        CT k_diff_before = 0;
        CT k_diff_behind = 0;

        bool is_beta_ok = geod.k_diffs(sin_beta, vertex, k_diff_before, k_diff_behind, check_sin_beta);

        if (! is_beta_ok)
        {
            return false;
        }

        CT const asin_t_t0j = ! geod.is_Cj_zero ? asin(bounded(t / geod.t0j, -c1, c1)) : 0;
        CT const sign_asin_t_t0j = geod.sign_lon_diff * asin_t_t0j;

        CT const lon_before = geod.lonj + sign_asin_t_t0j + k_diff_before;
        CT const lon_behind = geod.lonj - sign_asin_t_t0j + k_diff_behind;

        CT const lon_dist_before = math::longitude_distance_signed<radian>(lon_before, lon_sph);
        CT const lon_dist_behind = math::longitude_distance_signed<radian>(lon_behind, lon_sph);
        if (math::abs(lon_dist_before) <= math::abs(lon_dist_behind))
        {
            k_diff = k_diff_before;
            lon = lon_before;
        }
        else
        {
            k_diff = k_diff_behind;
            lon = lon_behind;
        }

        return true;
    }

    static inline void calc_ts(CT const& t, CT const& k,
                               geodesic_type const& geod1, geodesic_type const& geod2,
                               CT & t1, CT& t2)
    {
        CT const c0 = 0;
        CT const c1 = 1;
        CT const c2 = 2;

        CT const K = sin(k);

        BOOST_GEOMETRY_ASSERT(!geod1.is_Cj_zero || !geod2.is_Cj_zero);
        if (geod1.is_Cj_zero)
        {
            t1 = K * geod2.t0j;
            t2 = -t1;
        }
        else if (geod2.is_Cj_zero)
        {
            t1 = -K * geod1.t0j;
            t2 = -t1;
        }
        else
        {
            CT const A = math::sqr(geod1.t0j) + math::sqr(geod2.t0j);
            CT const B = c2 * geod1.t0j * geod2.t0j * math::sqrt(c1 - math::sqr(K));

            CT const K_t01_t02 = K * geod1.t0j * geod2.t0j;
            CT const D1 = math::sqrt(A + B);
            CT const D2 = math::sqrt(A - B);
            CT const t_new1 = math::equals(D1, c0) ? c0 : K_t01_t02 / D1;
            CT const t_new2 = math::equals(D2, c0) ? c0 : K_t01_t02 / D2;
            CT const t_new3 = -t_new1;
            CT const t_new4 = -t_new2;

            // Pick 2 nearest t_new, one greater and one lesser than current t
            CT const abs_t_new1 = math::abs(t_new1);
            CT const abs_t_new2 = math::abs(t_new2);
            CT const abs_t_max = (std::max)(abs_t_new1, abs_t_new2);
            t1 = -abs_t_max; // lesser
            t2 = abs_t_max; // greater
            if (t1 < t)
            {
                if (t_new1 < t && t_new1 > t1)
                    t1 = t_new1;
                if (t_new2 < t && t_new2 > t1)
                    t1 = t_new2;
                if (t_new3 < t && t_new3 > t1)
                    t1 = t_new3;
                if (t_new4 < t && t_new4 > t1)
                    t1 = t_new4;
            }
            if (t2 > t)
            {
                if (t_new1 > t && t_new1 < t2)
                    t2 = t_new1;
                if (t_new2 > t && t_new2 < t2)
                    t2 = t_new2;
                if (t_new3 > t && t_new3 < t2)
                    t2 = t_new3;
                if (t_new4 > t && t_new4 < t2)
                    t2 = t_new4;
            }
        }

        // the first one is the closer one
        if (math::abs(t - t2) < math::abs(t - t1))
        {
            std::swap(t2, t1);
        }
    }

    static inline CT fj(CT const& cos_beta, CT const& cos2_beta, CT const& Cj, CT const& e_sqr)
    {
        CT const c1 = 1;
        CT const Cj_sqr = math::sqr(Cj);
        return Cj / cos_beta * math::sqrt((c1 - e_sqr * cos2_beta) / (cos2_beta - Cj_sqr));
    }

    /*static inline CT vertical_intersection_longitude(CT const& ip_lon, CT const& seg_lon1, CT const& seg_lon2)
    {
        CT const c0 = 0;
        CT const lon_2 = ip_lon > c0 ? ip_lon - pi : ip_lon + pi;

        return (std::min)(math::longitude_difference<radian>(ip_lon, seg_lon1),
                          math::longitude_difference<radian>(ip_lon, seg_lon2))
            <=
               (std::min)(math::longitude_difference<radian>(lon_2, seg_lon1),
                          math::longitude_difference<radian>(lon_2, seg_lon2))
            ? ip_lon : lon_2;
    }*/
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_SJOBERG_INTERSECTION_HPP
