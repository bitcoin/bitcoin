// Boost.Geometry - gis-projections (based on PROJ4)

// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IGH_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IGH_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/proj/gn_sinu.hpp>
#include <boost/geometry/srs/projections/proj/moll.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/shared_ptr.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace igh
    {
            template <typename T>
            struct par_igh_zone
            {
                T x0;
                T y0;
                T lam0;
            };

            // NOTE: x0, y0, lam0 are not used in moll nor sinu projections
            // so it is a waste of memory to keep 12 copies of projections
            // with parameters as in the original Proj4.

            // TODO: It would be possible to further decrease the size of par_igh
            // because spherical sinu and moll has constant parameters.
            // TODO: Furthermore there is no need to store par_igh_zone parameters
            // since they are constant for zones. In both fwd() and inv() there are
            // parts of code dependent on specific zones (if statements) anyway
            // so these parameters could be hardcoded there instead of stored.

            template <typename T, typename Parameters>
            struct par_igh
            {
                moll_spheroid<T, Parameters> moll;
                sinu_spheroid<T, Parameters> sinu;
                par_igh_zone<T> zones[12];
                T dy0;

                // NOTE: The constructors of moll and sinu projections sets
                // par.es = 0

                template <typename Params>
                inline par_igh(Params const& params, Parameters & par)
                    : moll(params, par)
                    , sinu(params, par)
                {}

                inline void fwd(int zone, Parameters const& par, T const& lp_lon, T const& lp_lat, T & xy_x, T & xy_y) const
                {
                    if (zone <= 2 || zone >= 9) // 1, 2, 9, 10, 11, 12
                        moll.fwd(par, lp_lon, lp_lat, xy_x, xy_y);
                    else // 3, 4, 5, 6, 7, 8
                        sinu.fwd(par, lp_lon, lp_lat, xy_x, xy_y);
                }

                inline void inv(int zone, Parameters const& par, T const& xy_x, T const& xy_y, T & lp_lon, T & lp_lat) const
                {
                    if (zone <= 2 || zone >= 9) // 1, 2, 9, 10, 11, 12
                        moll.inv(par, xy_x, xy_y, lp_lon, lp_lat);
                    else // 3, 4, 5, 6, 7, 8
                        sinu.inv(par, xy_x, xy_y, lp_lon, lp_lat);
                }

                inline void set_zone(int zone, T const& x_0, T const& y_0, T const& lon_0)
                {
                    zones[zone - 1].x0 = x_0;
                    zones[zone - 1].y0 = y_0;
                    zones[zone - 1].lam0 = lon_0;
                }

                inline par_igh_zone<T> const& get_zone(int zone) const
                {
                    return zones[zone - 1];
                }
            };

            /* 40d 44' 11.8" [degrees] */
            template <typename T>
            inline T d4044118() { return (T(40) + T(44)/T(60.) + T(11.8)/T(3600.)) * geometry::math::d2r<T>(); }

            template <typename T>
            inline T d10() { return T(10) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d20() { return T(20) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d30() { return T(30) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d40() { return T(40) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d50() { return T(50) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d60() { return T(60) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d80() { return T(80) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d90() { return T(90) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d100() { return T(100) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d140() { return T(140) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d160() { return T(160) * geometry::math::d2r<T>(); }
            template <typename T>
            inline T d180() { return T(180) * geometry::math::d2r<T>(); }

            static const double epsilon = 1.e-10; // allow a little 'slack' on zone edge positions

            template <typename T, typename Parameters>
            struct base_igh_spheroid
            {
                par_igh<T, Parameters> m_proj_parm;

                template <typename Params>
                inline base_igh_spheroid(Params const& params, Parameters & par)
                    : m_proj_parm(params, par)
                {}

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T d4044118 = igh::d4044118<T>();
                    static const T d20  =  igh::d20<T>();
                    static const T d40  =  igh::d40<T>();
                    static const T d80  =  igh::d80<T>();
                    static const T d100 = igh::d100<T>();

                        int z;
                        if (lp_lat >=  d4044118) {          // 1|2
                          z = (lp_lon <= -d40 ? 1: 2);
                        }
                        else if (lp_lat >=  0) {            // 3|4
                          z = (lp_lon <= -d40 ? 3: 4);
                        }
                        else if (lp_lat >= -d4044118) {     // 5|6|7|8
                               if (lp_lon <= -d100) z =  5; // 5
                          else if (lp_lon <=  -d20) z =  6; // 6
                          else if (lp_lon <=   d80) z =  7; // 7
                          else z = 8;                       // 8
                        }
                        else {                              // 9|10|11|12
                               if (lp_lon <= -d100) z =  9; // 9
                          else if (lp_lon <=  -d20) z = 10; // 10
                          else if (lp_lon <=   d80) z = 11; // 11
                          else z = 12;                      // 12
                        }

                        lp_lon -= this->m_proj_parm.get_zone(z).lam0;
                        this->m_proj_parm.fwd(z, par, lp_lon, lp_lat, xy_x, xy_y);
                        xy_x += this->m_proj_parm.get_zone(z).x0;
                        xy_y += this->m_proj_parm.get_zone(z).y0;
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T d4044118 = igh::d4044118<T>();
                    static const T d10  =  igh::d10<T>();
                    static const T d20  =  igh::d20<T>();
                    static const T d40  =  igh::d40<T>();
                    static const T d50  =  igh::d50<T>();
                    static const T d60  =  igh::d60<T>();
                    static const T d80  =  igh::d80<T>();
                    static const T d90  =  igh::d90<T>();
                    static const T d100 = igh::d100<T>();
                    static const T d160 = igh::d160<T>();
                    static const T d180 = igh::d180<T>();

                    static const T c2 = 2.0;
                    
                    const T y90 = this->m_proj_parm.dy0 + sqrt(c2); // lt=90 corresponds to y=y0+sqrt(2.0)

                        int z = 0;
                        if (xy_y > y90+epsilon || xy_y < -y90+epsilon) // 0
                          z = 0;
                        else if (xy_y >=  d4044118)       // 1|2
                          z = (xy_x <= -d40? 1: 2);
                        else if (xy_y >=  0)              // 3|4
                          z = (xy_x <= -d40? 3: 4);
                        else if (xy_y >= -d4044118) {     // 5|6|7|8
                               if (xy_x <= -d100) z =  5; // 5
                          else if (xy_x <=  -d20) z =  6; // 6
                          else if (xy_x <=   d80) z =  7; // 7
                          else z = 8;                     // 8
                        }
                        else {                            // 9|10|11|12
                               if (xy_x <= -d100) z =  9; // 9
                          else if (xy_x <=  -d20) z = 10; // 10
                          else if (xy_x <=   d80) z = 11; // 11
                          else z = 12;                    // 12
                        }

                        if (z)
                        {
                          int ok = 0;

                          xy_x -= this->m_proj_parm.get_zone(z).x0;
                          xy_y -= this->m_proj_parm.get_zone(z).y0;
                          this->m_proj_parm.inv(z, par, xy_x, xy_y, lp_lon, lp_lat);
                          lp_lon += this->m_proj_parm.get_zone(z).lam0;

                          switch (z) {
                            case  1: ok = (lp_lon >= -d180-epsilon && lp_lon <=  -d40+epsilon) ||
                                         ((lp_lon >=  -d40-epsilon && lp_lon <=  -d10+epsilon) &&
                                          (lp_lat >=   d60-epsilon && lp_lat <=   d90+epsilon)); break;
                            case  2: ok = (lp_lon >=  -d40-epsilon && lp_lon <=  d180+epsilon) ||
                                         ((lp_lon >= -d180-epsilon && lp_lon <= -d160+epsilon) &&
                                          (lp_lat >=   d50-epsilon && lp_lat <=   d90+epsilon)) ||
                                         ((lp_lon >=  -d50-epsilon && lp_lon <=  -d40+epsilon) &&
                                          (lp_lat >=   d60-epsilon && lp_lat <=   d90+epsilon)); break;
                            case  3: ok = (lp_lon >= -d180-epsilon && lp_lon <=  -d40+epsilon); break;
                            case  4: ok = (lp_lon >=  -d40-epsilon && lp_lon <=  d180+epsilon); break;
                            case  5: ok = (lp_lon >= -d180-epsilon && lp_lon <= -d100+epsilon); break;
                            case  6: ok = (lp_lon >= -d100-epsilon && lp_lon <=  -d20+epsilon); break;
                            case  7: ok = (lp_lon >=  -d20-epsilon && lp_lon <=   d80+epsilon); break;
                            case  8: ok = (lp_lon >=   d80-epsilon && lp_lon <=  d180+epsilon); break;
                            case  9: ok = (lp_lon >= -d180-epsilon && lp_lon <= -d100+epsilon); break;
                            case 10: ok = (lp_lon >= -d100-epsilon && lp_lon <=  -d20+epsilon); break;
                            case 11: ok = (lp_lon >=  -d20-epsilon && lp_lon <=   d80+epsilon); break;
                            case 12: ok = (lp_lon >=   d80-epsilon && lp_lon <=  d180+epsilon); break;
                          }

                          z = (!ok? 0: z); // projectable?
                        }
                     // if (!z) pj_errno = -15; // invalid x or y
                        if (!z) lp_lon = HUGE_VAL;
                        if (!z) lp_lat = HUGE_VAL;
                }

                static inline std::string get_name()
                {
                    return "igh_spheroid";
                }

            };

            // Interrupted Goode Homolosine
            template <typename Params, typename Parameters, typename T>
            inline void setup_igh(Params const& , Parameters& par, par_igh<T, Parameters>& proj_parm)
            {
                static const T d0   =  0;
                static const T d4044118 = igh::d4044118<T>();
                static const T d20  =  igh::d20<T>();
                static const T d30  =  igh::d30<T>();
                static const T d60  =  igh::d60<T>();
                static const T d100 = igh::d100<T>();
                static const T d140 = igh::d140<T>();
                static const T d160 = igh::d160<T>();

            /*
              Zones:

                -180            -40                       180
                  +--------------+-------------------------+    Zones 1,2,9,10,11 & 12:
                  |1             |2                        |      Mollweide projection
                  |              |                         |
                  +--------------+-------------------------+    Zones 3,4,5,6,7 & 8:
                  |3             |4                        |      Sinusoidal projection
                  |              |                         |
                0 +-------+------+-+-----------+-----------+
                  |5      |6       |7          |8          |
                  |       |        |           |           |
                  +-------+--------+-----------+-----------+
                  |9      |10      |11         |12         |
                  |       |        |           |           |
                  +-------+--------+-----------+-----------+
                -180    -100      -20         80          180
            */
                
                    T lp_lam = 0, lp_phi = d4044118;
                    T xy1_x, xy1_y;
                    T xy3_x, xy3_y;

                    // sinusoidal zones
                    proj_parm.set_zone(3, -d100, d0, -d100);
                    proj_parm.set_zone(4,   d30, d0,   d30);
                    proj_parm.set_zone(5, -d160, d0, -d160);
                    proj_parm.set_zone(6,  -d60, d0,  -d60);
                    proj_parm.set_zone(7,   d20, d0,   d20);
                    proj_parm.set_zone(8,  d140, d0,  d140);

                    // mollweide zones
                    proj_parm.set_zone(1, -d100, d0, -d100);

                    // NOTE: x0, y0, lam0 are not used in moll nor sinu fwd
                    // so the order of initialization doesn't matter that much.
                    // But keep the original one from Proj4.

                    // y0 ?
                    proj_parm.fwd(1, par, lp_lam, lp_phi, xy1_x, xy1_y); // zone 1
                    proj_parm.fwd(3, par, lp_lam, lp_phi, xy3_x, xy3_y); // zone 3
                    // y0 + xy1_y = xy3_y for lt = 40d44'11.8"
                    proj_parm.dy0 = xy3_y - xy1_y;

                    proj_parm.zones[0].y0 = proj_parm.dy0; // zone 1

                    // mollweide zones (cont'd)
                    proj_parm.set_zone(2,   d30,  proj_parm.dy0,   d30);
                    proj_parm.set_zone(9, -d160, -proj_parm.dy0, -d160);
                    proj_parm.set_zone(10, -d60, -proj_parm.dy0,  -d60);
                    proj_parm.set_zone(11,  d20, -proj_parm.dy0,   d20);
                    proj_parm.set_zone(12, d140, -proj_parm.dy0,  d140);

                    // NOTE: Already done before in sinu and moll constructor
                    //par.es = 0.;
            }

    }} // namespace detail::igh
    #endif // doxygen

    /*!
        \brief Interrupted Goode Homolosine projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_igh.gif
    */
    template <typename T, typename Parameters>
    struct igh_spheroid : public detail::igh::base_igh_spheroid<T, Parameters>
    {
        template <typename Params>
        inline igh_spheroid(Params const& params, Parameters & par)
            : detail::igh::base_igh_spheroid<T, Parameters>(params, par)
        {
            detail::igh::setup_igh(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_igh, igh_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(igh_entry, igh_spheroid)

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(igh_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(igh, igh_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_IGH_HPP

