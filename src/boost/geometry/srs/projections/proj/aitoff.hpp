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

// Purpose:  Implementation of the aitoff (Aitoff) and wintri (Winkel Tripel)
//           projections.
// Author:   Gerald Evenden (1995)
//           Drazen Tutic, Lovro Gradiser (2015) - add inverse
//           Thomas Knudsen (2016) - revise/add regression tests
// Copyright (c) 1995, Gerald Evenden

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_AITOFF_HPP
#define BOOST_GEOMETRY_PROJECTIONS_AITOFF_HPP

#include <boost/core/ignore_unused.hpp>

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
    namespace detail { namespace aitoff
    {
            enum mode_type {
                mode_aitoff = 0,
                mode_winkel_tripel = 1
            };

            template <typename T>
            struct par_aitoff
            {
                T    cosphi1;
                mode_type mode;
            };

            template <typename T, typename Parameters>
            struct base_aitoff_spheroid
            {
                par_aitoff<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T c, d;

                    if((d = acos(cos(lp_lat) * cos(c = 0.5 * lp_lon)))) {/* basic Aitoff */
                        xy_x = 2. * d * cos(lp_lat) * sin(c) * (xy_y = 1. / sin(d));
                        xy_y *= d * sin(lp_lat);
                    } else
                        xy_x = xy_y = 0.;
                    if (this->m_proj_parm.mode == mode_winkel_tripel) { /* Winkel Tripel */
                        xy_x = (xy_x + lp_lon * this->m_proj_parm.cosphi1) * 0.5;
                        xy_y = (xy_y + lp_lat) * 0.5;
                    }
                }
                /***********************************************************************************
                *
                * Inverse functions added by Drazen Tutic and Lovro Gradiser based on paper:
                *
                * I.Özbug Biklirici and Cengizhan Ipbüker. A General Algorithm for the Inverse
                * Transformation of Map Projections Using Jacobian Matrices. In Proceedings of the
                * Third International Symposium Mathematical & Computational Applications,
                * pages 175{182, Turkey, September 2002.
                *
                * Expected accuracy is defined by epsilon = 1e-12. Should be appropriate for
                * most applications of Aitoff and Winkel Tripel projections.
                *
                * Longitudes of 180W and 180E can be mixed in solution obtained.
                *
                * Inverse for Aitoff projection in poles is undefined, longitude value of 0 is assumed.
                *
                * Contact : dtutic@geof.hr
                * Date: 2015-02-16
                *
                ************************************************************************************/

                // INVERSE(s_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T pi = detail::pi<T>();
                    static const T two_pi = detail::two_pi<T>();
                    static const T epsilon = 1e-12;

                    int iter, max_iter = 10, round = 0, max_round = 20;
                    T D, C, f1, f2, f1p, f1l, f2p, f2l, dp, dl, sl, sp, cp, cl, x, y;

                    if ((fabs(xy_x) < epsilon) && (fabs(xy_y) < epsilon )) {
                        lp_lat = 0.; lp_lon = 0.;
                        return;
                    }

                    /* intial values for Newton-Raphson method */
                    lp_lat = xy_y; lp_lon = xy_x;
                    do {
                        iter = 0;
                        do {
                            sl = sin(lp_lon * 0.5); cl = cos(lp_lon * 0.5);
                            sp = sin(lp_lat); cp = cos(lp_lat);
                            D = cp * cl;
                            C = 1. - D * D;
                            D = acos(D) / math::pow(C, T(1.5));
                            f1 = 2. * D * C * cp * sl;
                            f2 = D * C * sp;
                            f1p = 2.* (sl * cl * sp * cp / C - D * sp * sl);
                            f1l = cp * cp * sl * sl / C + D * cp * cl * sp * sp;
                            f2p = sp * sp * cl / C + D * sl * sl * cp;
                            f2l = 0.5 * (sp * cp * sl / C - D * sp * cp * cp * sl * cl);
                            if (this->m_proj_parm.mode == mode_winkel_tripel) { /* Winkel Tripel */
                                f1 = 0.5 * (f1 + lp_lon * this->m_proj_parm.cosphi1);
                                f2 = 0.5 * (f2 + lp_lat);
                                f1p *= 0.5;
                                f1l = 0.5 * (f1l + this->m_proj_parm.cosphi1);
                                f2p = 0.5 * (f2p + 1.);
                                f2l *= 0.5;
                            }
                            f1 -= xy_x; f2 -= xy_y;
                            dl = (f2 * f1p - f1 * f2p) / (dp = f1p * f2l - f2p * f1l);
                            dp = (f1 * f2l - f2 * f1l) / dp;
                            dl = fmod(dl, pi); /* set to interval [-M_PI, M_PI] */
                            lp_lat -= dp;    lp_lon -= dl;
                        } while ((fabs(dp) > epsilon || fabs(dl) > epsilon) && (iter++ < max_iter));
                        if (lp_lat > two_pi) lp_lat -= 2.*(lp_lat-two_pi); /* correct if symmetrical solution for Aitoff */
                        if (lp_lat < -two_pi) lp_lat -= 2.*(lp_lat+two_pi); /* correct if symmetrical solution for Aitoff */
                        if ((fabs(fabs(lp_lat) - two_pi) < epsilon) && (!this->m_proj_parm.mode)) lp_lon = 0.; /* if pole in Aitoff, return longitude of 0 */

                        /* calculate x,y coordinates with solution obtained */
                        if((D = acos(cos(lp_lat) * cos(C = 0.5 * lp_lon))) != 0.0) {/* Aitoff */
                            x = 2. * D * cos(lp_lat) * sin(C) * (y = 1. / sin(D));
                            y *= D * sin(lp_lat);
                        } else
                            x = y = 0.;
                        if (this->m_proj_parm.mode == mode_winkel_tripel) { /* Winkel Tripel */
                            x = (x + lp_lon * this->m_proj_parm.cosphi1) * 0.5;
                            y = (y + lp_lat) * 0.5;
                        }
                    /* if too far from given values of x,y, repeat with better approximation of phi,lam */
                    } while (((fabs(xy_x-x) > epsilon) || (fabs(xy_y-y) > epsilon)) && (round++ < max_round));

                    if (iter == max_iter && round == max_round)
                    {
                        BOOST_THROW_EXCEPTION( projection_exception(error_non_convergent) );
                        //fprintf(stderr, "Warning: Accuracy of 1e-12 not reached. Last increments: dlat=%e and dlon=%e\n", dp, dl);
                    }
                }

                static inline std::string get_name()
                {
                    return "aitoff_spheroid";
                }

            };

            template <typename Parameters>
            inline void setup(Parameters& par)
            {
                par.es = 0.;
            }


            // Aitoff
            template <typename Parameters, typename T>
            inline void setup_aitoff(Parameters& par, par_aitoff<T>& proj_parm)
            {
                proj_parm.mode = mode_aitoff;
                setup(par);
            }

            // Winkel Tripel
            template <typename Params, typename Parameters, typename T>
            inline void setup_wintri(Params& params, Parameters& par, par_aitoff<T>& proj_parm)
            {
                static const T two_div_pi = detail::two_div_pi<T>();

                T phi1;

                proj_parm.mode = mode_winkel_tripel;
                if (pj_param_r<srs::spar::lat_1>(params, "lat_1", srs::dpar::lat_1, phi1)) {
                    if ((proj_parm.cosphi1 = cos(phi1)) == 0.)
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_larger_than_90) );
                } else /* 50d28' or phi1=acos(2/pi) */
                    proj_parm.cosphi1 = two_div_pi;
                setup(par);
            }

    }} // namespace detail::aitoff
    #endif // doxygen

    /*!
        \brief Aitoff projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Example
        \image html ex_aitoff.gif
    */
    template <typename T, typename Parameters>
    struct aitoff_spheroid : public detail::aitoff::base_aitoff_spheroid<T, Parameters>
    {
        template <typename Params>
        inline aitoff_spheroid(Params const& , Parameters & par)
        {
            detail::aitoff::setup_aitoff(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Winkel Tripel projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - lat_1: Latitude of first standard parallel (degrees)
        \par Example
        \image html ex_wintri.gif
    */
    template <typename T, typename Parameters>
    struct wintri_spheroid : public detail::aitoff::base_aitoff_spheroid<T, Parameters>
    {
        template <typename Params>
        inline wintri_spheroid(Params const& params, Parameters & par)
        {
            detail::aitoff::setup_wintri(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_aitoff, aitoff_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_wintri, wintri_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(aitoff_entry, aitoff_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(wintri_entry, wintri_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(aitoff_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(aitoff, aitoff_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(wintri, wintri_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_AITOFF_HPP

