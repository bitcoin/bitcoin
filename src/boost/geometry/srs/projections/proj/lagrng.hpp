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

#ifndef BOOST_GEOMETRY_PROJECTIONS_LAGRNG_HPP
#define BOOST_GEOMETRY_PROJECTIONS_LAGRNG_HPP

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
    namespace detail { namespace lagrng
    {

            static const double tolerance = 1e-10;

            template <typename T>
            struct par_lagrng
            {
                T    a1;
                T    rw;
                T    hrw;
            };

            template <typename T, typename Parameters>
            struct base_lagrng_spheroid
            {
                par_lagrng<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T v, c;

                    if (fabs(fabs(lp_lat) - half_pi) < tolerance) {
                        xy_x = 0;
                        xy_y = lp_lat < 0 ? -2. : 2.;
                    } else {
                        lp_lat = sin(lp_lat);
                        v = this->m_proj_parm.a1 * math::pow((T(1) + lp_lat)/(T(1) - lp_lat), this->m_proj_parm.hrw);
                        if ((c = 0.5 * (v + 1./v) + cos(lp_lon *= this->m_proj_parm.rw)) < tolerance) {
                            BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                        }
                        xy_x = 2. * sin(lp_lon) / c;
                        xy_y = (v - 1./v) / c;
                    }
                }

                static inline std::string get_name()
                {
                    return "lagrng_spheroid";
                }

            };

            // Lagrange
            template <typename Params, typename Parameters, typename T>
            inline void setup_lagrng(Params const& params, Parameters& par, par_lagrng<T>& proj_parm)
            {
                T phi1;

                proj_parm.rw = 0.0;
                bool is_w_set = pj_param_f<srs::spar::w>(params, "W", srs::dpar::w, proj_parm.rw);
                
                // Boost.Geometry specific, set default parameters manually
                if (! is_w_set) {
                    bool const use_defaults = ! pj_get_param_b<srs::spar::no_defs>(params, "no_defs", srs::dpar::no_defs);
                    if (use_defaults) {
                        proj_parm.rw = 2;
                    }
                }

                if (proj_parm.rw <= 0)
                    BOOST_THROW_EXCEPTION( projection_exception(error_w_or_m_zero_or_less) );

                proj_parm.rw = 1. / proj_parm.rw;
                proj_parm.hrw = 0.5 * proj_parm.rw;
                phi1 = pj_get_param_r<T, srs::spar::lat_1>(params, "lat_1", srs::dpar::lat_1);
                if (fabs(fabs(phi1 = sin(phi1)) - 1.) < tolerance)
                    BOOST_THROW_EXCEPTION( projection_exception(error_lat_larger_than_90) );

                proj_parm.a1 = math::pow((T(1) - phi1)/(T(1) + phi1), proj_parm.hrw);

                par.es = 0.;
            }

    }} // namespace detail::lagrng
    #endif // doxygen

    /*!
        \brief Lagrange projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Projection parameters
         - W (real)
         - lat_1: Latitude of first standard parallel (degrees)
        \par Example
        \image html ex_lagrng.gif
    */
    template <typename T, typename Parameters>
    struct lagrng_spheroid : public detail::lagrng::base_lagrng_spheroid<T, Parameters>
    {
        template <typename Params>
        inline lagrng_spheroid(Params const& params, Parameters & par)
        {
            detail::lagrng::setup_lagrng(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_lagrng, lagrng_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(lagrng_entry, lagrng_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(lagrng_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(lagrng, lagrng_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_LAGRNG_HPP

