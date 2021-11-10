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
// PROJ4 is converted to Geometry Library by Barend Gehrels (Geodan, Amsterdam)

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP

#include <cstdlib>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/srs/projections/dpar.hpp>
#include <boost/geometry/srs/projections/impl/dms_parser.hpp>
#include <boost/geometry/srs/projections/impl/pj_datum_set.hpp>
#include <boost/geometry/srs/projections/impl/pj_datums.hpp>
#include <boost/geometry/srs/projections/impl/pj_ell_set.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/impl/pj_units.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>
#include <boost/geometry/srs/projections/spar.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/condition.hpp>


namespace boost { namespace geometry { namespace projections
{


namespace detail
{

/************************************************************************/
/*                           pj_init_proj()                             */
/************************************************************************/

template <typename T>
inline void pj_init_proj(srs::detail::proj4_parameters const& params,
                         parameters<T> & par)
{
    par.id = pj_get_param_s(params, "proj");
}

template <typename T>
inline void pj_init_proj(srs::dpar::parameters<T> const& params,
                         parameters<T> & par)
{
    typename srs::dpar::parameters<T>::const_iterator it = pj_param_find(params, srs::dpar::proj);
    if (it != params.end())
    {
        par.id = static_cast<srs::dpar::value_proj>(it->template get_value<int>());
    }
}

template <typename T, typename ...Ps>
inline void pj_init_proj(srs::spar::parameters<Ps...> const& ,
                         parameters<T> & par)
{
    typedef srs::spar::parameters<Ps...> params_type;
    typedef typename geometry::tuples::find_if
        <
            params_type,
            srs::spar::detail::is_param_tr<srs::spar::detail::proj_traits>::pred
        >::type proj_type;

    static const bool is_found = geometry::tuples::is_found<proj_type>::value;

    BOOST_GEOMETRY_STATIC_ASSERT((is_found), "Projection not named.", params_type);

    par.id = srs::spar::detail::proj_traits<proj_type>::id;
}

/************************************************************************/
/*                           pj_init_units()                            */
/************************************************************************/

template <typename T, bool Vertical>
inline void pj_init_units(srs::detail::proj4_parameters const& params,
                          T & to_meter,
                          T & fr_meter,
                          T const& default_to_meter,
                          T const& default_fr_meter)
{
    std::string s;
    std::string units = pj_get_param_s(params, Vertical ? "vunits" : "units");
    if (! units.empty())
    {
        static const int n = sizeof(pj_units) / sizeof(pj_units[0]);
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_units[i].id == units)
            {
                index = i;
            }
        }

        if (index == -1) {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknow_unit_id) );
        }
        s = pj_units[index].to_meter;
    }

    if (s.empty())
    {
        s = pj_get_param_s(params, Vertical ? "vto_meter" : "to_meter");
    }

    // TODO: numerator and denominator could be taken from pj_units
    if (! s.empty())
    {
        std::size_t const pos = s.find('/');
        if (pos == std::string::npos)
        {
            to_meter = geometry::str_cast<T>(s);
        }
        else
        {
            T const numerator = geometry::str_cast<T>(s.substr(0, pos));
            T const denominator = geometry::str_cast<T>(s.substr(pos + 1));
            if (numerator == 0.0 || denominator == 0.0)
            {
                BOOST_THROW_EXCEPTION( projection_exception(error_unit_factor_less_than_0) );
            }
            to_meter = numerator / denominator;
        }
        if (to_meter == 0.0)
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unit_factor_less_than_0) );
        }
        fr_meter = 1. / to_meter;
    }
    else
    {
        to_meter = default_to_meter;
        fr_meter = default_fr_meter;
    }
}

template <typename T, bool Vertical>
inline void pj_init_units(srs::dpar::parameters<T> const& params,
                          T & to_meter,
                          T & fr_meter,
                          T const& default_to_meter,
                          T const& default_fr_meter)
{
    typename srs::dpar::parameters<T>::const_iterator
        it = pj_param_find(params, Vertical ? srs::dpar::vunits : srs::dpar::units);
    if (it != params.end())
    {
        static const int n = sizeof(pj_units) / sizeof(pj_units[0]);
        const int i = it->template get_value<int>();
        if (i >= 0 && i < n)
        {
            T const numerator = pj_units[i].numerator;
            T const denominator = pj_units[i].denominator;
            if (numerator == 0.0 || denominator == 0.0)
            {
                BOOST_THROW_EXCEPTION( projection_exception(error_unit_factor_less_than_0) );
            }
            to_meter = numerator / denominator;
            fr_meter = 1. / to_meter;
        }
        else
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknow_unit_id) );
        }
    }
    else
    {
        it = pj_param_find(params, Vertical ? srs::dpar::vto_meter : srs::dpar::to_meter);
        if (it != params.end())
        {
            to_meter = it->template get_value<T>();
            fr_meter = 1. / to_meter;
        }
        else
        {
            to_meter = default_to_meter;
            fr_meter = default_fr_meter;
        }
    }
}

template
<
    typename Params,
    bool Vertical,
    int UnitsI = geometry::tuples::find_index_if
        <
            Params,
            std::conditional_t
                <
                    Vertical,
                    srs::spar::detail::is_param_t<srs::spar::vunits>,
                    srs::spar::detail::is_param_tr<srs::spar::detail::units_traits>
                >::template pred
        >::value,
    int ToMeterI = geometry::tuples::find_index_if
        <
            Params,
            std::conditional_t
                <
                    Vertical,
                    srs::spar::detail::is_param_t<srs::spar::vto_meter>,
                    srs::spar::detail::is_param_t<srs::spar::to_meter>
                >::template pred
        >::value,
    int N = geometry::tuples::size<Params>::value
>
struct pj_init_units_static
    : pj_init_units_static<Params, Vertical, UnitsI, N, N>
{};

template <typename Params, bool Vertical, int UnitsI, int N>
struct pj_init_units_static<Params, Vertical, UnitsI, N, N>
{
    static const int n = sizeof(pj_units) / sizeof(pj_units[0]);
    static const int i = srs::spar::detail::units_traits
                    <
                        typename geometry::tuples::element<UnitsI, Params>::type
                    >::id;
    static const bool is_valid = i >= 0 && i < n;

    BOOST_GEOMETRY_STATIC_ASSERT((is_valid), "Unknown unit ID.", Params);

    template <typename T>
    static void apply(Params const& ,
                      T & to_meter, T & fr_meter,
                      T const& , T const& )
    {
        T const numerator = pj_units[i].numerator;
        T const denominator = pj_units[i].denominator;
        if (numerator == 0.0 || denominator == 0.0)
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unit_factor_less_than_0) );
        }
        to_meter = numerator / denominator;
        fr_meter = 1. / to_meter;
    }
};

template <typename Params, bool Vertical, int ToMeterI, int N>
struct pj_init_units_static<Params, Vertical, N, ToMeterI, N>
{
    template <typename T>
    static void apply(Params const& params,
                      T & to_meter, T & fr_meter,
                      T const& , T const& )
    {
        to_meter = geometry::tuples::get<ToMeterI>(params).value;
        fr_meter = 1. / to_meter;
    }
};

template <typename Params, bool Vertical, int N>
struct pj_init_units_static<Params, Vertical, N, N, N>
{
    template <typename T>
    static void apply(Params const& ,
                      T & to_meter, T & fr_meter,
                      T const& default_to_meter, T const& default_fr_meter)
    {
        to_meter = default_to_meter;
        fr_meter = default_fr_meter;
    }
};

template <typename T, bool Vertical, typename ...Ps>
inline void pj_init_units(srs::spar::parameters<Ps...> const& params,
                          T & to_meter,
                          T & fr_meter,
                          T const& default_to_meter,
                          T const& default_fr_meter)
{
    pj_init_units_static
        <
            srs::spar::parameters<Ps...>,
            Vertical
        >::apply(params, to_meter, fr_meter, default_to_meter, default_fr_meter);
}

/************************************************************************/
/*                           pj_init_pm()                               */
/************************************************************************/

template <typename T>
inline void pj_init_pm(srs::detail::proj4_parameters const& params, T& val)
{
    std::string pm = pj_get_param_s(params, "pm");
    if (! pm.empty())
    {
        int n = sizeof(pj_prime_meridians) / sizeof(pj_prime_meridians[0]);
        for (int i = 0; i < n ; i++)
        {
            if(pj_prime_meridians[i].id == pm)
            {
                val = pj_prime_meridians[i].deg * math::d2r<T>();
                return;
            }
        }

        // TODO: Is this try-catch needed?
        // In other cases the bad_str_cast exception is simply thrown
        BOOST_TRY
        {
            val = dms_parser<T, true>::apply(pm).angle();
            return;
        }
        BOOST_CATCH(geometry::bad_str_cast const&)
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_prime_meridian) );
        }
        BOOST_CATCH_END
    }
    
    val = 0.0;
}

template <typename T>
inline void pj_init_pm(srs::dpar::parameters<T> const& params, T& val)
{
    typename srs::dpar::parameters<T>::const_iterator it = pj_param_find(params, srs::dpar::pm);
    if (it != params.end())
    {
        if (it->template is_value_set<int>())
        {
            int n = sizeof(pj_prime_meridians) / sizeof(pj_prime_meridians[0]);
            int i = it->template get_value<int>();
            if (i >= 0 && i < n)
            {
                val = pj_prime_meridians[i].deg * math::d2r<T>();
                return;
            }
            else
            {
                BOOST_THROW_EXCEPTION( projection_exception(error_unknown_prime_meridian) );
            }
        }
        else if (it->template is_value_set<T>())
        {
            val = it->template get_value<T>() * math::d2r<T>();
            return;
        }
    }

    val = 0.0;
}

template
<
    typename Params,
    int I = geometry::tuples::find_index_if
        <
            Params,
            srs::spar::detail::is_param_tr<srs::spar::detail::pm_traits>::pred
        >::value,
    int N = geometry::tuples::size<Params>::value
>
struct pj_init_pm_static
{
    template <typename T>
    static void apply(Params const& params, T & val)
    {
        typedef typename geometry::tuples::element<I, Params>::type param_type;

        val = srs::spar::detail::pm_traits<param_type>::value(geometry::tuples::get<I>(params));
    }
};
template <typename Params, int N>
struct pj_init_pm_static<Params, N, N>
{
    template <typename T>
    static void apply(Params const& , T & val)
    {
        val = 0;
    }
};

template <typename T, typename ...Ps>
inline void pj_init_pm(srs::spar::parameters<Ps...> const& params, T& val)
{
    pj_init_pm_static
        <
            srs::spar::parameters<Ps...>
        >::apply(params, val);
}

/************************************************************************/
/*                              pj_init()                               */
/*                                                                      */
/*      Main entry point for initialing a PJ projections                */
/*      definition.                                                     */
/************************************************************************/
template <typename T, typename Params>
inline parameters<T> pj_init(Params const& params)
{
    parameters<T> pin;

    // find projection -> implemented in projection factory
    pj_init_proj(params, pin);
    // exception thrown in projection<>
    // TODO: consider throwing here both projection_unknown_id_exception and
    // projection_not_named_exception in order to throw before other exceptions
    //if (pin.name.empty())
    //{ BOOST_THROW_EXCEPTION( projection_not_named_exception() ); }

    // NOTE: proj4 gets defaults from "proj_def.dat".
    // In Boost.Geometry this is emulated by manually setting them in
    // pj_ell_init and projections aea, lcc and lagrng
    
    /* set datum parameters */
    pj_datum_init(params, pin);

    /* set ellipsoid/sphere parameters */
    pj_ell_init(params, pin.a, pin.es);

    pin.a_orig = pin.a;
    pin.es_orig = pin.es;

    pin.e = sqrt(pin.es);
    pin.ra = 1. / pin.a;
    pin.one_es = 1. - pin.es;
    if (pin.one_es == 0.) {
        BOOST_THROW_EXCEPTION( projection_exception(error_eccentricity_is_one) );
    }
    pin.rone_es = 1./pin.one_es;

    /* Now that we have ellipse information check for WGS84 datum */
    if( pin.datum_type == datum_3param
        && pin.datum_params[0] == 0.0
        && pin.datum_params[1] == 0.0
        && pin.datum_params[2] == 0.0
        && pin.a == 6378137.0
        && geometry::math::abs(pin.es - 0.006694379990) < 0.000000000050 )/*WGS84/GRS80*/
    {
        pin.datum_type = datum_wgs84;
    }

    /* set pin.geoc coordinate system */
    pin.geoc = (pin.es && pj_get_param_b<srs::spar::geoc>(params, "geoc", srs::dpar::geoc));

    /* over-ranging flag */
    pin.over = pj_get_param_b<srs::spar::over>(params, "over", srs::dpar::over);

    /* longitude center for wrapping */
    pin.is_long_wrap_set = pj_param_r<srs::spar::lon_wrap>(params, "lon_wrap", srs::dpar::lon_wrap, pin.long_wrap_center);

    /* central meridian */
    pin.lam0 = pj_get_param_r<T, srs::spar::lon_0>(params, "lon_0", srs::dpar::lon_0);

    /* central latitude */
    pin.phi0 = pj_get_param_r<T, srs::spar::lat_0>(params, "lat_0", srs::dpar::lat_0);

    /* false easting and northing */
    pin.x0 = pj_get_param_f<T, srs::spar::x_0>(params, "x_0", srs::dpar::x_0);
    pin.y0 = pj_get_param_f<T, srs::spar::y_0>(params, "y_0", srs::dpar::y_0);

    /* general scaling factor */
    if (pj_param_f<srs::spar::k_0>(params, "k_0", srs::dpar::k_0, pin.k0)) {
        /* empty */
    } else if (pj_param_f<srs::spar::k>(params, "k", srs::dpar::k, pin.k0)) {
        /* empty */
    } else
        pin.k0 = 1.;
    if (pin.k0 <= 0.) {
        BOOST_THROW_EXCEPTION( projection_exception(error_k_less_than_zero) );
    }

    /* set units */
    pj_init_units<T, false>(params, pin.to_meter, pin.fr_meter, 1., 1.);
    pj_init_units<T, true>(params, pin.vto_meter, pin.vfr_meter, pin.to_meter, pin.fr_meter);

    /* prime meridian */
    pj_init_pm(params, pin.from_greenwich);

    return pin;
}


} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP
