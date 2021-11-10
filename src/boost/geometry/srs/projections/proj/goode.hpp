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

#ifndef BOOST_GEOMETRY_PROJECTIONS_GOODE_HPP
#define BOOST_GEOMETRY_PROJECTIONS_GOODE_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/proj/gn_sinu.hpp>
#include <boost/geometry/srs/projections/proj/moll.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace goode
    {

            static const double Y_COR = 0.05280;
            static const double PHI_LIM = .71093078197902358062;

            // TODO: It would be possible to further decrease the size of par_goode
            // because spherical sinu and moll has constant parameters.

            template <typename T, typename Par>
            struct par_goode
            {
                sinu_spheroid<T, Par>    sinu;
                moll_spheroid<T, Par>    moll;
                
                // NOTE: It is ok to share parameters between projections because
                // the only member that is modified in the constructors of
                // spherical sinu and moll projections is es = 0 which is set
                // below in setup_goode() anyway.
                // Moreover in these projections parameters are not used
                // in fwd() nor inv().

                template <typename Params>
                par_goode(Params const& params, Par & par)
                    : sinu(params, par)
                    , moll(params, par)
                {}
            };

            template <typename T, typename Par>
            inline void s_forward(T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y,
                                  Par const& par, par_goode<T, Par> const& proj_par)
            {
                if (fabs(lp_lat) <= PHI_LIM)
                    proj_par.sinu.fwd(par, lp_lon, lp_lat, xy_x, xy_y);
                else {
                    proj_par.moll.fwd(par, lp_lon, lp_lat, xy_x, xy_y);
                    xy_y -= lp_lat >= 0.0 ? Y_COR : -Y_COR;
                }
            }

            template <typename T, typename Par>
            inline void s_inverse(T const& xy_x, T xy_y, T& lp_lon, T& lp_lat,
                                  Par const& par, par_goode<T, Par> const& proj_par)
            {
                if (fabs(xy_y) <= PHI_LIM)
                    proj_par.sinu.inv(par, xy_x, xy_y, lp_lon, lp_lat);
                else {
                    xy_y += xy_y >= 0.0 ? Y_COR : -Y_COR;
                    proj_par.moll.inv(par, xy_x, xy_y, lp_lon, lp_lat);
                }
            }

            // Goode Homolosine
            template <typename Par>
            inline Par& setup_goode(Par& par)
            {
                par.es = 0.;

                // NOTE: The following explicit initialization of sinu projection
                // is not needed because setup_goode() is called before proj_par.sinu
                // is constructed and m_par of parent projection is used.

                //proj_par.sinu.m_par.es = 0.;
                //detail::gn_sinu::setup_sinu(proj_par.sinu.m_par, proj_par.sinu.m_proj_parm);

                return par;
            }

    }} // namespace detail::goode
    #endif // doxygen

    /*!
        \brief Goode Homolosine projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_goode.gif
    */
    template <typename T, typename Parameters>
    struct goode_spheroid
    {
        detail::goode::par_goode<T, Parameters> m_proj_parm;

        template <typename Params>
        inline goode_spheroid(Params const& params, Parameters & par)
            : m_proj_parm(params, detail::goode::setup_goode(par))
        {}

        // FORWARD(s_forward)  spheroid
        // Project coordinates from geographic (lon, lat) to cartesian (x, y)
        inline void fwd(Parameters const& par, T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
        {
            detail::goode::s_forward(lp_lon, lp_lat, xy_x, xy_y, par, this->m_proj_parm);
        }

        // INVERSE(s_inverse)  spheroid
        // Project coordinates from cartesian (x, y) to geographic (lon, lat)
        inline void inv(Parameters const& par, T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
        {
            detail::goode::s_inverse(xy_x, xy_y, lp_lon, lp_lat, par, this->m_proj_parm);
        }

        static inline std::string get_name()
        {
            return "goode_spheroid";
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_goode, goode_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(goode_entry, goode_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(goode_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(goode, goode_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_GOODE_HPP

