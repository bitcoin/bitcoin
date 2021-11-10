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

#ifndef BOOST_GEOMETRY_PROJECTIONS_STS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_STS_HPP

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
    namespace detail { namespace sts
    {
            template <typename T>
            struct par_sts
            {
                T C_x, C_y, C_p;
                bool tan_mode;
            };

            template <typename T, typename Parameters>
            struct base_sts_spheroid
            {
                par_sts<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    T c;

                    xy_x = this->m_proj_parm.C_x * lp_lon * cos(lp_lat);
                    xy_y = this->m_proj_parm.C_y;
                    lp_lat *= this->m_proj_parm.C_p;
                    c = cos(lp_lat);
                    if (this->m_proj_parm.tan_mode) {
                        xy_x *= c * c;
                        xy_y *= tan(lp_lat);
                    } else {
                        xy_x /= c;
                        xy_y *= sin(lp_lat);
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T c;

                    xy_y /= this->m_proj_parm.C_y;
                    c = cos(lp_lat = this->m_proj_parm.tan_mode ? atan(xy_y) : aasin(xy_y));
                    lp_lat /= this->m_proj_parm.C_p;
                    lp_lon = xy_x / (this->m_proj_parm.C_x * cos(lp_lat));
                    if (this->m_proj_parm.tan_mode)
                        lp_lon /= c * c;
                    else
                        lp_lon *= c;
                }

                static inline std::string get_name()
                {
                    return "sts_spheroid";
                }

            };

            template <typename Parameters, typename T>
            inline void setup(Parameters& par, par_sts<T>& proj_parm, T const& p, T const& q, bool mode) 
            {
                par.es = 0.;
                proj_parm.C_x = q / p;
                proj_parm.C_y = p;
                proj_parm.C_p = 1/ q;
                proj_parm.tan_mode = mode;
            }


            // Foucaut
            template <typename Parameters, typename T>
            inline void setup_fouc(Parameters& par, par_sts<T>& proj_parm)
            {
                setup(par, proj_parm, 2., 2., true);
            }

            // Kavraisky V
            template <typename Parameters, typename T>
            inline void setup_kav5(Parameters& par, par_sts<T>& proj_parm)
            {
                setup(par, proj_parm, 1.50488, 1.35439, false);
            }

            // Quartic Authalic
            template <typename Parameters, typename T>
            inline void setup_qua_aut(Parameters& par, par_sts<T>& proj_parm)
            {
                setup(par, proj_parm, 2., 2., false);
            }

            // McBryde-Thomas Flat-Polar Sine (No. 1)
            template <typename Parameters, typename T>
            inline void setup_mbt_s(Parameters& par, par_sts<T>& proj_parm)
            {
                setup(par, proj_parm, 1.48875, 1.36509, false);
            }

    }} // namespace detail::sts
    #endif // doxygen

    /*!
        \brief Kavraisky V projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_kav5.gif
    */
    template <typename T, typename Parameters>
    struct kav5_spheroid : public detail::sts::base_sts_spheroid<T, Parameters>
    {
        template <typename Params>
        inline kav5_spheroid(Params const& , Parameters & par)
        {
            detail::sts::setup_kav5(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Quartic Authalic projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_qua_aut.gif
    */
    template <typename T, typename Parameters>
    struct qua_aut_spheroid : public detail::sts::base_sts_spheroid<T, Parameters>
    {
        template <typename Params>
        inline qua_aut_spheroid(Params const& , Parameters & par)
        {
            detail::sts::setup_qua_aut(par, this->m_proj_parm);
        }
    };

    /*!
        \brief McBryde-Thomas Flat-Polar Sine (No. 1) projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_mbt_s.gif
    */
    template <typename T, typename Parameters>
    struct mbt_s_spheroid : public detail::sts::base_sts_spheroid<T, Parameters>
    {
        template <typename Params>
        inline mbt_s_spheroid(Params const& , Parameters & par)
        {
            detail::sts::setup_mbt_s(par, this->m_proj_parm);
        }
    };

    /*!
        \brief Foucaut projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Pseudocylindrical
         - Spheroid
        \par Example
        \image html ex_fouc.gif
    */
    template <typename T, typename Parameters>
    struct fouc_spheroid : public detail::sts::base_sts_spheroid<T, Parameters>
    {
        template <typename Params>
        inline fouc_spheroid(Params const& , Parameters & par)
        {
            detail::sts::setup_fouc(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_kav5, kav5_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_qua_aut, qua_aut_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_mbt_s, mbt_s_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_fouc, fouc_spheroid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(kav5_entry, kav5_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(qua_aut_entry, qua_aut_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(mbt_s_entry, mbt_s_spheroid)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(fouc_entry, fouc_spheroid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(sts_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(kav5, kav5_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(qua_aut, qua_aut_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(mbt_s, mbt_s_entry)
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(fouc, fouc_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_STS_HPP

