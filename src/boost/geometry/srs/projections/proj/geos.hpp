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

// Copyright (c) 2004   Gerald I. Evenden
// Copyright (c) 2012   Martin Raspaud

// See also (section 4.4.3.2):
//   http://www.eumetsat.int/en/area4/msg/news/us_doc/cgms_03_26.pdf

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_GEOS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_GEOS_HPP

#include <boost/math/special_functions/hypot.hpp>

#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail { namespace geos
    {
            template <typename T>
            struct par_geos
            {
                T           h;
                T           radius_p;
                T           radius_p2;
                T           radius_p_inv2;
                T           radius_g;
                T           radius_g_1;
                T           C;
                bool        flip_axis;
            };

            template <typename T, typename Parameters>
            struct base_geos_ellipsoid
            {
                par_geos<T> m_proj_parm;

                // FORWARD(e_forward)  ellipsoid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T lp_lat, T& xy_x, T& xy_y) const
                {
                    T r, Vx, Vy, Vz, tmp;

                    /* Calculation of geocentric latitude. */
                    lp_lat = atan (this->m_proj_parm.radius_p2 * tan (lp_lat));
                
                    /* Calculation of the three components of the vector from satellite to
                    ** position on earth surface (lon,lat).*/
                    r = (this->m_proj_parm.radius_p) / boost::math::hypot(this->m_proj_parm.radius_p * cos (lp_lat), sin (lp_lat));
                    Vx = r * cos (lp_lon) * cos (lp_lat);
                    Vy = r * sin (lp_lon) * cos (lp_lat);
                    Vz = r * sin (lp_lat);

                    /* Check visibility. */
                    if (((this->m_proj_parm.radius_g - Vx) * Vx - Vy * Vy - Vz * Vz * this->m_proj_parm.radius_p_inv2) < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }

                    /* Calculation based on view angles from satellite. */
                    tmp = this->m_proj_parm.radius_g - Vx;

                    if(this->m_proj_parm.flip_axis) {
                        xy_x = this->m_proj_parm.radius_g_1 * atan (Vy / boost::math::hypot (Vz, tmp));
                        xy_y = this->m_proj_parm.radius_g_1 * atan (Vz / tmp);
                    } else {
                        xy_x = this->m_proj_parm.radius_g_1 * atan (Vy / tmp);
                        xy_y = this->m_proj_parm.radius_g_1 * atan (Vz / boost::math::hypot (Vy, tmp));
                    }
                }

                // INVERSE(e_inverse)  ellipsoid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T Vx, Vy, Vz, a, b, det, k;

                    /* Setting three components of vector from satellite to position.*/
                    Vx = -1.0;
                        
                    if(this->m_proj_parm.flip_axis) {
                        Vz = tan (xy_y / this->m_proj_parm.radius_g_1);
                        Vy = tan (xy_x / this->m_proj_parm.radius_g_1) * boost::math::hypot(1.0, Vz);
                    } else {
                        Vy = tan (xy_x / this->m_proj_parm.radius_g_1);
                        Vz = tan (xy_y / this->m_proj_parm.radius_g_1) * boost::math::hypot(1.0, Vy);
                    }

                    /* Calculation of terms in cubic equation and determinant.*/
                    a = Vz / this->m_proj_parm.radius_p;
                    a   = Vy * Vy + a * a + Vx * Vx;
                    b   = 2 * this->m_proj_parm.radius_g * Vx;
                    if ((det = (b * b) - 4 * a * this->m_proj_parm.C) < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }

                    /* Calculation of three components of vector from satellite to position.*/
                    k  = (-b - sqrt(det)) / (2. * a);
                    Vx = this->m_proj_parm.radius_g + k * Vx;
                    Vy *= k;
                    Vz *= k;

                    /* Calculation of longitude and latitude.*/
                    lp_lon = atan2 (Vy, Vx);
                    lp_lat = atan (Vz * cos (lp_lon) / Vx);
                    lp_lat = atan (this->m_proj_parm.radius_p_inv2 * tan (lp_lat));
                }

                static inline std::string get_name()
                {
                    return "geos_ellipsoid";
                }

            };

            template <typename T, typename Parameters>
            struct base_geos_spheroid
            {
                par_geos<T> m_proj_parm;

                // FORWARD(s_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    T Vx, Vy, Vz, tmp;

                    /* Calculation of the three components of the vector from satellite to
                    ** position on earth surface (lon,lat).*/
                    tmp = cos(lp_lat);
                    Vx = cos (lp_lon) * tmp;
                    Vy = sin (lp_lon) * tmp;
                    Vz = sin (lp_lat);

                    /* Check visibility.*/
                    // TODO: in proj4 5.0.0 this check is not present
                    if (((this->m_proj_parm.radius_g - Vx) * Vx - Vy * Vy - Vz * Vz) < 0.)
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );

                    /* Calculation based on view angles from satellite.*/
                    tmp = this->m_proj_parm.radius_g - Vx;

                    if(this->m_proj_parm.flip_axis) {
                        xy_x = this->m_proj_parm.radius_g_1 * atan(Vy / boost::math::hypot(Vz, tmp));
                        xy_y = this->m_proj_parm.radius_g_1 * atan(Vz / tmp);
                    } else {
                        xy_x = this->m_proj_parm.radius_g_1 * atan(Vy / tmp);
                        xy_y = this->m_proj_parm.radius_g_1 * atan(Vz / boost::math::hypot(Vy, tmp));
                    }
                }

                // INVERSE(s_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    T Vx, Vy, Vz, a, b, det, k;

                    /* Setting three components of vector from satellite to position.*/
                    Vx = -1.0;
                    if(this->m_proj_parm.flip_axis) {
                        Vz = tan (xy_y / (this->m_proj_parm.radius_g - 1.0));
                        Vy = tan (xy_x / (this->m_proj_parm.radius_g - 1.0)) * sqrt (1.0 + Vz * Vz);
                    } else {
                        Vy = tan (xy_x / (this->m_proj_parm.radius_g - 1.0));
                        Vz = tan (xy_y / (this->m_proj_parm.radius_g - 1.0)) * sqrt (1.0 + Vy * Vy);
                    }
                    
                    /* Calculation of terms in cubic equation and determinant.*/
                    a   = Vy * Vy + Vz * Vz + Vx * Vx;
                    b   = 2 * this->m_proj_parm.radius_g * Vx;
                    if ((det = (b * b) - 4 * a * this->m_proj_parm.C) < 0.) {
                        BOOST_THROW_EXCEPTION( projection_exception(error_tolerance_condition) );
                    }

                    /* Calculation of three components of vector from satellite to position.*/
                    k  = (-b - sqrt(det)) / (2 * a);
                    Vx = this->m_proj_parm.radius_g + k * Vx;
                    Vy *= k;
                    Vz *= k;

                    /* Calculation of longitude and latitude.*/
                    lp_lon = atan2 (Vy, Vx);
                    lp_lat = atan (Vz * cos (lp_lon) / Vx);
                }

                static inline std::string get_name()
                {
                    return "geos_spheroid";
                }

            };

            inline bool geos_flip_axis(srs::detail::proj4_parameters const& params)
            {
                std::string sweep_axis = pj_get_param_s(params, "sweep");
                if (sweep_axis.empty())
                    return false;
                else {
                    if (sweep_axis[1] != '\0' || (sweep_axis[0] != 'x' && sweep_axis[0] != 'y'))
                        BOOST_THROW_EXCEPTION( projection_exception(error_invalid_sweep_axis) );

                    if (sweep_axis[0] == 'x')
                        return true;
                    else
                        return false;
                }
            }

            template <typename T>
            inline bool geos_flip_axis(srs::dpar::parameters<T> const& params)
            {
                typename srs::dpar::parameters<T>::const_iterator
                    it = pj_param_find(params, srs::dpar::sweep);
                if (it == params.end()) {
                    return false;
                } else {
                    srs::dpar::value_sweep s = static_cast<srs::dpar::value_sweep>(it->template get_value<int>());
                    return s == srs::dpar::sweep_x;
                }
            }

            // Geostationary Satellite View
            template <typename Params, typename Parameters, typename T>
            inline void setup_geos(Params const& params, Parameters& par, par_geos<T>& proj_parm)
            {
                std::string sweep_axis;

                if ((proj_parm.h = pj_get_param_f<T, srs::spar::h>(params, "h", srs::dpar::h)) <= 0.)
                    BOOST_THROW_EXCEPTION( projection_exception(error_h_less_than_zero) );

                if (par.phi0 != 0.0)
                    BOOST_THROW_EXCEPTION( projection_exception(error_unknown_prime_meridian) );

                
                proj_parm.flip_axis = geos_flip_axis(params);

                proj_parm.radius_g_1 = proj_parm.h / par.a;
                proj_parm.radius_g = 1. + proj_parm.radius_g_1;
                proj_parm.C  = proj_parm.radius_g * proj_parm.radius_g - 1.0;
                if (par.es != 0.0) {
                    proj_parm.radius_p      = sqrt (par.one_es);
                    proj_parm.radius_p2     = par.one_es;
                    proj_parm.radius_p_inv2 = par.rone_es;
                } else {
                    proj_parm.radius_p = proj_parm.radius_p2 = proj_parm.radius_p_inv2 = 1.0;
                }
            }

    }} // namespace detail::geos
    #endif // doxygen

    /*!
        \brief Geostationary Satellite View projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - h: Height (real)
         - sweep: Sweep axis ('x' or 'y') (string)
        \par Example
        \image html ex_geos.gif
    */
    template <typename T, typename Parameters>
    struct geos_ellipsoid : public detail::geos::base_geos_ellipsoid<T, Parameters>
    {
        template <typename Params>
        inline geos_ellipsoid(Params const& params, Parameters const& par)
        {
            detail::geos::setup_geos(params, par, this->m_proj_parm);
        }
    };

    /*!
        \brief Geostationary Satellite View projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Azimuthal
         - Spheroid
         - Ellipsoid
        \par Projection parameters
         - h: Height (real)
         - sweep: Sweep axis ('x' or 'y') (string)
        \par Example
        \image html ex_geos.gif
    */
    template <typename T, typename Parameters>
    struct geos_spheroid : public detail::geos::base_geos_spheroid<T, Parameters>
    {
        template <typename Params>
        inline geos_spheroid(Params const& params, Parameters const& par)
        {
            detail::geos::setup_geos(params, par, this->m_proj_parm);
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_STATIC_PROJECTION_FI2(srs::spar::proj_geos, geos_spheroid, geos_ellipsoid)

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_FI2(geos_entry, geos_spheroid, geos_ellipsoid)
        
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(geos_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(geos, geos_entry);
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_GEOS_HPP

