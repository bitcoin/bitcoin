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

#ifndef BOOST_GEOMETRY_PROJECTIONS_URMFPS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_URMFPS_HPP

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
    namespace detail { namespace urmfps
    {

            static const double C_x = 0.8773826753;
            static const double Cy = 1.139753528477;

            template <typename T>
            struct par_urmfps
            {
                T    n, C_y;
            };

            template <typename T, typename Parameters>
            struct base_urmfps_spheroid
            {
                par_urmfps<T> m_proj_parm;

                // FORWARD(s_forward)  sphere
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    lp_lat = aasin(this->m_proj_parm.n * sin(lp_lat));
                    xy_x = C_x * lp_lon * cos(lp_lat);
                    xy_y = this->m_proj_parm.C_y * lp_lat;
                }

                // INVERSE(s_inverse)  sphere
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    xy_y /= this->m_proj_parm.C_y;
                    lp_lat = aasin(sin(xy_y) / this->m_proj_parm.n);
                    lp_lon = xy_x / (C_x * cos(xy_y));
                }

                static inline std::string get_name()
                {
                    return "urmfps_spheroid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters& par, par_urmfps<T>& proj_parm) 
            {
                proj_parm.C_y = Cy / proj_parm.n;
                par.es = 0.;
            }


            // Urmaev Flat-Polar Sinusoidal
            template <typename Params, typename Parameters, typename T>
            inline void setup_urmfps(Params const& params, Parameters& par, par_urmfps<T>& proj_parm)
            {
                if (pj_param_f<srs::spar::n>(params, "n", srs::dpar::n, proj_parm.n)) {
                    if (proj_parm.n <= 0. || proj_parm.n > 1.)
                        BOOST_THROW_EXCEPTION( projection_exception(error_n_out_of_range) );
                } else
                    BOOST_THROW_EXCEPTION( projection_exception(error_n_out_of_range) );

                setup(par, proj_parm);
            }

            // Wagner I (Kavraisky VI)
            template <typename Parameters, typename T>
            inline void setup_wag1(Parameters& par, par_urmfps<T>& proj_parm)
            {
                proj_parm.n = 0.8660254037844386467637231707;
                setup(par, proj_parm);
            }

    }} // namespace detail::urmfps
    #endif // doxygen

    /*!
        \brief Urmaev Flat-Polar Sinusoidal projection
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
        \image html ex_urmfps.gif
    */
    template <typename T, typename Parameters>
    struct urmfps_spheroid : public detail::urmfps::base_urmfps_spheroid<T, Parameters>
    {
        template <typename Params>
        inline urmfps_spheroid(Params const& params, Parameters & par)
        {
            detail::urmfps::setup_urmfps(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Wagner I (Kavraisky VI) projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_wag1.gif
    */
    template <typename T, typename Parameters>
    struct wag1_spheroid : public detail::urmfps::base_urmfps_spheroid<T, Parameters>
    {
        template <typename Params>
        inline wag1_spheroid(Params const& , Parameters & par)
        {
            detail::urmfps::setup_wag1(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_urmfps, urmfps_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_wag1, wag1_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(urmfps_entry, urmfps_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(wag1_entry, wag1_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(urmfps_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(urmfps, urmfps_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(wag1, wag1_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_URMFPS_HPP

