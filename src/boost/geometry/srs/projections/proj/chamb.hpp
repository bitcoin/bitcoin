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

#ifndef BOOST_GEOMETRY_PROJECTIONS_CHAMB_HPP
#define BOOST_GEOMETRY_PROJECTIONS_CHAMB_HPP

#include <cstdio>

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
    namespace detail { namespace chamb
    {

            //static const double third = 0.333333333333333333;
            static const double tolerance = 1e-9;

            // specific for 'chamb'
            template <typename T>
            struct vect_ra { T r, Az; };
            template <typename T>
            struct point_xy { T x, y; };

            template <typename T>
            struct par_chamb
            {
                struct { /* control point data */
                    T phi, lam;
                    T cosphi, sinphi;
                    vect_ra<T> v;
                    point_xy<T> p;
                    T Az;
                } c[3];
                point_xy<T> p;
                T beta_0, beta_1, beta_2;
            };

            /* distance and azimuth from point 1 to point 2 */
            template <typename T>
            inline vect_ra<T> vect(T const& dphi, T const& c1, T const& s1, T const& c2, T const& s2, T const& dlam)
            {
                vect_ra<T> v;
                T cdl, dp, dl;

                cdl = cos(dlam);
                if (fabs(dphi) > 1. || fabs(dlam) > 1.)
                    v.r = aacos(s1 * s2 + c1 * c2 * cdl);
                else { /* more accurate for smaller distances */
                    dp = sin(.5 * dphi);
                    dl = sin(.5 * dlam);
                    v.r = 2. * aasin(sqrt(dp * dp + c1 * c2 * dl * dl));
                }
                if (fabs(v.r) > tolerance)
                    v.Az = atan2(c2 * sin(dlam), c1 * s2 - s1 * c2 * cdl);
                else
                    v.r = v.Az = 0.;
                return v;
            }

            /* law of cosines */
            template <typename T>
            inline T lc(T const& b, T const& c, T const& a)
            {
                return aacos(.5 * (b * b + c * c - a * a) / (b * c));
            }

            template <typename T, typename Parameters>
            struct base_chamb_spheroid
            {
                par_chamb<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T third = detail::third<T>();

                    T sinphi, cosphi, a;
                    vect_ra<T> v[3];
                    int i, j;

                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    for (i = 0; i < 3; ++i) { /* dist/azimiths from control */
                        v[i] = vect(lp_lat - this->m_proj_parm.c[i].phi, this->m_proj_parm.c[i].cosphi, this->m_proj_parm.c[i].sinphi,
                            cosphi, sinphi, lp_lon - this->m_proj_parm.c[i].lam);
                        if (v[i].r == 0.0)
                            break;
                        v[i].Az = adjlon(v[i].Az - this->m_proj_parm.c[i].v.Az);
                    }
                    if (i < 3) /* current point at control point */
                        { xy_x = this->m_proj_parm.c[i].p.x; xy_y = this->m_proj_parm.c[i].p.y; }
                    else { /* point mean of intersepts */
                        { xy_x = this->m_proj_parm.p.x; xy_y = this->m_proj_parm.p.y; }
                        for (i = 0; i < 3; ++i) {
                            j = i == 2 ? 0 : i + 1;
                            a = lc(this->m_proj_parm.c[i].v.r, v[i].r, v[j].r);
                            if (v[i].Az < 0.)
                                a = -a;
                            if (! i) { /* coord comp unique to each arc */
                                xy_x += v[i].r * cos(a);
                                xy_y -= v[i].r * sin(a);
                            } else if (i == 1) {
                                a = this->m_proj_parm.beta_1 - a;
                                xy_x -= v[i].r * cos(a);
                                xy_y -= v[i].r * sin(a);
                            } else {
                                a = this->m_proj_parm.beta_2 - a;
                                xy_x += v[i].r * cos(a);
                                xy_y += v[i].r * sin(a);
                            }
                        }
                        xy_x *= third; /* mean of arc intercepts */
                        xy_y *= third;
                    }
                }

                static inline std::string get_name()
                {
                    return "chamb_spheroid";
                }

            };

            template <typename T>
            inline T chamb_init_lat(srs::detail::proj4_parameters const& params, int i)
            {
                static const std::string lat[3] = {"lat_1", "lat_2", "lat_3"};
                return _pj_get_param_r<T>(params, lat[i]);
            }
            template <typename T>
            inline T chamb_init_lat(srs::dpar::parameters<T> const& params, int i)
            {
                static const srs::dpar::name_r lat[3] = {srs::dpar::lat_1, srs::dpar::lat_2, srs::dpar::lat_3};
                return _pj_get_param_r<T>(params, lat[i]);
            }

            template <typename T>
            inline T chamb_init_lon(srs::detail::proj4_parameters const& params, int i)
            {
                static const std::string lon[3] = {"lon_1", "lon_2", "lon_3"};
                return _pj_get_param_r<T>(params, lon[i]);
            }
            template <typename T>
            inline T chamb_init_lon(srs::dpar::parameters<T> const& params, int i)
            {
                static const srs::dpar::name_r lon[3] = {srs::dpar::lon_1, srs::dpar::lon_2, srs::dpar::lon_3};
                return _pj_get_param_r<T>(params, lon[i]);
            }

            // Chamberlin Trimetric
            template <typename Params, typename Parameters, typename T>
            inline void setup_chamb(Params const& params, Parameters& par, par_chamb<T>& proj_parm)
            {
                static const T pi = detail::pi<T>();

                int i, j;

                for (i = 0; i < 3; ++i) { /* get control point locations */
                    proj_parm.c[i].phi = chamb_init_lat<T>(params, i);
                    proj_parm.c[i].lam = chamb_init_lon<T>(params, i);
                    proj_parm.c[i].lam = adjlon(proj_parm.c[i].lam - par.lam0);
                    proj_parm.c[i].cosphi = cos(proj_parm.c[i].phi);
                    proj_parm.c[i].sinphi = sin(proj_parm.c[i].phi);
                }
                for (i = 0; i < 3; ++i) { /* inter ctl pt. distances and azimuths */
                    j = i == 2 ? 0 : i + 1;
                    proj_parm.c[i].v = vect(proj_parm.c[j].phi - proj_parm.c[i].phi, proj_parm.c[i].cosphi, proj_parm.c[i].sinphi,
                        proj_parm.c[j].cosphi, proj_parm.c[j].sinphi, proj_parm.c[j].lam - proj_parm.c[i].lam);
                    if (proj_parm.c[i].v.r == 0.0)
                        BOOST_THROW_EXCEPTION( projection_exception(error_control_point_no_dist) );
                    /* co-linearity problem ignored for now */
                }
                proj_parm.beta_0 = lc(proj_parm.c[0].v.r, proj_parm.c[2].v.r, proj_parm.c[1].v.r);
                proj_parm.beta_1 = lc(proj_parm.c[0].v.r, proj_parm.c[1].v.r, proj_parm.c[2].v.r);
                proj_parm.beta_2 = pi - proj_parm.beta_0;
                proj_parm.p.y = 2. * (proj_parm.c[0].p.y = proj_parm.c[1].p.y = proj_parm.c[2].v.r * sin(proj_parm.beta_0));
                proj_parm.c[2].p.y = 0.;
                proj_parm.c[0].p.x = - (proj_parm.c[1].p.x = 0.5 * proj_parm.c[0].v.r);
                proj_parm.p.x = proj_parm.c[2].p.x = proj_parm.c[0].p.x + proj_parm.c[2].v.r * cos(proj_parm.beta_0);

                par.es = 0.;
            }

    }} // namespace detail::chamb
    #endif // doxygen

    /*!
        \brief Chamberlin Trimetric projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Projection parameters
         - lat_1: Latitude of control point 1 (degrees)
         - lon_1: Longitude of control point 1 (degrees)
         - lat_2: Latitude of control point 2 (degrees)
         - lon_2: Longitude of control point 2 (degrees)
         - lat_3: Latitude of control point 3 (degrees)
         - lon_3: Longitude of control point 3 (degrees)
        \par Example
        \image html ex_chamb.gif
    */
    template <typename T, typename Parameters>
    struct chamb_spheroid : public detail::chamb::base_chamb_spheroid<T, Parameters>
    {
        template <typename Params>
        inline chamb_spheroid(Params const& params, Parameters & par)
        {
            detail::chamb::setup_chamb(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_chamb, chamb_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(chamb_entry, chamb_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(chamb_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(chamb, chamb_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_CHAMB_HPP

