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

#ifndef BOOST_GEOMETRY_PROJECTIONS_URM5_HPP
#define BOOST_GEOMETRY_PROJECTIONS_URM5_HPP

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
    namespace detail { namespace urm5
    {
            template <typename T>
            struct par_urm5
            {
                T m, rmn, q3, n;
            };

            template <typename T, typename Parameters>
            struct base_urm5_spheroid
            {
                par_urm5<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const&, T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    T t;

                    t = lp_lat = aasin(this->m_proj_parm.n * sin(lp_lat));
                    xy_x = this->m_proj_parm.m * lp_lon * cos(lp_lat);
                    t *= t;
                    xy_y = lp_lat * (1. + t * this->m_proj_parm.q3) * this->m_proj_parm.rmn;
                }

                static inline std::string get_name()
                {
                    return "urm5_spheroid";
                }

            };

            // Urmaev V
            template <typename Params, typename Parameters, typename T>
            inline void setup_urm5(Params const& params, Parameters& par, par_urm5<T>& proj_parm)
            {
                T alpha, t;

                if (pj_param_f<srs::spar::n>(params, "n", srs::dpar::n, proj_parm.n)) {
                    if (proj_parm.n <= 0. || proj_parm.n > 1.)
                        BOOST_THROW_EXCEPTION( projection_exception(error_n_out_of_range) );
                } else {
                    BOOST_THROW_EXCEPTION( projection_exception(error_n_out_of_range) );
                }
                proj_parm.q3 = pj_get_param_f<T, srs::spar::q>(params, "q", srs::dpar::q) / 3.;
                alpha = pj_get_param_r<T, srs::spar::alpha>(params, "alpha", srs::dpar::alpha);
                t = proj_parm.n * sin(alpha);
                proj_parm.m = cos(alpha) / sqrt(1. - t * t);
                proj_parm.rmn = 1. / (proj_parm.m * proj_parm.n);

                par.es = 0.;
            }

    }} // namespace detail::urm5
    #endif // doxygen

    /*!
        \brief Urmaev V projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
         - no inverse
        \par Projection parameters
         - n (real)
         - q (real)
         - alpha: Alpha (degrees)
        \par Example
        \image html ex_urm5.gif
    */
    template <typename T, typename Parameters>
    struct urm5_spheroid : public detail::urm5::base_urm5_spheroid<T, Parameters>
    {
        template <typename Params>
        inline urm5_spheroid(Params const& params, Parameters & par)
        {
            detail::urm5::setup_urm5(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_urm5, urm5_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(urm5_entry, urm5_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(urm5_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(urm5, urm5_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_URM5_HPP

