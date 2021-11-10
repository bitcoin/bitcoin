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

/*****************************************************************************

               Lambert Conformal Conic Alternative
               -----------------------------------

    This is Gerald Evenden's 2003 implementation of an alternative
    "almost" LCC, which has been in use historically, but which
    should NOT be used for new projects - i.e: use this implementation
    if you need interoperability with old data represented in this
    projection, but not in any other case.

    The code was originally discussed on the PROJ.4 mailing list in
    a thread archived over at

    http://lists.maptools.org/pipermail/proj/2003-March/000644.html

    It was discussed again in the thread starting at

    http://lists.maptools.org/pipermail/proj/2017-October/007828.html
        and continuing at
    http://lists.maptools.org/pipermail/proj/2017-November/007831.html

    which prompted Clifford J. Mugnier to add these clarifying notes:

    The French Army Truncated Cubic Lambert (partially conformal) Conic
    projection is the Legal system for the projection in France between
    the late 1800s and 1948 when the French Legislature changed the law
    to recognize the fully conformal version.

    It was (might still be in one or two North African prior French
    Colonies) used in North Africa in Algeria, Tunisia, & Morocco, as
    well as in Syria during the Levant.

    Last time I have seen it used was about 30+ years ago in
    Algeria when it was used to define Lease Block boundaries for
    Petroleum Exploration & Production.

    (signed)

    Clifford J. Mugnier, c.p., c.m.s.
    Chief of Geodesy
    LSU Center for GeoInformatics
    Dept. of Civil Engineering
    LOUISIANA STATE UNIVERSITY

*****************************************************************************/

#ifndef BOOST_GEOMETRY_PROJECTIONS_LCCA_HPP
#define BOOST_GEOMETRY_PROJECTIONS_LCCA_HPP

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_mlfn.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace lcca
    {

            static const int max_iter = 10;
            static const double del_tol = 1e-12;

            template <typename T>
            struct par_lcca
            {
                detail::en<T> en;
                T    r0, l, M0;
                T    C;
            };

            template <typename T> /* func to compute dr */
            inline T fS(T const& S, T const& C)
            {
                return(S * ( 1. + S * S * C));
            }

            template <typename T> /* deriv of fs */
            inline T fSp(T const& S, T const& C)
            {
                return(1. + 3.* S * S * C);
            }

            template <typename T, typename Parameters>
            struct base_lcca_ellipsoid
            {
                par_lcca<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& par, T lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T S, r, dr;

                    S = pj_mlfn(lp_lat, sin(lp_lat), cos(lp_lat), this->m_proj_parm.en) - this->m_proj_parm.M0;
                    dr = fS(S, this->m_proj_parm.C);
                    r = this->m_proj_parm.r0 - dr;
                    xy_x = par.k0 * (r * sin( lp_lon *= this->m_proj_parm.l ) );
                    xy_y = par.k0 * (this->m_proj_parm.r0 - r * cos(lp_lon) );
                }

                // INVERSE(e_inverse)  ellipsoid & spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& par, T xy_x, T xy_y, T& lp_lon, T& lp_lat) const
                {
                    T theta, dr, S, dif;
                    int i;

                    xy_x /= par.k0;
                    xy_y /= par.k0;
                    theta = atan2(xy_x , this->m_proj_parm.r0 - xy_y);
                    dr = xy_y - xy_x * tan(0.5 * theta);
                    lp_lon = theta / this->m_proj_parm.l;
                    S = dr;
                    for (i = max_iter; i ; --i) {
                        S -= (dif = (fS(S, this->m_proj_parm.C) - dr) / fSp(S, this->m_proj_parm.C));
                        if (fabs(dif) < del_tol) break;
                    }
                    if (!i) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }
                    lp_lat = pj_inv_mlfn(S + this->m_proj_parm.M0, par.es, this->m_proj_parm.en);
                }

                static inline std::string get_name()
                {
                    return "lcca_ellipsoid";
                }

            };

            // Lambert Conformal Conic Alternative
            template <typename Parameters, typename T>
            inline void setup_lcca(Parameters const& par, par_lcca<T>& proj_parm)
            {
                T s2p0, N0, R0, tan0;

                proj_parm.en = pj_enfn<T>(par.es);
                
                if (par.phi0 == 0.) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_lat_0_is_zero) );
                }
                proj_parm.l = sin(par.phi0);
                proj_parm.M0 = pj_mlfn(par.phi0, proj_parm.l, cos(par.phi0), proj_parm.en);
                s2p0 = proj_parm.l * proj_parm.l;
                R0 = 1. / (1. - par.es * s2p0);
                N0 = sqrt(R0);
                R0 *= par.one_es * N0;
                tan0 = tan(par.phi0);
                proj_parm.r0 = N0 / tan0;
                proj_parm.C = 1. / (6. * R0 * N0);
            }

    }} // namespace detail::lcca
    #endif // doxygen

    /*!
        \brief Lambert Conformal Conic Alternative projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Conic
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - lat_0: Latitude of origin
        \par Example
        \image html ex_lcca.gif
    */
    template <typename T, typename Parameters>
    struct lcca_ellipsoid : public detail::lcca::base_lcca_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline lcca_ellipsoid(Params const& , Parameters const& par)
        {
            detail::lcca::setup_lcca(par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI(srs::spar::proj_lcca, lcca_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI(lcca_entry, lcca_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(lcca_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(lcca, lcca_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_LCCA_HPP

