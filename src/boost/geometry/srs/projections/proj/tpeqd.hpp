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

#ifndef BOOST_GEOMETRY_PROJECTIONS_TPEQD_HPP
#define BOOST_GEOMETRY_PROJECTIONS_TPEQD_HPP

#include <boost/geometry/util/math.hpp>
#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/srs/projections/impl/aasincos.hpp>
#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace tpeqd
    {
            template <typename T>
            struct par_tpeqd
            {
                T cp1, sp1, cp2, sp2, ccs, cs, sc, r2z0, z02, dlam2;
                T hz0, thz0, rhshz0, ca, sa, lp, lamc;
            };

            template <typename T, typename Parameters>
            struct base_tpeqd_spheroid
            {
                par_tpeqd<T> m_proj_parm;

                // FORWARD(s_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T t, z1, z2, dl1, dl2, sp, cp;

                    sp = sin(lp_lat);
                    cp = cos(lp_lat);
                    z1 = aacos(this->m_proj_parm.sp1 * sp + this->m_proj_parm.cp1 * cp * cos(dl1 = lp_lon + this->m_proj_parm.dlam2));
                    z2 = aacos(this->m_proj_parm.sp2 * sp + this->m_proj_parm.cp2 * cp * cos(dl2 = lp_lon - this->m_proj_parm.dlam2));
                    z1 *= z1;
                    z2 *= z2;

                    xy_x = this->m_proj_parm.r2z0 * (t = z1 - z2);
                    t = this->m_proj_parm.z02 - t;
                    xy_y = this->m_proj_parm.r2z0 * asqrt(4. * this->m_proj_parm.z02 * z2 - t * t);
                    if ((this->m_proj_parm.ccs * sp - cp * (this->m_proj_parm.cs * sin(dl1) - this->m_proj_parm.sc * sin(dl2))) < 0.)
                        xy_y = -xy_y;
                }

                // INVERSE(s_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T cz1, cz2, s, d, cp, sp;

                    cz1 = cos(boost::math::hypot(xy_y, xy_x + this->m_proj_parm.hz0));
                    cz2 = cos(boost::math::hypot(xy_y, xy_x - this->m_proj_parm.hz0));
                    s = cz1 + cz2;
                    d = cz1 - cz2;
                    lp_lon = - atan2(d, (s * this->m_proj_parm.thz0));
                    lp_lat = aacos(boost::math::hypot(this->m_proj_parm.thz0 * s, d) * this->m_proj_parm.rhshz0);
                    if ( xy_y < 0. )
                        lp_lat = - lp_lat;
                    /* lam--phi now in system relative to P1--P2 base equator */
                    sp = sin(lp_lat);
                    cp = cos(lp_lat);
                    lp_lat = aasin(this->m_proj_parm.sa * sp + this->m_proj_parm.ca * cp * (s = cos(lp_lon -= this->m_proj_parm.lp)));
                    lp_lon = atan2(cp * sin(lp_lon), this->m_proj_parm.sa * cp * s - this->m_proj_parm.ca * sp) + this->m_proj_parm.lamc;
                }

                static inline std::string get_name()
                {
                    return "tpeqd_spheroid";
                }

            };

            // Two Point Equidistant
            template <typename Params, typename Parameters, typename T>
            inline void setup_tpeqd(Params const& params, Parameters& par, par_tpeqd<T>& proj_parm)
            {
                T lam_1, lam_2, phi_1, phi_2, A12, pp;

                /* get control point locations */
                phi_1 = pj_get_param_r<T, srs::spar::lat_1>(params, "lat_1", srs::dpar::lat_1);
                lam_1 = pj_get_param_r<T, srs::spar::lon_1>(params, "lon_1", srs::dpar::lon_1);
                phi_2 = pj_get_param_r<T, srs::spar::lat_2>(params, "lat_2", srs::dpar::lat_2);
                lam_2 = pj_get_param_r<T, srs::spar::lon_2>(params, "lon_2", srs::dpar::lon_2);

                if (phi_1 == phi_2 && lam_1 == lam_2)
                    BOOST_THROW_EXCEPTION( projection_exception(error_control_point_no_dist) );

                par.lam0 = adjlon(0.5 * (lam_1 + lam_2));
                proj_parm.dlam2 = adjlon(lam_2 - lam_1);

                proj_parm.cp1 = cos(phi_1);
                proj_parm.cp2 = cos(phi_2);
                proj_parm.sp1 = sin(phi_1);
                proj_parm.sp2 = sin(phi_2);
                proj_parm.cs = proj_parm.cp1 * proj_parm.sp2;
                proj_parm.sc = proj_parm.sp1 * proj_parm.cp2;
                proj_parm.ccs = proj_parm.cp1 * proj_parm.cp2 * sin(proj_parm.dlam2);
                proj_parm.z02 = aacos(proj_parm.sp1 * proj_parm.sp2 + proj_parm.cp1 * proj_parm.cp2 * cos(proj_parm.dlam2));
                proj_parm.hz0 = .5 * proj_parm.z02;
                A12 = atan2(proj_parm.cp2 * sin(proj_parm.dlam2),
                    proj_parm.cp1 * proj_parm.sp2 - proj_parm.sp1 * proj_parm.cp2 * cos(proj_parm.dlam2));
                proj_parm.ca = cos(pp = aasin(proj_parm.cp1 * sin(A12)));
                proj_parm.sa = sin(pp);
                proj_parm.lp = adjlon(atan2(proj_parm.cp1 * cos(A12), proj_parm.sp1) - proj_parm.hz0);
                proj_parm.dlam2 *= .5;
                proj_parm.lamc = geometry::math::half_pi<T>() - atan2(sin(A12) * proj_parm.sp1, cos(A12)) - proj_parm.dlam2;
                proj_parm.thz0 = tan(proj_parm.hz0);
                proj_parm.rhshz0 = .5 / sin(proj_parm.hz0);
                proj_parm.r2z0 = 0.5 / proj_parm.z02;
                proj_parm.z02 *= proj_parm.z02;

                par.es = 0.;
            }

    }} // namespace detail::tpeqd
    #endif // doxygen

    /*!
        \brief Two Point Equidistant projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - lat_1: Latitude of first standard parallel (degrees)
         - lon_1 (degrees)
         - lat_2: Latitude of second standard parallel (degrees)
         - lon_2 (degrees)
        \par Example
        \image html ex_tpeqd.gif
    */
    template <typename T, typename Parameters>
    struct tpeqd_spheroid : public detail::tpeqd::base_tpeqd_spheroid<T, Parameters>
    {
        template <typename Params>
        inline tpeqd_spheroid(Params const& params, Parameters & par)
        {
            detail::tpeqd::setup_tpeqd(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_tpeqd, tpeqd_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(tpeqd_entry, tpeqd_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(tpeqd_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(tpeqd, tpeqd_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_TPEQD_HPP

