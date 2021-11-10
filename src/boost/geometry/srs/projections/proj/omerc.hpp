// Boost.Geometry - gis-projections (based on PROJ4)

// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2018, 2019.
// Modifications copyright (c) 2017-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Boost.Geometry by Barend Gehrels

// Last updated version of proj: 5.0.0

// Original copyright notice:

// Copyright (c) 2003, 2006   Gerald I. Evenden

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_GEOMETRY_PROJECTIONS_OMERC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_OMERC_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/pj_phi2.hpp>
#include <boost/geometry/srs/projections/impl/pj_tsfn.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace omerc
    {
            template <typename T>
            struct par_omerc
            {
                T    A, B, E, AB, ArB, BrA, rB, singam, cosgam, sinrot, cosrot;
                T    v_pole_n, v_pole_s, u_0;
                bool no_rot;
            };

            static const double tolerance = 1.e-7;
            static const double epsilon = 1.e-10;

            template <typename T, typename Parameters>
            struct base_omerc_ellipsoid
            {
                par_omerc<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T  s, t, U, V, W, temp, u, v;

                    if (fabs(fabs(lp_lat) - half_pi) > epsilon) {
                        W = this->m_proj_parm.E / math::pow(pj_tsfn(lp_lat, sin(lp_lat), par.e), this->m_proj_parm.B);
                        temp = 1. / W;
                        s = .5 * (W - temp);
                        t = .5 * (W + temp);
                        V = sin(this->m_proj_parm.B * lp_lon);
                        U = (s * this->m_proj_parm.singam - V * this->m_proj_parm.cosgam) / t;
                        if (fabs(fabs(U) - 1.0) < epsilon) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        v = 0.5 * this->m_proj_parm.ArB * log((1. - U)/(1. + U));
                        temp = cos(this->m_proj_parm.B * lp_lon);
                        if(fabs(temp) < tolerance) {
                            u = this->m_proj_parm.A * lp_lon;
                        } else {
                            u = this->m_proj_parm.ArB * atan2((s * this->m_proj_parm.cosgam + V * this->m_proj_parm.singam), temp);
                        }
                    } else {
                        v = lp_lat > 0 ? this->m_proj_parm.v_pole_n : this->m_proj_parm.v_pole_s;
                        u = this->m_proj_parm.ArB * lp_lat;
                    }
                    if (this->m_proj_parm.no_rot) {
                        xy_x = u;
                        xy_y = v;
                    } else {
                        u -= this->m_proj_parm.u_0;
                        xy_x = v * this->m_proj_parm.cosrot + u * this->m_proj_parm.sinrot;
                        xy_y = u * this->m_proj_parm.cosrot - v * this->m_proj_parm.sinrot;
                    }
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T  u, v, Qp, Sp, Tp, Vp, Up;

                    if (this->m_proj_parm.no_rot) {
                        v = xy_y;
                        u = xy_x;
                    } else {
                        v = xy_x * this->m_proj_parm.cosrot - xy_y * this->m_proj_parm.sinrot;
                        u = xy_y * this->m_proj_parm.cosrot + xy_x * this->m_proj_parm.sinrot + this->m_proj_parm.u_0;
                    }
                    Qp = exp(- this->m_proj_parm.BrA * v);
                    Sp = .5 * (Qp - 1. / Qp);
                    Tp = .5 * (Qp + 1. / Qp);
                    Vp = sin(this->m_proj_parm.BrA * u);
                    Up = (Vp * this->m_proj_parm.cosgam + Sp * this->m_proj_parm.singam) / Tp;
                    if (fabs(fabs(Up) - 1.) < epsilon) {
                        lp_lon = 0.;
                        lp_lat = Up < 0. ? -half_pi : half_pi;
                    } else {
                        lp_lat = this->m_proj_parm.E / sqrt((1. + Up) / (1. - Up));
                        if ((lp_lat = pj_phi2(math::pow(lp_lat, T(1) / this->m_proj_parm.B), par.e)) == HUGE_VAL) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        lp_lon = - this->m_proj_parm.rB * atan2((Sp * this->m_proj_parm.cosgam -
                            Vp * this->m_proj_parm.singam), cos(this->m_proj_parm.BrA * u));
                    }
                }

                static inline std::string get_name()
                {
                    return "omerc_ellipsoid";
                }

            };

            // Oblique Mercator
            template <typename Params, typename Parameters, typename T>
            inline void setup_omerc(Params const& params, Parameters & par, par_omerc<T>& proj_parm)
            {
                static const T fourth_pi = detail::fourth_pi<T>();
                static const T half_pi = detail::half_pi<T>();
                static const T pi = detail::pi<T>();
                static const T two_pi = detail::two_pi<T>();

                T con, com, cosph0, D, F, H, L, sinph0, p, J, gamma=0,
                  gamma0, lamc=0, lam1=0, lam2=0, phi1=0, phi2=0, alpha_c=0;
                int alp, gam, no_off = 0;

                proj_parm.no_rot = pj_get_param_b<srs::spar::no_rot>(params, "no_rot", srs::dpar::no_rot);
                alp = pj_param_r<srs::spar::alpha>(params, "alpha", srs::dpar::alpha, alpha_c);
                gam = pj_param_r<srs::spar::gamma>(params, "gamma", srs::dpar::gamma, gamma);
                if (alp || gam) {
                    lamc = pj_get_param_r<T, srs::spar::lonc>(params, "lonc", srs::dpar::lonc);
                    // NOTE: This is not needed in Boost.Geometry
                    //no_off =
                    //            /* For libproj4 compatability */
                    //            pj_param_exists(par.params, "no_off")
                    //            /* for backward compatibility */
                    //            || pj_param_exists(par.params, "no_uoff");
                    //if( no_off )
                    //{
                    //    /* Mark the parameter as used, so that the pj_get_def() return them */
                    //    pj_get_param_s(par.params, "no_uoff");
                    //    pj_get_param_s(par.params, "no_off");
                    //}
                } else {
                    lam1 = pj_get_param_r<T, srs::spar::lon_1>(params, "lon_1", srs::dpar::lon_1);
                    phi1 = pj_get_param_r<T, srs::spar::lat_1>(params, "lat_1", srs::dpar::lat_1);
                    lam2 = pj_get_param_r<T, srs::spar::lon_2>(params, "lon_2", srs::dpar::lon_2);
                    phi2 = pj_get_param_r<T, srs::spar::lat_2>(params, "lat_2", srs::dpar::lat_2);
                    if (fabs(phi1 - phi2) <= tolerance ||
                        (con = fabs(phi1)) <= tolerance ||
                        fabs(con - half_pi) <= tolerance ||
                        fabs(fabs(par.phi0) - half_pi) <= tolerance ||
                        fabs(fabs(phi2) - half_pi) <= tolerance)
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_0_or_alpha_eq_90) );
                }
                com = sqrt(par.one_es);
                if (fabs(par.phi0) > epsilon) {
                    sinph0 = sin(par.phi0);
                    cosph0 = cos(par.phi0);
                    con = 1. - par.es * sinph0 * sinph0;
                    proj_parm.B = cosph0 * cosph0;
                    proj_parm.B = sqrt(1. + par.es * proj_parm.B * proj_parm.B / par.one_es);
                    proj_parm.A = proj_parm.B * par.k0 * com / con;
                    D = proj_parm.B * com / (cosph0 * sqrt(con));
                    if ((F = D * D - 1.) <= 0.)
                        F = 0.;
                    else {
                        F = sqrt(F);
                        if (par.phi0 < 0.)
                            F = -F;
                    }
                    proj_parm.E = F += D;
                    proj_parm.E *= math::pow(pj_tsfn(par.phi0, sinph0, par.e), proj_parm.B);
                } else {
                    proj_parm.B = 1. / com;
                    proj_parm.A = par.k0;
                    proj_parm.E = D = F = 1.;
                }
                if (alp || gam) {
                    if (alp) {
                        gamma0 = aasin(sin(alpha_c) / D);
                        if (!gam)
                            gamma = alpha_c;
                    } else
                        alpha_c = aasin(D*sin(gamma0 = gamma));
                    par.lam0 = lamc - aasin(.5 * (F - 1. / F) *
                       tan(gamma0)) / proj_parm.B;
                } else {
                    H = math::pow(pj_tsfn(phi1, sin(phi1), par.e), proj_parm.B);
                    L = math::pow(pj_tsfn(phi2, sin(phi2), par.e), proj_parm.B);
                    F = proj_parm.E / H;
                    p = (L - H) / (L + H);
                    J = proj_parm.E * proj_parm.E;
                    J = (J - L * H) / (J + L * H);
                    if ((con = lam1 - lam2) < -pi)
                        lam2 -= two_pi;
                    else if (con > pi)
                        lam2 += two_pi;
                    par.lam0 = adjlon(.5 * (lam1 + lam2) - atan(
                       J * tan(.5 * proj_parm.B * (lam1 - lam2)) / p) / proj_parm.B);
                    gamma0 = atan(2. * sin(proj_parm.B * adjlon(lam1 - par.lam0)) /
                       (F - 1. / F));
                    gamma = alpha_c = aasin(D * sin(gamma0));
                }
                proj_parm.singam = sin(gamma0);
                proj_parm.cosgam = cos(gamma0);
                proj_parm.sinrot = sin(gamma);
                proj_parm.cosrot = cos(gamma);
                proj_parm.BrA = 1. / (proj_parm.ArB = proj_parm.A * (proj_parm.rB = 1. / proj_parm.B));
                proj_parm.AB = proj_parm.A * proj_parm.B;
                if (no_off)
                    proj_parm.u_0 = 0;
                else {
                    proj_parm.u_0 = fabs(proj_parm.ArB * atan(sqrt(D * D - 1.) / cos(alpha_c)));
                    if (par.phi0 < 0.)
                        proj_parm.u_0 = - proj_parm.u_0;
                }
                F = 0.5 * gamma0;
                proj_parm.v_pole_n = proj_parm.ArB * log(tan(fourth_pi - F));
                proj_parm.v_pole_s = proj_parm.ArB * log(tan(fourth_pi + F));
            }

    }} // namespace detail::omerc
    #endif // doxygen

    /*!
        \brief Oblique Mercator projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - no_rot: No rotation
         - alpha: Alpha (degrees)
         - gamma: Gamma (degrees)
         - no_off: Only for compatibility with libproj, proj4 (string)
         - lonc: Longitude (only used if alpha (or gamma) is specified) (degrees)
         - lon_1 (degrees)
         - lat_1: Latitude of first standard parallel (degrees)
         - lon_2 (degrees)
         - lat_2: Latitude of second standard parallel (degrees)
         - no_uoff (string)
        \par Example
        \image html ex_omerc.gif
    */
    template <typename T, typename Parameters>
    struct omerc_ellipsoid : public detail::omerc::base_omerc_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline omerc_ellipsoid(Params const& params, Parameters & par)
        {
            detail::omerc::setup_omerc(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_omerc, omerc_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(omerc_entry, omerc_ellipsoid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(omerc_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(omerc, omerc_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_OMERC_HPP

