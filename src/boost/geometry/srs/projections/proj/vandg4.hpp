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

#ifndef BOOST_GEOMETRY_PROJECTIONS_VANDG4_HPP
#define BOOST_GEOMETRY_PROJECTIONS_VANDG4_HPP

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
    namespace detail { namespace vandg4
    {

            static const double tolerance = 1e-10;

            template <typename T, typename Parameters>
            struct base_vandg4_spheroid
            {
                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();
                    static const T two_div_pi = detail::two_div_pi<T>();

                    T x1, t, bt, ct, ft, bt2, ct2, dt, dt2;

                    if (fabs(lp_lat) < tolerance) {
                        xy_x = lp_lon;
                        xy_y = 0.;
                    } else if (fabs(lp_lon) < tolerance || fabs(fabs(lp_lat) - half_pi) < tolerance) {
                        xy_x = 0.;
                        xy_y = lp_lat;
                    } else {
                        bt = fabs(two_div_pi * lp_lat);
                        bt2 = bt * bt;
                        ct = 0.5 * (bt * (8. - bt * (2. + bt2)) - 5.)
                            / (bt2 * (bt - 1.));
                        ct2 = ct * ct;
                        dt = two_div_pi * lp_lon;
                        dt = dt + 1. / dt;
                        dt = sqrt(dt * dt - 4.);
                        if ((fabs(lp_lon) - half_pi) < 0.) dt = -dt;
                        dt2 = dt * dt;
                        x1 = bt + ct; x1 *= x1;
                        t = bt + 3.*ct;
                        ft = x1 * (bt2 + ct2 * dt2 - 1.) + (1.-bt2) * (
                            bt2 * (t * t + 4. * ct2) +
                            ct2 * (12. * bt * ct + 4. * ct2) );
                        x1 = (dt*(x1 + ct2 - 1.) + 2.*sqrt(ft)) /
                            (4.* x1 + dt2);
                        xy_x = half_pi * x1;
                        xy_y = half_pi * sqrt(1. + dt * fabs(x1) - x1 * x1);
                        if (lp_lon < 0.) xy_x = -xy_x;
                        if (lp_lat < 0.) xy_y = -xy_y;
                    }
                }

                static inline std::string get_name()
                {
                    return "vandg4_spheroid";
                }

            };

            // van der Grinten IV
            template <typename Parameters>
            inline void setup_vandg4(Parameters& par)
            {
                par.es = 0.;
            }

    }} // namespace detail::vandg4
    #endif // doxygen

    /*!
        \brief van der Grinten IV projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
         - no inverse
        \par Example
        \image html ex_vandg4.gif
    */
    template <typename T, typename Parameters>
    struct vandg4_spheroid : public detail::vandg4::base_vandg4_spheroid<T, Parameters>
    {
        template <typename Params>
        inline vandg4_spheroid(Params const& , Parameters & par)
        {
            detail::vandg4::setup_vandg4(par);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_F(srs::spar::proj_vandg4, vandg4_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_F(vandg4_entry, vandg4_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(vandg4_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(vandg4, vandg4_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_VANDG4_HPP

