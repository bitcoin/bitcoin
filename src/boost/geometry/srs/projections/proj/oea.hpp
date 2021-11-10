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

#ifndef BOOST_GEOMETRY_PROJECTIONS_OEA_HPP
#define BOOST_GEOMETRY_PROJECTIONS_OEA_HPP

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
    namespace detail { namespace oea
    {
            template <typename T>
            struct par_oea
            {
                T    theta;
                T    m, n;
                T    two_r_m, two_r_n, rm, rn, hm, hn;
                T    cp0, sp0;
            };

            template <typename T, typename Parameters>
            struct base_oea_spheroid
            {
                par_oea<T> m_proj_parm;

                // FORWARD(s_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T Az, M, N, cp, sp, cl, shz;

                    cp = cos(lp_lat);
                    sp = sin(lp_lat);
                    cl = cos(lp_lon);
                    Az = aatan2(cp * sin(lp_lon), this->m_proj_parm.cp0 * sp - this->m_proj_parm.sp0 * cp * cl) + this->m_proj_parm.theta;
                    shz = sin(0.5 * aacos(this->m_proj_parm.sp0 * sp + this->m_proj_parm.cp0 * cp * cl));
                    M = aasin(shz * sin(Az));
                    N = aasin(shz * cos(Az) * cos(M) / cos(M * this->m_proj_parm.two_r_m));
                    xy_y = this->m_proj_parm.n * sin(N * this->m_proj_parm.two_r_n);
                    xy_x = this->m_proj_parm.m * sin(M * this->m_proj_parm.two_r_m) * cos(N) / cos(N * this->m_proj_parm.two_r_n);
                }

                // INVERSE(s_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T N, M, xp, yp, z, Az, cz, sz, cAz;

                    N = this->m_proj_parm.hn * aasin(xy_y * this->m_proj_parm.rn);
                    M = this->m_proj_parm.hm * aasin(xy_x * this->m_proj_parm.rm * cos(N * this->m_proj_parm.two_r_n) / cos(N));
                    xp = 2. * sin(M);
                    yp = 2. * sin(N) * cos(M * this->m_proj_parm.two_r_m) / cos(M);
                    cAz = cos(Az = aatan2(xp, yp) - this->m_proj_parm.theta);
                    z = 2. * aasin(0.5 * boost::math::hypot(xp, yp));
                    sz = sin(z);
                    cz = cos(z);
                    lp_lat = aasin(this->m_proj_parm.sp0 * cz + this->m_proj_parm.cp0 * sz * cAz);
                    lp_lon = aatan2(sz * sin(Az),
                        this->m_proj_parm.cp0 * cz - this->m_proj_parm.sp0 * sz * cAz);
                }

                static inline std::string get_name()
                {
                    return "oea_spheroid";
                }

            };

            // Oblated Equal Area
            template <typename Params, typename Parameters, typename T>
            inline void setup_oea(Params const& params, Parameters& par, par_oea<T>& proj_parm)
            {
                if (((proj_parm.n = pj_get_param_f<T, srs::spar::n>(params, "n", srs::dpar::n)) <= 0.) ||
                    ((proj_parm.m = pj_get_param_f<T, srs::spar::m>(params, "m", srs::dpar::m)) <= 0.)) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_invalid_m_or_n) );
                } else {
                    proj_parm.theta = pj_get_param_r<T, srs::spar::theta>(params, "theta", srs::dpar::theta);
                    proj_parm.sp0 = sin(par.phi0);
                    proj_parm.cp0 = cos(par.phi0);
                    proj_parm.rn = 1./ proj_parm.n;
                    proj_parm.rm = 1./ proj_parm.m;
                    proj_parm.two_r_n = 2. * proj_parm.rn;
                    proj_parm.two_r_m = 2. * proj_parm.rm;
                    proj_parm.hm = 0.5 * proj_parm.m;
                    proj_parm.hn = 0.5 * proj_parm.n;
                    par.es = 0.;
                }
            }

    }} // namespace detail::oea
    #endif // doxygen

    /*!
        \brief Oblated Equal Area projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - n (real)
         - m (real)
         - theta: Theta (degrees)
        \par Example
        \image html ex_oea.gif
    */
    template <typename T, typename Parameters>
    struct oea_spheroid : public detail::oea::base_oea_spheroid<T, Parameters>
    {
        template <typename Params>
        inline oea_spheroid(Params const& params, Parameters & par)
        {
            detail::oea::setup_oea(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_oea, oea_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(oea_entry, oea_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(oea_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(oea, oea_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_OEA_HPP

