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

#ifndef BOOST_GEOMETRY_PROJECTIONS_MOLL_HPP
#define BOOST_GEOMETRY_PROJECTIONS_MOLL_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/aasincos.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace moll
    {

            static const int max_iter = 10;
            static const double loop_tol = 1e-7;

            template <typename T>
            struct par_moll
            {
                T    C_x, C_y, C_p;
            };

            template <typename T, typename Parameters>
            struct base_moll_spheroid
            {
                par_moll<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    static const T half_pi = detail::half_pi<T>();

                    T k, V;
                    int i;

                    k = this->m_proj_parm.C_p * sin(lp_lat);
                    for (i = max_iter; i ; --i) {
                        lp_lat -= V = (lp_lat + sin(lp_lat) - k) /
                            (1. + cos(lp_lat));
                        if (fabs(V) < loop_tol)
                            break;
                    }
                    if (!i)
                        lp_lat = (lp_lat < 0.) ? -half_pi : half_pi;
                    else
                        lp_lat *= 0.5;
                    xy_x = this->m_proj_parm.C_x * lp_lon * cos(lp_lat);
                    xy_y = this->m_proj_parm.C_y * sin(lp_lat);
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    static const T pi = detail::pi<T>();

                    lp_lat = aasin(xy_y / this->m_proj_parm.C_y);
                    lp_lon = xy_x / (this->m_proj_parm.C_x * cos(lp_lat));
                    if (fabs(lp_lon) < pi) {
                        lp_lat += lp_lat;
                        lp_lat = aasin((lp_lat + sin(lp_lat)) / this->m_proj_parm.C_p);
                    } else {
                        lp_lon = lp_lat = HUGE_VAL;
                    }
                }

                static inline std::string get_name()
                {
                    return "moll_spheroid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters& par, par_moll<T>& proj_parm, T const& p) 
            {
                T r, sp, p2 = p + p;

                par.es = 0;
                sp = sin(p);
                r = sqrt(geometry::math::two_pi<T>() * sp / (p2 + sin(p2)));

                proj_parm.C_x = 2. * r / geometry::math::pi<T>();
                proj_parm.C_y = r / sp;
                proj_parm.C_p = p2 + sin(p2);
            }


            // Mollweide
            template <typename Parameters, typename T>
            inline void setup_moll(Parameters& par, par_moll<T>& proj_parm)
            {
                setup(par, proj_parm, geometry::math::half_pi<T>());
            }

            // Wagner IV
            template <typename Parameters, typename T>
            inline void setup_wag4(Parameters& par, par_moll<T>& proj_parm)
            {
                setup(par, proj_parm, geometry::math::pi<T>()/3.);
            }

            // Wagner V
            template <typename Parameters, typename T>
            inline void setup_wag5(Parameters& par, par_moll<T>& proj_parm)
            {
                par.es = 0;
                proj_parm.C_x = 0.90977;
                proj_parm.C_y = 1.65014;
                proj_parm.C_p = 3.00896;
            }

    }} // namespace detail::moll
    #endif // doxygen

    /*!
        \brief Mollweide projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_moll.gif
    */
    template <typename T, typename Parameters>
    struct moll_spheroid : public detail::moll::base_moll_spheroid<T, Parameters>
    {
        template <typename Params>
        inline moll_spheroid(Params const& , Parameters & par)
        {
            detail::moll::setup_moll(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Wagner IV projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_wag4.gif
    */
    template <typename T, typename Parameters>
    struct wag4_spheroid : public detail::moll::base_moll_spheroid<T, Parameters>
    {
        template <typename Params>
        inline wag4_spheroid(Params const& , Parameters & par)
        {
            detail::moll::setup_wag4(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Wagner V projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_wag5.gif
    */
    template <typename T, typename Parameters>
    struct wag5_spheroid : public detail::moll::base_moll_spheroid<T, Parameters>
    {
        template <typename Params>
        inline wag5_spheroid(Params const& , Parameters & par)
        {
            detail::moll::setup_wag5(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_moll, moll_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_wag4, wag4_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_wag5, wag5_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(moll_entry, moll_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(wag4_entry, wag4_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(wag5_entry, wag5_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(moll_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(moll, moll_entry);
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(wag4, wag4_entry);
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(wag5, wag5_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_MOLL_HPP

