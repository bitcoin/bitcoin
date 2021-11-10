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

#ifndef BOOST_GEOMETRY_PROJECTIONS_LSAT_HPP
#define BOOST_GEOMETRY_PROJECTIONS_LSAT_HPP

#include <boost/geometry/srs/projections/impl/aasincos.hpp>
#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace lsat
    {
            static const double tolerance = 1e-7;

            template <typename T>
            struct par_lsat
            {
                T a2, a4, b, c1, c3;
                T q, t, u, w, p22, sa, ca, xj, rlm, rlm2;
            };

            /* based upon Snyder and Linck, USGS-NMD */
            template <typename T>
            inline void
            seraz0(T lam, T const& mult, par_lsat<T>& proj_parm)
            {
                T sdsq, h, s, fc, sd, sq, d__1 = 0;

                lam *= geometry::math::d2r<T>();
                sd = sin(lam);
                sdsq = sd * sd;
                s = proj_parm.p22 * proj_parm.sa * cos(lam) * sqrt((1. + proj_parm.t * sdsq)
                    / ((1. + proj_parm.w * sdsq) * (1. + proj_parm.q * sdsq)));

                d__1 = 1. + proj_parm.q * sdsq;
                h = sqrt((1. + proj_parm.q * sdsq) / (1. + proj_parm.w * sdsq)) * ((1. + proj_parm.w * sdsq)
                    / (d__1 * d__1) - proj_parm.p22 * proj_parm.ca);

                sq = sqrt(proj_parm.xj * proj_parm.xj + s * s);
                fc = mult * (h * proj_parm.xj - s * s) / sq;
                proj_parm.b += fc;
                proj_parm.a2 += fc * cos(lam + lam);
                proj_parm.a4 += fc * cos(lam * 4.);
                fc = mult * s * (h + proj_parm.xj) / sq;
                proj_parm.c1 += fc * cos(lam);
                proj_parm.c3 += fc * cos(lam * 3.);
            }

            template <typename T, typename Parameters>
            struct base_lsat_ellipsoid
            {
                par_lsat<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T fourth_pi = detail::fourth_pi<T>();
                    static const T half_pi = detail::half_pi<T>();
                    static const T one_and_half_pi = detail::one_and_half_pi<T>();
                    static const T two_and_half_pi = detail::two_and_half_pi<T>();

                    int l, nn;
                    T lamt = 0.0, xlam, sdsq, c, d, s, lamdp = 0.0, phidp, lampp, tanph;
                    T lamtp, cl, sd, sp, sav, tanphi;

                    if (lp_lat > half_pi)
                        lp_lat = half_pi;
                    else if (lp_lat < -half_pi)
                        lp_lat = -half_pi;

                    if (lp_lat >= 0. )
                        lampp = half_pi;
                    else
                        lampp = one_and_half_pi;
                    tanphi = tan(lp_lat);
                    for (nn = 0;;) {
                        T fac;
                        sav = lampp;
                        lamtp = lp_lon + this->m_proj_parm.p22 * lampp;
                        cl = cos(lamtp);
                        if (fabs(cl) < tolerance)
                            lamtp -= tolerance;
                        if( cl < 0 )
                            fac = lampp + sin(lampp) * half_pi;
                        else
                            fac = lampp - sin(lampp) * half_pi;
                        for (l = 50; l; --l) {
                            lamt = lp_lon + this->m_proj_parm.p22 * sav;
                            c = cos(lamt);
                            if (fabs(c) < tolerance)
                                lamt -= tolerance;
                            xlam = (par.one_es * tanphi * this->m_proj_parm.sa + sin(lamt) * this->m_proj_parm.ca) / c;
                            lamdp = atan(xlam) + fac;
                            if (fabs(fabs(sav) - fabs(lamdp)) < tolerance)
                                break;
                            sav = lamdp;
                        }
                        if (!l || ++nn >= 3 || (lamdp > this->m_proj_parm.rlm && lamdp < this->m_proj_parm.rlm2))
                            break;
                        if (lamdp <= this->m_proj_parm.rlm)
                            lampp = two_and_half_pi;
                        else if (lamdp >= this->m_proj_parm.rlm2)
                            lampp = half_pi;
                    }
                    if (l) {
                        sp = sin(lp_lat);
                        phidp = aasin((par.one_es * this->m_proj_parm.ca * sp - this->m_proj_parm.sa * cos(lp_lat) *
                            sin(lamt)) / sqrt(1. - par.es * sp * sp));
                        tanph = log(tan(fourth_pi + .5 * phidp));
                        sd = sin(lamdp);
                        sdsq = sd * sd;
                        s = this->m_proj_parm.p22 * this->m_proj_parm.sa * cos(lamdp) * sqrt((1. + this->m_proj_parm.t * sdsq)
                             / ((1. + this->m_proj_parm.w * sdsq) * (1. + this->m_proj_parm.q * sdsq)));
                        d = sqrt(this->m_proj_parm.xj * this->m_proj_parm.xj + s * s);
                        xy_x = this->m_proj_parm.b * lamdp + this->m_proj_parm.a2 * sin(2. * lamdp) + this->m_proj_parm.a4 *
                            sin(lamdp * 4.) - tanph * s / d;
                        xy_y = this->m_proj_parm.c1 * sd + this->m_proj_parm.c3 * sin(lamdp * 3.) + tanph * this->m_proj_parm.xj / d;
                    } else
                        xy_x = xy_y = HUGE_VAL;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T fourth_pi = detail::fourth_pi<T>();
                    static const T half_pi = detail::half_pi<T>();

                    int nn;
                    T lamt, sdsq, s, lamdp, phidp, sppsq, dd, sd, sl, fac, scl, sav, spp;

                    lamdp = xy_x / this->m_proj_parm.b;
                    nn = 50;
                    do {
                        sav = lamdp;
                        sd = sin(lamdp);
                        sdsq = sd * sd;
                        s = this->m_proj_parm.p22 * this->m_proj_parm.sa * cos(lamdp) * sqrt((1. + this->m_proj_parm.t * sdsq)
                             / ((1. + this->m_proj_parm.w * sdsq) * (1. + this->m_proj_parm.q * sdsq)));
                        lamdp = xy_x + xy_y * s / this->m_proj_parm.xj - this->m_proj_parm.a2 * sin(
                            2. * lamdp) - this->m_proj_parm.a4 * sin(lamdp * 4.) - s / this->m_proj_parm.xj * (
                            this->m_proj_parm.c1 * sin(lamdp) + this->m_proj_parm.c3 * sin(lamdp * 3.));
                        lamdp /= this->m_proj_parm.b;
                    } while (fabs(lamdp - sav) >= tolerance && --nn);
                    sl = sin(lamdp);
                    fac = exp(sqrt(1. + s * s / this->m_proj_parm.xj / this->m_proj_parm.xj) * (xy_y -
                        this->m_proj_parm.c1 * sl - this->m_proj_parm.c3 * sin(lamdp * 3.)));
                    phidp = 2. * (atan(fac) - fourth_pi);
                    dd = sl * sl;
                    if (fabs(cos(lamdp)) < tolerance)
                        lamdp -= tolerance;
                    spp = sin(phidp);
                    sppsq = spp * spp;
                    lamt = atan(((1. - sppsq * par.rone_es) * tan(lamdp) *
                        this->m_proj_parm.ca - spp * this->m_proj_parm.sa * sqrt((1. + this->m_proj_parm.q * dd) * (
                        1. - sppsq) - sppsq * this->m_proj_parm.u) / cos(lamdp)) / (1. - sppsq
                        * (1. + this->m_proj_parm.u)));
                    sl = lamt >= 0. ? 1. : -1.;
                    scl = cos(lamdp) >= 0. ? 1. : -1;
                    lamt -= half_pi * (1. - scl) * sl;
                    lp_lon = lamt - this->m_proj_parm.p22 * lamdp;
                    if (fabs(this->m_proj_parm.sa) < tolerance)
                        lp_lat = aasin(spp / sqrt(par.one_es * par.one_es + par.es * sppsq));
                    else
                        lp_lat = atan((tan(lamdp) * cos(lamt) - this->m_proj_parm.ca * sin(lamt)) /
                            (par.one_es * this->m_proj_parm.sa));
                }

                static inline std::string get_name()
                {
                    return "lsat_ellipsoid";
                }

            };

            // Space oblique for LANDSAT
            template <typename Params, typename Parameters, typename T>
            inline void setup_lsat(Params const& params, Parameters& par, par_lsat<T>& proj_parm)
            {
                static T const d2r = geometry::math::d2r<T>();
                static T const pi = detail::pi<T>();
                static T const two_pi = detail::two_pi<T>();

                int land, path;
                T lam, alf, esc, ess;

                land = pj_get_param_i<srs::spar::lsat>(params, "lsat", srs::dpar::lsat);
                if (land <= 0 || land > 5)
                    BOOST_THROW_EXCEPTION( projection_exception(error_lsat_not_in_range) );

                path = pj_get_param_i<srs::spar::path>(params, "path", srs::dpar::path);
                if (path <= 0 || path > (land <= 3 ? 251 : 233))
                    BOOST_THROW_EXCEPTION( projection_exception(error_path_not_in_range) );

                if (land <= 3) {
                    par.lam0 = d2r * 128.87 - two_pi / 251. * path;
                    proj_parm.p22 = 103.2669323;
                    alf = d2r * 99.092;
                } else {
                    par.lam0 = d2r * 129.3 - two_pi / 233. * path;
                    proj_parm.p22 = 98.8841202;
                    alf = d2r * 98.2;
                }
                proj_parm.p22 /= 1440.;
                proj_parm.sa = sin(alf);
                proj_parm.ca = cos(alf);
                if (fabs(proj_parm.ca) < 1e-9)
                    proj_parm.ca = 1e-9;
                esc = par.es * proj_parm.ca * proj_parm.ca;
                ess = par.es * proj_parm.sa * proj_parm.sa;
                proj_parm.w = (1. - esc) * par.rone_es;
                proj_parm.w = proj_parm.w * proj_parm.w - 1.;
                proj_parm.q = ess * par.rone_es;
                proj_parm.t = ess * (2. - par.es) * par.rone_es * par.rone_es;
                proj_parm.u = esc * par.rone_es;
                proj_parm.xj = par.one_es * par.one_es * par.one_es;
                proj_parm.rlm = pi * (1. / 248. + .5161290322580645);
                proj_parm.rlm2 = proj_parm.rlm + two_pi;
                proj_parm.a2 = proj_parm.a4 = proj_parm.b = proj_parm.c1 = proj_parm.c3 = 0.;
                seraz0(0., 1., proj_parm);
                for (lam = 9.; lam <= 81.0001; lam += 18.)
                    seraz0(lam, 4., proj_parm);
                for (lam = 18; lam <= 72.0001; lam += 18.)
                    seraz0(lam, 2., proj_parm);
                seraz0(90., 1., proj_parm);
                proj_parm.a2 /= 30.;
                proj_parm.a4 /= 60.;
                proj_parm.b /= 30.;
                proj_parm.c1 /= 15.;
                proj_parm.c3 /= 45.;
            }

    }} // namespace detail::lsat
    #endif // doxygen

    /*!
        \brief Space oblique for LANDSAT projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Cylindrical
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lsat (integer)
         - path (integer)
        \par Example
        \image html ex_lsat.gif
    */
    template <typename T, typename Parameters>
    struct lsat_ellipsoid : public detail::lsat::base_lsat_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline lsat_ellipsoid(Params const& params, Parameters & par)
        {
            detail::lsat::setup_lsat(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_lsat, lsat_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(lsat_entry, lsat_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(lsat_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(lsat, lsat_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_LSAT_HPP

