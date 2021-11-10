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

#ifndef BOOST_GEOMETRY_PROJECTIONS_BACON_HPP
#define BOOST_GEOMETRY_PROJECTIONS_BACON_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace bacon
    {

            //static const double half_pi_sqr = 2.46740110027233965467;
            static const double epsilon = 1e-10;

            struct par_bacon
            {
                bool bacn;
                bool ortl;
            };

            template <typename T, typename Parameters>
            struct base_bacon_spheroid
            {
                par_bacon m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T half_pi_sqr = detail::half_pi_sqr<T>();

                    T ax, f;

                    xy_y = this->m_proj_parm.bacn ? half_pi * sin(lp_lat) : lp_lat;
                    if ((ax = fabs(lp_lon)) >= epsilon) {
                        if (this->m_proj_parm.ortl && ax >= half_pi)
                            xy_x = sqrt(half_pi_sqr - lp_lat * lp_lat + epsilon) + ax - half_pi;
                        else {
                            f = 0.5 * (half_pi_sqr / ax + ax);
                            xy_x = ax - f + sqrt(f * f - xy_y * xy_y);
                        }
                        if (lp_lon < 0.) xy_x = - xy_x;
                    } else
                        xy_x = 0.;
                }

                static inline std::string get_name()
                {
                    return "bacon_spheroid";
                }

            };

            // Apian Globular I
            template <typename Parameters>
            inline void setup_apian(Parameters& par, par_bacon& proj_parm)
            {
                proj_parm.bacn = proj_parm.ortl = false;
                par.es = 0.;
            }

            // Ortelius Oval
            template <typename Parameters>
            inline void setup_ortel(Parameters& par, par_bacon& proj_parm)
            {
                proj_parm.bacn = false;
                proj_parm.ortl = true;
                par.es = 0.;
            }

            // Bacon Globular
            template <typename Parameters>
            inline void setup_bacon(Parameters& par, par_bacon& proj_parm)
            {
                proj_parm.bacn = true;
                proj_parm.ortl = false;
                par.es = 0.;
            }

    }} // namespace detail::bacon
    #endif // doxygen

    /*!
        \brief Apian Globular I projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Example
        \image html ex_apian.gif
    */
    template <typename T, typename Parameters>
    struct apian_spheroid : public detail::bacon::base_bacon_spheroid<T, Parameters>
    {
        template <typename Params>
        inline apian_spheroid(Params const& , Parameters & par)
        {
            detail::bacon::setup_apian(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Ortelius Oval projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Example
        \image html ex_ortel.gif
    */
    template <typename T, typename Parameters>
    struct ortel_spheroid : public detail::bacon::base_bacon_spheroid<T, Parameters>
    {
        template <typename Params>
        inline ortel_spheroid(Params const& , Parameters & par)
        {
            detail::bacon::setup_ortel(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Bacon Globular projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Example
        \image html ex_bacon.gif
    */
    template <typename T, typename Parameters>
    struct bacon_spheroid : public detail::bacon::base_bacon_spheroid<T, Parameters>
    {
        template <typename Params>
        inline bacon_spheroid(Params const& , Parameters & par)
        {
            detail::bacon::setup_bacon(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_apian, apian_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_bacon, bacon_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_ortel, ortel_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(apian_entry, apian_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(ortel_entry, ortel_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(bacon_entry, bacon_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(bacon_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(apian, apian_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(ortel, ortel_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(bacon, bacon_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_BACON_HPP

