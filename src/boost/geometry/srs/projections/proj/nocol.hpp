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

#ifndef BOOST_GEOMETRY_PROJECTIONS_NOCOL_HPP
#define BOOST_GEOMETRY_PROJECTIONS_NOCOL_HPP

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
    namespace detail { namespace nocol
    {

            static const double epsilon = 1e-10;

            template <typename T, typename Parameters>
            struct base_nocol_spheroid
            {
                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    if (fabs(lp_lon) < epsilon) {
                        xy_x = 0;
                        xy_y = lp_lat;
                    } else if (fabs(lp_lat) < epsilon) {
                        xy_x = lp_lon;
                        xy_y = 0.;
                    } else if (fabs(fabs(lp_lon) - half_pi) < epsilon) {
                        xy_x = lp_lon * cos(lp_lat);
                        xy_y = half_pi * sin(lp_lat);
                    } else if (fabs(fabs(lp_lat) - half_pi) < epsilon) {
                        xy_x = 0;
                        xy_y = lp_lat;
                    } else {
                        T tb, c, d, m, n, r2, sp;

                        tb = half_pi / lp_lon - lp_lon / half_pi;
                        c = lp_lat / half_pi;
                        d = (1 - c * c)/((sp = sin(lp_lat)) - c);
                        r2 = tb / d;
                        r2 *= r2;
                        m = (tb * sp / d - 0.5 * tb)/(1. + r2);
                        n = (sp / r2 + 0.5 * d)/(1. + 1./r2);
                        xy_x = cos(lp_lat);
                        xy_x = sqrt(m * m + xy_x * xy_x / (1. + r2));
                        xy_x = half_pi * ( m + (lp_lon < 0. ? -xy_x : xy_x));
                        xy_y = sqrt(n * n - (sp * sp / r2 + d * sp - 1.) /
                            (1. + 1./r2));
                        xy_y = half_pi * ( n + (lp_lat < 0. ? xy_y : -xy_y ));
                    }
                }

                static inline std::string get_name()
                {
                    return "nocol_spheroid";
                }

            };

            // Nicolosi Globular
            template <typename Parameters>
            inline void setup_nicol(Parameters& par)
            {
                par.es = 0.;
            }

    }} // namespace detail::nocol
    #endif // doxygen

    /*!
        \brief Nicolosi Globular projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Example
        \image html ex_nicol.gif
    */
    template <typename T, typename Parameters>
    struct nicol_spheroid : public detail::nocol::base_nocol_spheroid<T, Parameters>
    {
        template <typename Params>
        inline nicol_spheroid(Params const& , Parameters & par)
        {
            detail::nocol::setup_nicol(par);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_nicol, nicol_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(nicol_entry, nicol_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(nocol_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(nicol, nicol_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_NOCOL_HPP

