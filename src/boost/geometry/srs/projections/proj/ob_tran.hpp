// Boost.Geometry - gis-projections (based on PROJ4)

// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_OB_TRAN_HPP
#define BOOST_GEOMETRY_PROJECTIONS_OB_TRAN_HPP

#include <type_traits>

#include <boost/geometry/util/math.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/srs/projections/impl/aasincos.hpp>
#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/impl/pj_ell_set.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry
{

namespace projections
{
    #ifndef DOXYGEN_NO_DETAIL
    namespace detail {
    
        // fwd declaration needed below
        template <typename T>
        inline detail::dynamic_wrapper_b<T, projections::parameters<T> >*
            create_new(srs::detail::proj4_parameters const& params,
                       projections::parameters<T> const& parameters);

        template <typename T>
        inline detail::dynamic_wrapper_b<T, projections::parameters<T> >*
            create_new(srs::dpar::parameters<T> const& params,
                       projections::parameters<T> const& parameters);

    } // namespace detail

    namespace detail { namespace ob_tran
    {

            static const double tolerance = 1e-10;

            template <typename Parameters>
            inline Parameters o_proj_parameters(srs::detail::proj4_parameters const& params,
                                                Parameters const& par)
            {
                /* copy existing header into new */
                Parameters pj = par;

                /* get name of projection to be translated */
                pj.id = pj_get_param_s(params, "o_proj");
                if (pj.id.is_unknown())
                    BOOST_THROW_EXCEPTION( projection_exception(error_no_rotation_proj) );

                /* avoid endless recursion */
                if( pj.id.name == "ob_tran")
                    BOOST_THROW_EXCEPTION( projection_exception(error_failed_to_find_proj) );

                // Commented out for consistency with Proj4 >= 5.0.0
                /* force spherical earth */
                //pj.one_es = pj.rone_es = 1.;
                //pj.es = pj.e = 0.;

                return pj;
            }

            template <typename T, typename Parameters>
            inline Parameters o_proj_parameters(srs::dpar::parameters<T> const& params,
                                                Parameters const& par)
            {
                /* copy existing header into new */
                Parameters pj = par;

                /* get name of projection to be translated */
                typename srs::dpar::parameters<T>::const_iterator
                    it = pj_param_find(params, srs::dpar::o_proj);
                if (it != params.end())
                    pj.id = static_cast<srs::dpar::value_proj>(it->template get_value<int>());
                else
                    BOOST_THROW_EXCEPTION( projection_exception(error_no_rotation_proj) );

                /* avoid endless recursion */
                if( pj.id.id == srs::dpar::proj_ob_tran)
                    BOOST_THROW_EXCEPTION( projection_exception(error_failed_to_find_proj) );

                // Commented out for consistency with Proj4 >= 5.0.0
                /* force spherical earth */
                //pj.one_es = pj.rone_es = 1.;
                //pj.es = pj.e = 0.;

                return pj;
            }

            template <typename ...Ps, typename Parameters>
            inline Parameters o_proj_parameters(srs::spar::parameters<Ps...> const& /*params*/,
                                                Parameters const& par)
            {
                /* copy existing header into new */
                Parameters pj = par;

                /* get name of projection to be translated */
                typedef srs::spar::parameters<Ps...> params_type;
                typedef typename geometry::tuples::find_if
                    <
                        params_type,
                        srs::spar::detail::is_param_t<srs::spar::o_proj>::pred
                    >::type o_proj_type;

                static const bool is_found = geometry::tuples::is_found<o_proj_type>::value;
                BOOST_GEOMETRY_STATIC_ASSERT((is_found),
                    "Rotation projection not specified.",
                    params_type);

                typedef typename o_proj_type::type proj_type;
                static const bool is_specialized = srs::spar::detail::proj_traits<proj_type>::is_specialized;
                BOOST_GEOMETRY_STATIC_ASSERT((is_specialized),
                    "Rotation projection not specified.",
                    params_type);

                pj.id = srs::spar::detail::proj_traits<proj_type>::id;

                /* avoid endless recursion */
                static const bool is_non_resursive = ! std::is_same<proj_type, srs::spar::proj_ob_tran>::value;
                BOOST_GEOMETRY_STATIC_ASSERT((is_non_resursive),
                    "o_proj parameter can not be set to ob_tran projection.",
                    params_type);

                // Commented out for consistency with Proj4 >= 5.0.0
                /* force spherical earth */
                //pj.one_es = pj.rone_es = 1.;
                //pj.es = pj.e = 0.;

                return pj;
            }

            // TODO: It's possible that the original Parameters could be used
            // instead of a copy in link.
            // But it's not possible with the current implementation of
            // dynamic_wrapper_b always storing params

            template <typename T, typename Parameters>
            struct par_ob_tran
            {
                template <typename Params>
                par_ob_tran(Params const& params, Parameters const& par)
                    : link(projections::detail::create_new(params, o_proj_parameters(params, par)))
                {
                    if (! link.get())
                        BOOST_THROW_EXCEPTION( projection_exception(error_unknown_projection_id) );
                }

                inline void fwd(T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    link->fwd(link->params(), lp_lon, lp_lat, xy_x, xy_y);
                }

                inline void inv(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    link->inv(link->params(), xy_x, xy_y, lp_lon, lp_lat);
                }

                boost::shared_ptr<dynamic_wrapper_b<T, Parameters> > link;
                T lamp;
                T cphip, sphip;
            };

            template <typename StaticParameters, typename T, typename Parameters>
            struct par_ob_tran_static
            {
                // this metafunction handles static error handling
                typedef typename srs::spar::detail::pick_o_proj_tag
                    <
                        StaticParameters
                    >::type o_proj_tag;

                /* avoid endless recursion */
                static const bool is_o_proj_not_ob_tran = ! std::is_same<o_proj_tag, srs::spar::proj_ob_tran>::value;
                BOOST_GEOMETRY_STATIC_ASSERT((is_o_proj_not_ob_tran),
                    "o_proj parameter can not be set to ob_tran projection.",
                    StaticParameters);

                typedef typename projections::detail::static_projection_type
                    <
                        o_proj_tag,
                        // Commented out for consistency with Proj4 >= 5.0.0
                        //srs_sphere_tag, // force spherical
                        typename projections::detail::static_srs_tag<StaticParameters>::type,
                        StaticParameters,
                        T,
                        Parameters
                    >::type projection_type;

                par_ob_tran_static(StaticParameters const& params, Parameters const& par)
                    : link(params, o_proj_parameters(params, par))
                {}

                inline void fwd(T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    link.fwd(link.params(), lp_lon, lp_lat, xy_x, xy_y);
                }

                inline void inv(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    link.inv(link.params(), xy_x, xy_y, lp_lon, lp_lat);
                }

                projection_type link;
                T lamp;
                T cphip, sphip;
            };

            template <typename T, typename Par>
            inline void o_forward(T lp_lon, T lp_lat, T& xy_x, T& xy_y, Par const& proj_parm)
            {
                T coslam, sinphi, cosphi;
                
                coslam = cos(lp_lon);
                sinphi = sin(lp_lat);
                cosphi = cos(lp_lat);
                lp_lon = adjlon(aatan2(cosphi * sin(lp_lon), proj_parm.sphip * cosphi * coslam +
                    proj_parm.cphip * sinphi) + proj_parm.lamp);
                lp_lat = aasin(proj_parm.sphip * sinphi - proj_parm.cphip * cosphi * coslam);

                proj_parm.fwd(lp_lon, lp_lat, xy_x, xy_y);
            }

            template <typename T, typename Par>
            inline void o_inverse(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat, Par const& proj_parm)
            {
                T coslam, sinphi, cosphi;

                proj_parm.inv(xy_x, xy_y, lp_lon, lp_lat);
                if (lp_lon != HUGE_VAL) {
                    coslam = cos(lp_lon -= proj_parm.lamp);
                    sinphi = sin(lp_lat);
                    cosphi = cos(lp_lat);
                    lp_lat = aasin(proj_parm.sphip * sinphi + proj_parm.cphip * cosphi * coslam);
                    lp_lon = aatan2(cosphi * sin(lp_lon), proj_parm.sphip * cosphi * coslam -
                        proj_parm.cphip * sinphi);
                }
            }

            template <typename T, typename Par>
            inline void t_forward(T lp_lon, T lp_lat, T& xy_x, T& xy_y, Par const& proj_parm)
            {
                T cosphi, coslam;

                cosphi = cos(lp_lat);
                coslam = cos(lp_lon);
                lp_lon = adjlon(aatan2(cosphi * sin(lp_lon), sin(lp_lat)) + proj_parm.lamp);
                lp_lat = aasin(- cosphi * coslam);

                proj_parm.fwd(lp_lon, lp_lat, xy_x, xy_y);
            }

            template <typename T, typename Par>
            inline void t_inverse(T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat, Par const& proj_parm)
            {
                T cosphi, t;

                proj_parm.inv(xy_x, xy_y, lp_lon, lp_lat);
                if (lp_lon != HUGE_VAL) {
                    cosphi = cos(lp_lat);
                    t = lp_lon - proj_parm.lamp;
                    lp_lon = aatan2(cosphi * sin(t), - sin(lp_lat));
                    lp_lat = aasin(cosphi * cos(t));
                }
            }

            // General Oblique Transformation
            template <typename T, typename Params, typename Parameters, typename ProjParameters>
            inline T setup_ob_tran(Params const& params, Parameters & /*par*/, ProjParameters& proj_parm)
            {
                static const T half_pi = detail::half_pi<T>();

                T phip, alpha;

                // Commented out for consistency with Proj4 >= 5.0.0
                //par.es = 0.; /* force to spherical */

                // proj_parm.link should be created at this point

                if (pj_param_r<srs::spar::o_alpha>(params, "o_alpha", srs::dpar::o_alpha, alpha)) {
                    T lamc, phic;

                    lamc    = pj_get_param_r<T, srs::spar::o_lon_c>(params, "o_lon_c", srs::dpar::o_lon_c);
                    phic    = pj_get_param_r<T, srs::spar::o_lon_c>(params, "o_lat_c", srs::dpar::o_lat_c);
                    //alpha   = pj_get_param_r(par.params, "o_alpha");
            
                    if (fabs(fabs(phic) - half_pi) <= tolerance)
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_0_or_alpha_eq_90) );

                    proj_parm.lamp = lamc + aatan2(-cos(alpha), -sin(alpha) * sin(phic));
                    phip = aasin(cos(phic) * sin(alpha));
                } else if (pj_param_r<srs::spar::o_lat_p>(params, "o_lat_p", srs::dpar::o_lat_p, phip)) { /* specified new pole */
                    proj_parm.lamp = pj_get_param_r<T, srs::spar::o_lon_p>(params, "o_lon_p", srs::dpar::o_lon_p);
                    //phip = pj_param_r(par.params, "o_lat_p");
                } else { /* specified new "equator" points */
                    T lam1, lam2, phi1, phi2, con;

                    lam1 = pj_get_param_r<T, srs::spar::o_lon_1>(params, "o_lon_1", srs::dpar::o_lon_1);
                    phi1 = pj_get_param_r<T, srs::spar::o_lat_1>(params, "o_lat_1", srs::dpar::o_lat_1);
                    lam2 = pj_get_param_r<T, srs::spar::o_lon_2>(params, "o_lon_2", srs::dpar::o_lon_2);
                    phi2 = pj_get_param_r<T, srs::spar::o_lat_2>(params, "o_lat_2", srs::dpar::o_lat_2);
                    if (fabs(phi1 - phi2) <= tolerance || (con = fabs(phi1)) <= tolerance ||
                        fabs(con - half_pi) <= tolerance || fabs(fabs(phi2) - half_pi) <= tolerance)
                        BOOST_THROW_EXCEPTION( projection_exception(error_lat_1_or_2_zero_or_90) );

                    proj_parm.lamp = atan2(cos(phi1) * sin(phi2) * cos(lam1) -
                        sin(phi1) * cos(phi2) * cos(lam2),
                        sin(phi1) * cos(phi2) * sin(lam2) -
                        cos(phi1) * sin(phi2) * sin(lam1));
                    phip = atan(-cos(proj_parm.lamp - lam1) / tan(phi1));
                }

                if (fabs(phip) > tolerance) { /* oblique */
                    proj_parm.cphip = cos(phip);
                    proj_parm.sphip = sin(phip);
                } else { /* transverse */
                }

                // TODO:
                /* Support some rather speculative test cases, where the rotated projection */
                /* is actually latlong. We do not want scaling in that case... */
                //if (proj_parm.link...mutable_parameters().right==PJ_IO_UNITS_ANGULAR)
                //    par.right = PJ_IO_UNITS_PROJECTED;

                // return phip to choose model
                return phip;
            }

            template <typename T, typename Parameters>
            struct base_ob_tran_oblique
            {
                par_ob_tran<T, Parameters> m_proj_parm;

                inline base_ob_tran_oblique(par_ob_tran<T, Parameters> const& proj_parm)
                    : m_proj_parm(proj_parm)
                {}

                // FORWARD(o_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    o_forward(lp_lon, lp_lat, xy_x, xy_y, this->m_proj_parm);
                }

                // INVERSE(o_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    o_inverse(xy_x, xy_y, lp_lon, lp_lat, this->m_proj_parm);
                }

                static inline std::string get_name()
                {
                    return "ob_tran_oblique";
                }

            };

            template <typename T, typename Parameters>
            struct base_ob_tran_transverse
            {
                par_ob_tran<T, Parameters> m_proj_parm;

                inline base_ob_tran_transverse(par_ob_tran<T, Parameters> const& proj_parm)
                    : m_proj_parm(proj_parm)
                {}

                // FORWARD(t_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    t_forward(lp_lon, lp_lat, xy_x, xy_y, this->m_proj_parm);
                }

                // INVERSE(t_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    t_inverse(xy_x, xy_y, lp_lon, lp_lat, this->m_proj_parm);
                }

                static inline std::string get_name()
                {
                    return "ob_tran_transverse";
                }

            };

            template <typename StaticParameters, typename T, typename Parameters>
            struct base_ob_tran_static
            {
                par_ob_tran_static<StaticParameters, T, Parameters> m_proj_parm;
                bool m_is_oblique;

                inline base_ob_tran_static(StaticParameters const& params, Parameters const& par)
                    : m_proj_parm(params, par)
                {}

                // FORWARD(o_forward)  spheroid
                // Project coordinates from geographic (lon, lat) to cartesian (x, y)
                inline void fwd(Parameters const& , T const& lp_lon, T const& lp_lat, T& xy_x, T& xy_y) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    if (m_is_oblique) {
                        o_forward(lp_lon, lp_lat, xy_x, xy_y, this->m_proj_parm);
                    } else {
                        t_forward(lp_lon, lp_lat, xy_x, xy_y, this->m_proj_parm);
                    }
                }

                // INVERSE(o_inverse)  spheroid
                // Project coordinates from cartesian (x, y) to geographic (lon, lat)
                inline void inv(Parameters const& , T const& xy_x, T const& xy_y, T& lp_lon, T& lp_lat) const
                {
                    // NOTE: Parameters ignored, m_proj_parm.link has a copy
                    if (m_is_oblique) {
                        o_inverse(xy_x, xy_y, lp_lon, lp_lat, this->m_proj_parm);
                    } else {
                        t_inverse(xy_x, xy_y, lp_lon, lp_lat, this->m_proj_parm);
                    }
                }

                static inline std::string get_name()
                {
                    return "ob_tran";
                }

            };

    }} // namespace detail::ob_tran
    #endif // doxygen

    /*!
        \brief General Oblique Transformation projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - o_proj (string)
         - Plus projection parameters
         - o_lat_p (degrees)
         - o_lon_p (degrees)
         - New pole
         - o_alpha: Alpha (degrees)
         - o_lon_c (degrees)
         - o_lat_c (degrees)
         - o_lon_1 (degrees)
         - o_lat_1: Latitude of first standard parallel (degrees)
         - o_lon_2 (degrees)
         - o_lat_2: Latitude of second standard parallel (degrees)
        \par Example
        \image html ex_ob_tran.gif
    */
    template <typename T, typename Parameters>
    struct ob_tran_oblique : public detail::ob_tran::base_ob_tran_oblique<T, Parameters>
    {
        template <typename Params>
        inline ob_tran_oblique(Params const& , Parameters const& ,
                               detail::ob_tran::par_ob_tran<T, Parameters> const& proj_parm)
            : detail::ob_tran::base_ob_tran_oblique<T, Parameters>(proj_parm)
        {
            // already done
            //detail::ob_tran::setup_ob_tran(this->m_par, this->m_proj_parm);
        }
    };

    /*!
        \brief General Oblique Transformation projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - o_proj (string)
         - Plus projection parameters
         - o_lat_p (degrees)
         - o_lon_p (degrees)
         - New pole
         - o_alpha: Alpha (degrees)
         - o_lon_c (degrees)
         - o_lat_c (degrees)
         - o_lon_1 (degrees)
         - o_lat_1: Latitude of first standard parallel (degrees)
         - o_lon_2 (degrees)
         - o_lat_2: Latitude of second standard parallel (degrees)
        \par Example
        \image html ex_ob_tran.gif
    */
    template <typename T, typename Parameters>
    struct ob_tran_transverse : public detail::ob_tran::base_ob_tran_transverse<T, Parameters>
    {        
        template <typename Params>
        inline ob_tran_transverse(Params const& , Parameters const& ,
                                  detail::ob_tran::par_ob_tran<T, Parameters> const& proj_parm)
            : detail::ob_tran::base_ob_tran_transverse<T, Parameters>(proj_parm)
        {
            // already done
            //detail::ob_tran::setup_ob_tran(this->m_par, this->m_proj_parm);
        }
    };

    /*!
        \brief General Oblique Transformation projection
        \ingroup projections
        \tparam Geographic latlong point type
        \tparam Cartesian xy point type
        \tparam Parameters parameter type
        \par Projection characteristics
         - Miscellaneous
         - Spheroid
        \par Projection parameters
         - o_proj (string)
         - Plus projection parameters
         - o_lat_p (degrees)
         - o_lon_p (degrees)
         - New pole
         - o_alpha: Alpha (degrees)
         - o_lon_c (degrees)
         - o_lat_c (degrees)
         - o_lon_1 (degrees)
         - o_lat_1: Latitude of first standard parallel (degrees)
         - o_lon_2 (degrees)
         - o_lat_2: Latitude of second standard parallel (degrees)
        \par Example
        \image html ex_ob_tran.gif
    */
    template <typename StaticParameters, typename T, typename Parameters>
    struct ob_tran_static : public detail::ob_tran::base_ob_tran_static<StaticParameters, T, Parameters>
    {
        inline ob_tran_static(StaticParameters const& params, Parameters const& par)
            : detail::ob_tran::base_ob_tran_static<StaticParameters, T, Parameters>(params, par)
        {
            T phip = detail::ob_tran::setup_ob_tran<T>(params, par, this->m_proj_parm);
            this->m_is_oblique = fabs(phip) > detail::ob_tran::tolerance;
        }
    };

    #ifndef DOXYGEN_NO_DETAIL
    namespace detail
    {

        // Static projection
        template <typename SP, typename CT, typename P>
        struct static_projection_type<srs::spar::proj_ob_tran, srs_sphere_tag, SP, CT, P>
        {
            typedef static_wrapper_fi<ob_tran_static<SP, CT, P>, P> type;
        };
        template <typename SP, typename CT, typename P>
        struct static_projection_type<srs::spar::proj_ob_tran, srs_spheroid_tag, SP, CT, P>
        {
            typedef static_wrapper_fi<ob_tran_static<SP, CT, P>, P> type;
        };

        // Factory entry(s)
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_BEGIN(ob_tran_entry)
        {
            Parameters parameters_copy = parameters;
            detail::ob_tran::par_ob_tran<T, Parameters> proj_parm(params, parameters_copy);
            T phip = detail::ob_tran::setup_ob_tran<T>(params, parameters_copy, proj_parm);

            if (fabs(phip) > detail::ob_tran::tolerance)
                return new dynamic_wrapper_fi<ob_tran_oblique<T, Parameters>, T, Parameters>(params, parameters_copy, proj_parm);
            else
                return new dynamic_wrapper_fi<ob_tran_transverse<T, Parameters>, T, Parameters>(params, parameters_copy, proj_parm);
        }
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_ENTRY_END

        BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_BEGIN(ob_tran_init)
        {
            BOOST_GEOMETRY_PROJECTIONS_DETAIL_FACTORY_INIT_ENTRY(ob_tran, ob_tran_entry)
        }

    } // namespace detail
    #endif // doxygen

} // namespace projections

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_PROJECTIONS_OB_TRAN_HPP

