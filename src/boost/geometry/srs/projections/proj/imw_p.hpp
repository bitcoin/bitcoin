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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMW_P_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMW_P_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_mlfn.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace imw_p
    {

            static const double tolerance = 1e-10;
            static const double epsilon = 1e-10;

            template <typename T>
            struct point_xy { T x, y; }; // specific for IMW_P

            enum mode_type {
                none_is_zero  =  0, /* phi_1 and phi_2 != 0 */
                phi_1_is_zero =  1, /* phi_1 = 0 */
                phi_2_is_zero = -1  /* phi_2 = 0 */
            };

            template <typename T>
            struct par_imw_p
            {
                T    P, Pp, Q, Qp, R_1, R_2, sphi_1, sphi_2, C2;
                T    phi_1, phi_2, lam_1;
                detail::en<T> en;
                mode_type mode;
            };

            template <typename Params, typename T>
            inline int phi12(Params const& params,
                             par_imw_p<T> & proj_parm, T *del, T *sig)
            {
                int err = 0;

                if (!pj_param_r<srs::spar::lat_1>(params, "lat_1", srs::dpar::lat_1, proj_parm.phi_1) ||
                    !pj_param_r<srs::spar::lat_2>(params, "lat_2", srs::dpar::lat_2, proj_parm.phi_2)) {
                    err = -41;
                } else {
                    //proj_parm.phi_1 = pj_get_param_r(par.params, "lat_1"); // set above
                    //proj_parm.phi_2 = pj_get_param_r(par.params, "lat_2"); // set above
                    *del = 0.5 * (proj_parm.phi_2 - proj_parm.phi_1);
                    *sig = 0.5 * (proj_parm.phi_2 + proj_parm.phi_1);
                    err = (fabs(*del) < epsilon || fabs(*sig) < epsilon) ? -42 : 0;
                }
                return err;
            }
            template <typename Parameters, typename T>
            inline point_xy<T> loc_for(T const& lp_lam, T const& lp_phi,
                                       Parameters const& par,
                                       par_imw_p<T> const& proj_parm,
                                       T *yc)
            {
                point_xy<T> xy;

                if (lp_phi == 0.0) {
                    xy.x = lp_lam;
                    xy.y = 0.;
                } else {
                    T xa, ya, xb, yb, xc, D, B, m, sp, t, R, C;

                    sp = sin(lp_phi);
                    m = pj_mlfn(lp_phi, sp, cos(lp_phi), proj_parm.en);
                    xa = proj_parm.Pp + proj_parm.Qp * m;
                    ya = proj_parm.P + proj_parm.Q * m;
                    R = 1. / (tan(lp_phi) * sqrt(1. - par.es * sp * sp));
                    C = sqrt(R * R - xa * xa);
                    if (lp_phi < 0.) C = - C;
                    C += ya - R;
                    if (proj_parm.mode == phi_2_is_zero) {
                        xb = lp_lam;
                        yb = proj_parm.C2;
                    } else {
                        t = lp_lam * proj_parm.sphi_2;
                        xb = proj_parm.R_2 * sin(t);
                        yb = proj_parm.C2 + proj_parm.R_2 * (1. - cos(t));
                    }
                    if (proj_parm.mode == phi_1_is_zero) {
                        xc = lp_lam;
                        *yc = 0.;
                    } else {
                        t = lp_lam * proj_parm.sphi_1;
                        xc = proj_parm.R_1 * sin(t);
                        *yc = proj_parm.R_1 * (1. - cos(t));
                    }
                    D = (xb - xc)/(yb - *yc);
                    B = xc + D * (C + R - *yc);
                    xy.x = D * sqrt(R * R * (1 + D * D) - B * B);
                    if (lp_phi > 0)
                        xy.x = - xy.x;
                    xy.x = (B + xy.x) / (1. + D * D);
                    xy.y = sqrt(R * R - xy.x * xy.x);
                    if (lp_phi > 0)
                        xy.y = - xy.y;
                    xy.y += C + R;
                }
                return (xy);
            }
            template <typename Parameters, typename T>
            inline void xy(Parameters const& par, par_imw_p<T> const& proj_parm,
                           T const& phi,
                           T *x, T *y, T *sp, T *R)
            {
                T F;

                *sp = sin(phi);
                *R = 1./(tan(phi) * sqrt(1. - par.es * *sp * *sp ));
                F = proj_parm.lam_1 * *sp;
                *y = *R * (1 - cos(F));
                *x = *R * sin(F);
            }

            template <typename T, typename Parameters>
            struct base_imw_p_ellipsoid
            {
                par_imw_p<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T yc = 0;
                    point_xy<T> xy = loc_for(lp_lon, lp_lat, par, m_proj_parm, &yc);
                    xy_x = xy.x; xy_y = xy.y;
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    point_xy<T> t;
                    T yc = 0.0;
                    int i = 0;
                    const int n_max_iter = 1000; /* Arbitrarily choosen number... */

                    lp_lat = this->m_proj_parm.phi_2;
                    lp_lon = xy_x / cos(lp_lat);
                    do {
                        t = loc_for(lp_lon, lp_lat, par, m_proj_parm, &yc);
                        lp_lat = ((lp_lat - this->m_proj_parm.phi_1) * (xy_y - yc) / (t.y - yc)) + this->m_proj_parm.phi_1;
                        lp_lon = lp_lon * xy_x / t.x;
                        i++;
                    } while (i < n_max_iter &&
                             (fabs(t.x - xy_x) > tolerance || fabs(t.y - xy_y) > tolerance));

                    if( i == n_max_iter )
                    {
                        lp_lon = lp_lat = HUGE_VAL;
                    }
                }

                static inline std::string get_name()
                {
                    return "imw_p_ellipsoid";
                }

            };

            // International Map of the World Polyconic
            template <typename Params, typename Parameters, typename T>
            inline void setup_imw_p(Params const& params, Parameters const& par, par_imw_p<T>& proj_parm)
            {
                T del, sig, s, t, x1, x2, T2, y1, m1, m2, y2;
                int err;

                proj_parm.en = pj_enfn<T>(par.es);
                if( (err = phi12(params, proj_parm, &del, &sig)) != 0)
                    BOOST_THROW_EXCEPTION( projection_exception(err) );
                if (proj_parm.phi_2 < proj_parm.phi_1) { /* make sure proj_parm.phi_1 most southerly */
                    del = proj_parm.phi_1;
                    proj_parm.phi_1 = proj_parm.phi_2;
                    proj_parm.phi_2 = del;
                }
                if (pj_param_r<srs::spar::lon_1>(params, "lon_1", srs::dpar::lon_1, proj_parm.lam_1)) {
                    /* empty */
                } else { /* use predefined based upon latitude */
                    sig = fabs(sig * geometry::math::r2d<T>());
                    if (sig <= 60)      sig = 2.;
                    else if (sig <= 76) sig = 4.;
                    else                sig = 8.;
                    proj_parm.lam_1 = sig * geometry::math::d2r<T>();
                }
                proj_parm.mode = none_is_zero;
                if (proj_parm.phi_1 != 0.0)
                    xy(par, proj_parm, proj_parm.phi_1, &x1, &y1, &proj_parm.sphi_1, &proj_parm.R_1);
                else {
                    proj_parm.mode = phi_1_is_zero;
                    y1 = 0.;
                    x1 = proj_parm.lam_1;
                }
                if (proj_parm.phi_2 != 0.0)
                    xy(par, proj_parm, proj_parm.phi_2, &x2, &T2, &proj_parm.sphi_2, &proj_parm.R_2);
                else {
                    proj_parm.mode = phi_2_is_zero;
                    T2 = 0.;
                    x2 = proj_parm.lam_1;
                }
                m1 = pj_mlfn(proj_parm.phi_1, proj_parm.sphi_1, cos(proj_parm.phi_1), proj_parm.en);
                m2 = pj_mlfn(proj_parm.phi_2, proj_parm.sphi_2, cos(proj_parm.phi_2), proj_parm.en);
                t = m2 - m1;
                s = x2 - x1;
                y2 = sqrt(t * t - s * s) + y1;
                proj_parm.C2 = y2 - T2;
                t = 1. / t;
                proj_parm.P = (m2 * y1 - m1 * y2) * t;
                proj_parm.Q = (y2 - y1) * t;
                proj_parm.Pp = (m2 * x1 - m1 * x2) * t;
                proj_parm.Qp = (x2 - x1) * t;
            }

    }} // namespace detail::imw_p
    #endif // doxygen

    /*!
        \brief International Map of the World Polyconic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Mod. Polyconic
         - Ellipsoid
        \par Projection parameters
         - lat_1: Latitude of first standard parallel
         - lat_2: Latitude of second standard parallel
         - lon_1 (degrees)
        \par Example
        \image html ex_imw_p.gif
    */
    template <typename T, typename Parameters>
    struct imw_p_ellipsoid : public detail::imw_p::base_imw_p_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline imw_p_ellipsoid(Params const& params, Parameters const& par)
        {
            detail::imw_p::setup_imw_p(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_imw_p, imw_p_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(imw_p_entry, imw_p_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(imw_p_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(imw_p, imw_p_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_IMW_P_HPP

