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

#ifndef BOOST_GEOMETRY_PROJECTIONS_FOUC_S_HPP
#define BOOST_GEOMETRY_PROJECTIONS_FOUC_S_HPP

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
    namespace detail { namespace fouc_s
    {

            static const int max_iter = 10;
            static const double loop_tol = 1e-7;

            template <typename T>
            struct par_fouc_s
            {
                T n, n1;
            };

            template <typename T, typename Parameters>
            struct base_fouc_s_spheroid
            {
                par_fouc_s<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T t;

                    t = cos(lp_lat);
                    xy_x = lp_lon * t / (this->m_proj_parm.n + this->m_proj_parm.n1 * t);
                    xy_y = this->m_proj_parm.n * lp_lat + this->m_proj_parm.n1 * sin(lp_lat);
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static T const half_pi = detail::half_pi<T>();

                    T V;
                    int i;

                    if (this->m_proj_parm.n != 0.0) {
                        lp_lat = xy_y;
                        for (i = max_iter; i ; --i) {
                            lp_lat -= V = (this->m_proj_parm.n * lp_lat + this->m_proj_parm.n1 * sin(lp_lat) - xy_y ) /
                                (this->m_proj_parm.n + this->m_proj_parm.n1 * cos(lp_lat));
                            if (fabs(V) < loop_tol)
                                break;
                        }
                        if (!i)
                            lp_lat = xy_y < 0. ? -half_pi : half_pi;
                    } else
                        lp_lat = aasin(xy_y);
                    V = cos(lp_lat);
                    lp_lon = xy_x * (this->m_proj_parm.n + this->m_proj_parm.n1 * V) / V;
                }

                static inline std::string get_name()
                {
                    return "fouc_s_spheroid";
                }

            };

            // Foucaut Sinusoidal
            template <typename Params, typename Parameters, typename T>
            inline void setup_fouc_s(Params const& params, Parameters& par, par_fouc_s<T>& proj_parm)
            {
                proj_parm.n = pj_get_param_f<T, srs::spar::n>(params, "n", srs::dpar::n);
                if ((proj_parm.n < 0.) || (proj_parm.n > 1.))
                    BOOST_THROW_EXCEPTION( projection_exception(error_n_out_of_range) );

                proj_parm.n1 = 1. - proj_parm.n;
                par.es = 0;
            }

    }} // namespace detail::fouc_s
    #endif // doxygen

    /*!
        \brief Foucaut Sinusoidal projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Projection parameters
         - n (real)
        \par Example
        \image html ex_fouc_s.gif
    */
    template <typename T, typename Parameters>
    struct fouc_s_spheroid : public detail::fouc_s::base_fouc_s_spheroid<T, Parameters>
    {
        template <typename Params>
        inline fouc_s_spheroid(Params const& params, Parameters & par)
        {
            detail::fouc_s::setup_fouc_s(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_fouc_s, fouc_s_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(fouc_s_entry, fouc_s_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(fouc_s_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(fouc_s, fouc_s_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_FOUC_S_HPP

