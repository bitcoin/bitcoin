// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Geometry Library by
//     Barend Gehrels (Geodan, Amsterdam)
//     Adam Wulkiewicz

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP

#include <string>
#include <type_traits>
#include <vector>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/formulas/eccentricity_sqr.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/srs/projections/dpar.hpp>
#include <boost/geometry/srs/projections/impl/pj_datum_set.hpp>
#include <boost/geometry/srs/projections/impl/pj_ellps.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>
#include <boost/geometry/srs/projections/spar.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail {

/* set ellipsoid parameters a and es */
template <typename T>
inline T SIXTH() { return .1666666666666666667; } /* 1/6 */
template <typename T>
inline T RA4() { return .04722222222222222222; } /* 17/360 */
template <typename T>
inline T RA6() { return .02215608465608465608; } /* 67/3024 */
template <typename T>
inline T RV4() { return .06944444444444444444; } /* 5/72 */
template <typename T>
inline T RV6() { return .04243827160493827160; } /* 55/1296 */

template <typename T>
inline T pj_ell_b_to_es(T const& a, T const& b)
{
    return 1. - (b * b) / (a * a);
}

/************************************************************************/
/*                          pj_ell_init_ellps()                         */
/************************************************************************/

// Originally a part of pj_ell_set()
template <typename T>
inline bool pj_ell_init_ellps(srs::detail::proj4_parameters const& params, T &a, T &b)
{
    /* check if ellps present and temporarily append its values to pl */
    std::string name = pj_get_param_s(params, "ellps");
    if (! name.empty())
    {
        const pj_ellps_type<T>* pj_ellps = pj_get_ellps<T>().first;
        const int n = pj_get_ellps<T>().second;
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_ellps[i].id == name)
            {
                index = i;
            }
        }

        if (index == -1) {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
        }

        pj_ellps_type<T> const& pj_ellp = pj_ellps[index];
        a = pj_ellp.a;
        b = pj_ellp.b;

        return true;
    }

    return false;
}

template <typename T>
inline bool pj_ell_init_ellps(srs::dpar::parameters<T> const& params, T &a, T &b)
{
    /* check if ellps present and temporarily append its values to pl */
    typename srs::dpar::parameters<T>::const_iterator
        it = pj_param_find(params, srs::dpar::ellps);
    if (it != params.end())
    {
        if (it->template is_value_set<int>())
        {
            const pj_ellps_type<T>* pj_ellps = pj_get_ellps<T>().first;
            const int n = pj_get_ellps<T>().second;
            int i = it->template get_value<int>();
        
            if (i < 0 || i >= n) {
                BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
            }

            pj_ellps_type<T> const& pj_ellp = pj_ellps[i];
            a = pj_ellp.a;
            b = pj_ellp.b;
        }
        else if (it->template is_value_set<T>())
        {
            a = it->template get_value<T>();
            b = a;
        }
        else if (it->template is_value_set<srs::spheroid<T> >())
        {
            srs::spheroid<T> const& s = it->template get_value<srs::spheroid<T> >();
            a = geometry::get_radius<0>(s);
            b = geometry::get_radius<2>(s);
        }
        else
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
        }

        return true;
    }

    return false;
}

template
<
    typename Params,
    int I = geometry::tuples::find_index_if
        <
            Params,
            srs::spar::detail::is_param_tr<srs::spar::detail::ellps_traits>::pred
        >::value,
    int N = geometry::tuples::size<Params>::value
>
struct pj_ell_init_ellps_static
{
    template <typename T>
    static bool apply(Params const& params, T &a, T &b)
    {
        typedef typename geometry::tuples::element<I, Params>::type param_type;
        typedef srs::spar::detail::ellps_traits<param_type> traits_type;
        typedef typename traits_type::template model_type<T>::type model_type;

        param_type const& param = geometry::tuples::get<I>(params);
        model_type const& model = traits_type::template model<T>(param);

        a = geometry::get_radius<0>(model);
        b = geometry::get_radius<2>(model);

        return true;
    }
};
template <typename Params, int N>
struct pj_ell_init_ellps_static<Params, N, N>
{
    template <typename T>
    static bool apply(Params const& , T & , T & )
    {
        return false;
    }
};

template <typename T, typename ...Ps>
inline bool pj_ell_init_ellps(srs::spar::parameters<Ps...> const& params,
                              T &a, T &b)
{
    return pj_ell_init_ellps_static
        <
            srs::spar::parameters<Ps...>
        >::apply(params, a, b);
}

/************************************************************************/
/*                             pj_ell_init()                            */
/************************************************************************/

/* initialize geographic shape parameters */
// This function works differently than the original pj_ell_set().
// It doesn't push parameters defined in ellps into params list.
// Instead it tries to use size (a, R) and shape (es, e, rf, f, b) parameters
// and then if needed falls back to ellps, then to datum and then to the default WGS84
template <typename Params, typename T>
inline void pj_ell_init(Params const& params, T &a, T &es)
{
    /* check for varying forms of ellipsoid input */
    a = es = 0.;

    /* R takes precedence */
    if (pj_param_f<srs::spar::r>(params, "R", srs::dpar::r, a)) {
        /* empty */
    } else { /* probable elliptical figure */

        // Set ellipsoid's size parameter
        a = pj_get_param_f<T, srs::spar::a>(params, "a", srs::dpar::a);
        bool is_a_set = a != 0.0;

        // Set ellipsoid's shape parameter
        T b = 0.0;
        bool is_ell_set = false;
        if (pj_param_f<srs::spar::es>(params, "es", srs::dpar::es, es)) {/* eccentricity squared */
            /* empty */
            is_ell_set = true;
        } else if (pj_param_f<srs::spar::e>(params, "e", srs::dpar::e, es)) { /* eccentricity */
            es = es * es;
            is_ell_set = true;
        } else if (pj_param_f<srs::spar::rf>(params, "rf", srs::dpar::rf, es)) { /* recip flattening */
            if (es == 0.0) {
                BOOST_THROW_EXCEPTION( projection_exception(error_rev_flattening_is_zero) );
            }    
            es = 1./ es;
            es = es * (2. - es);
            is_ell_set = true;
        } else if (pj_param_f<srs::spar::f>(params, "f", srs::dpar::f, es)) { /* flattening */
            es = es * (2. - es);
            is_ell_set = true;
        } else if (pj_param_f<srs::spar::b>(params, "b", srs::dpar::b, b)) { /* minor axis */
            es = pj_ell_b_to_es(a, b);
            is_ell_set = true;
        } /* else es == 0. and sphere of radius a */

        // NOTE: Below when ellps is used to initialize a and es
        // b is not set because it only has sense together with a
        // but a could have been set separately before, e.g. consider passing:
        // a=1 ellps=airy (a=6377563.396 b=6356256.910)
        // after setting size parameter a and shape parameter from ellps
        // b has to be recalculated

        // If ellipsoid's parameters are not set directly
        //   use ellps parameter
        if (! is_a_set || ! is_ell_set) {
            T ellps_a = 0, ellps_b = 0;
            if (pj_ell_init_ellps(params, ellps_a, ellps_b)) {
                if (! is_a_set) {
                    a = ellps_a;
                    is_a_set = true;
                }
                if (! is_ell_set) {
                    es = pj_ell_b_to_es(ellps_a, ellps_b);
                    is_ell_set = true;
                }
            }
        }

        // If ellipsoid's parameters are not set
        //   use ellps defined by datum parameter
        if (! is_a_set || ! is_ell_set)
        {
            const pj_datums_type<T>* datum = pj_datum_find_datum<T>(params);
            if (datum != NULL)
            {
                pj_ellps_type<T> const& pj_ellp = pj_get_ellps<T>().first[datum->ellps];
                if (! is_a_set) {
                    a = pj_ellp.a;
                    is_a_set = true;
                }
                if (! is_ell_set) {
                    es = pj_ell_b_to_es(pj_ellp.a, pj_ellp.b);
                    is_ell_set = true;
                }
            }
        }

        // If ellipsoid's parameters are still not set
        //   use default WGS84
        if ((! is_a_set || ! is_ell_set)
         && ! pj_get_param_b<srs::spar::no_defs>(params, "no_defs", srs::dpar::no_defs))
        {
            pj_ellps_type<T> const& pj_ellp = pj_get_ellps<T>().first[srs::dpar::ellps_wgs84];
            if (! is_a_set) {
                a = pj_ellp.a;
                is_a_set = true;
            }
            if (! is_ell_set) {
                es = pj_ell_b_to_es(pj_ellp.a, pj_ellp.b);
                is_ell_set = true;
            }
        }

        if (b == 0.0)
            b = a * sqrt(1. - es);

        /* following options turn ellipsoid into equivalent sphere */
        if (pj_get_param_b<srs::spar::r_au>(params, "R_A", srs::dpar::r_au)) { /* sphere--area of ellipsoid */
            a *= 1. - es * (SIXTH<T>() + es * (RA4<T>() + es * RA6<T>()));
            es = 0.;
        } else if (pj_get_param_b<srs::spar::r_v>(params, "R_V", srs::dpar::r_v)) { /* sphere--vol. of ellipsoid */
            a *= 1. - es * (SIXTH<T>() + es * (RV4<T>() + es * RV6<T>()));
            es = 0.;
        } else if (pj_get_param_b<srs::spar::r_a>(params, "R_a", srs::dpar::r_a)) { /* sphere--arithmetic mean */
            a = .5 * (a + b);
            es = 0.;
        } else if (pj_get_param_b<srs::spar::r_g>(params, "R_g", srs::dpar::r_g)) { /* sphere--geometric mean */
            a = sqrt(a * b);
            es = 0.;
        } else if (pj_get_param_b<srs::spar::r_h>(params, "R_h", srs::dpar::r_h)) { /* sphere--harmonic mean */
            a = 2. * a * b / (a + b);
            es = 0.;
        } else {
            T tmp;
            bool i = pj_param_r<srs::spar::r_lat_a>(params, "R_lat_a", srs::dpar::r_lat_a, tmp);
            if (i || /* sphere--arith. */
                pj_param_r<srs::spar::r_lat_g>(params, "R_lat_g", srs::dpar::r_lat_g, tmp)) { /* or geom. mean at latitude */

                tmp = sin(tmp);
                if (geometry::math::abs(tmp) > geometry::math::half_pi<T>()) {
                    BOOST_THROW_EXCEPTION( projection_exception(error_ref_rad_larger_than_90) );
                }
                tmp = 1. - es * tmp * tmp;
                a *= i ? .5 * (1. - es + tmp) / ( tmp * sqrt(tmp)) :
                    sqrt(1. - es) / tmp;
                es = 0.;
            }
        }
    }

    /* some remaining checks */
    if (es < 0.) {
        BOOST_THROW_EXCEPTION( projection_exception(error_es_less_than_zero) );
    }
    if (a <= 0.) {
        BOOST_THROW_EXCEPTION( projection_exception(error_major_axis_not_given) );
    }
}

template <typename Params>
struct static_srs_tag_check_nonexpanded
{
    typedef std::conditional_t
        <
            geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param_t<srs::spar::r>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::r_au>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::r_v>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::r_a>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::r_g>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::r_h>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param_t<srs::spar::r_lat_a>::pred
                >::value
         || geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param_t<srs::spar::r_lat_g>::pred
                >::value,
            srs_sphere_tag,
            // NOTE: The assumption here is that if the user defines either one of:
            // b, es, e, f, rf parameters then he wants to define spheroid, not sphere
            std::conditional_t
                <
                    geometry::tuples::exists_if
                        <
                            Params, srs::spar::detail::is_param_t<srs::spar::b>::pred
                        >::value
                 || geometry::tuples::exists_if
                        <
                            Params, srs::spar::detail::is_param_t<srs::spar::es>::pred
                        >::value
                 || geometry::tuples::exists_if
                        <
                            Params, srs::spar::detail::is_param_t<srs::spar::e>::pred
                        >::value
                 || geometry::tuples::exists_if
                        <
                            Params, srs::spar::detail::is_param_t<srs::spar::rf>::pred
                        >::value
                 || geometry::tuples::exists_if
                        <
                            Params, srs::spar::detail::is_param_t<srs::spar::f>::pred
                        >::value,
                    srs_spheroid_tag,
                    void
                >
        > type;
};

template <typename Params>
struct static_srs_tag_check_ellps
{
    typedef typename geometry::tag
        <
            typename srs::spar::detail::ellps_traits
                <
                    typename geometry::tuples::find_if
                        <
                            Params,
                            srs::spar::detail::is_param_tr<srs::spar::detail::ellps_traits>::pred
                        >::type
                >::template model_type<double>::type // dummy type
        >::type type;
};

template <typename Params>
struct static_srs_tag_check_datum
{
    typedef typename geometry::tag
        <
            typename srs::spar::detail::ellps_traits
                <
                    typename srs::spar::detail::datum_traits
                        <
                            typename geometry::tuples::find_if
                                <
                                    Params,
                                    srs::spar::detail::is_param_tr<srs::spar::detail::datum_traits>::pred
                                >::type
                        >::ellps_type
                >::template model_type<double>::type // dummy type
        >::type type;
};

template
<
    typename Params,
    typename NonExpandedTag = typename static_srs_tag_check_nonexpanded
                                <
                                    Params
                                >::type,
    typename EllpsTag = typename static_srs_tag_check_ellps
                            <
                                Params
                            >::type,
    typename DatumTag = typename static_srs_tag_check_datum
                            <
                                Params
                            >::type
>
struct static_srs_tag
{
    // User passed one of the non-ellps, non-datum parameters
    typedef NonExpandedTag type;
};

template <typename Params, typename EllpsTag, typename DatumTag>
struct static_srs_tag<Params, void, EllpsTag, DatumTag>
{
    // User didn't pass neither one of the non-ellps, non-datum parameters
    // but passed ellps
    typedef EllpsTag type;
};

template <typename Params, typename DatumTag>
struct static_srs_tag<Params, void, void, DatumTag>
{
    // User didn't pass neither one of the non-ellps, non-datum parameters
    // nor ellps parameter but passed datum parameter
    typedef DatumTag type;
};

template <typename Params>
struct static_srs_tag<Params, void, void, void>
{
    // User didn't pass any parameter defining model
    // so use default or generate error
    typedef std::conditional_t
        <
            geometry::tuples::exists_if
                <
                    Params, srs::spar::detail::is_param<srs::spar::no_defs>::pred
                >::value,
            void,
            srs_spheroid_tag // WGS84
        > type;

    static const bool is_found = ! std::is_void<type>::value;
    BOOST_GEOMETRY_STATIC_ASSERT((is_found),
        "Expected ellipsoid or sphere definition.",
        Params);
};


template <typename T>
inline void pj_calc_ellipsoid_params(parameters<T> & p, T const& a, T const& es) {
/****************************************************************************************
    Calculate a large number of ancillary ellipsoidal parameters, in addition to
    the two traditional PROJ defining parameters: Semimajor axis, a, and the
    eccentricity squared, es.

    Most of these parameters are fairly cheap to compute in comparison to the overall
    effort involved in initializing a PJ object. They may, however, take a substantial
    part of the time taken in computing an individual point transformation.

    So by providing them up front, we can amortize the (already modest) cost over all
    transformations carried out over the entire lifetime of a PJ object, rather than
    incur that cost for every single transformation.

    Most of the parameter calculations here are based on the "angular eccentricity",
    i.e. the angle, measured from the semiminor axis, of a line going from the north
    pole to one of the foci of the ellipsoid - or in other words: The arc sine of the
    eccentricity.

    The formulae used are mostly taken from:

    Richard H. Rapp: Geometric Geodesy, Part I, (178 pp, 1991).
    Columbus, Ohio:  Dept. of Geodetic Science
    and Surveying, Ohio State University.

****************************************************************************************/

    p.a = a;
    p.es = es;

    /* Compute some ancillary ellipsoidal parameters */
    if (p.e==0)
        p.e = sqrt(p.es);  /* eccentricity */
    //p.alpha = asin (p.e);  /* angular eccentricity */

    /* second eccentricity */
    //p.e2  = tan (p.alpha);
    //p.e2s = p.e2 * p.e2;

    /* third eccentricity */
    //p.e3    = (0!=p.alpha)? sin (p.alpha) / sqrt(2 - sin (p.alpha)*sin (p.alpha)): 0;
    //p.e3s = p.e3 * p.e3;

    /* flattening */
    //if (0==p.f)
    //    p.f  = 1 - cos (p.alpha);   /* = 1 - sqrt (1 - PIN->es); */
    //p.rf = p.f != 0.0 ? 1.0/p.f: HUGE_VAL;

    /* second flattening */
    //p.f2  = (cos(p.alpha)!=0)? 1/cos (p.alpha) - 1: 0;
    //p.rf2 = p.f2 != 0.0 ? 1/p.f2: HUGE_VAL;

    /* third flattening */
    //p.n  = pow (tan (p.alpha/2), 2);
    //p.rn = p.n != 0.0 ? 1/p.n: HUGE_VAL;

    /* ...and a few more */
    //if (0==p.b)
    //    p.b  = (1 - p.f)*p.a;
    //p.rb = 1. / p.b;
    p.ra = 1. / p.a;

    p.one_es = 1. - p.es;
    if (p.one_es == 0.) {
        BOOST_THROW_EXCEPTION( projection_exception(error_eccentricity_is_one) );
    }

    p.rone_es = 1./p.one_es;
}


} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP
