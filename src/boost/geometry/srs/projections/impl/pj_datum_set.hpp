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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP


#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <boost/geometry/srs/projections/dpar.hpp>
#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/impl/pj_datums.hpp>
#include <boost/geometry/srs/projections/impl/pj_param.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>
#include <boost/geometry/srs/projections/spar.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail {


/* SEC_TO_RAD = Pi/180/3600 */
template <typename T>
inline T sec_to_rad() { return 4.84813681109535993589914102357e-6; }

/************************************************************************/
/*                        pj_datum_find_datum()                         */
/************************************************************************/

template <typename T>
inline const pj_datums_type<T>* pj_datum_find_datum(srs::detail::proj4_parameters const& params)
{
    std::string name = pj_get_param_s(params, "datum");
    if(! name.empty())
    {
        /* find the datum definition */
        const pj_datums_type<T>* pj_datums = pj_get_datums<T>().first;
        const int n = pj_get_datums<T>().second;
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_datums[i].id == name)
            {
                index = i;
            }
        }

        if (index != -1)
        {
            return pj_datums + index;
        }
        else
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
        }
    }

    return NULL;
}

template <typename T>
inline const pj_datums_type<T>* pj_datum_find_datum(srs::dpar::parameters<T> const& params)
{
    typename srs::dpar::parameters<T>::const_iterator
        it = pj_param_find(params, srs::dpar::datum);
    
    if (it != params.end())
    {
        const pj_datums_type<T>* pj_datums = pj_get_datums<T>().first;
        const int n = pj_get_datums<T>().second;
        int i = it->template get_value<int>();
        if (i >= 0 && i < n)
        {
            return pj_datums + i;
        }
        else
        {
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
        }
    }

    return NULL;
}

template
<
    typename Params,
    typename Param = typename geometry::tuples::find_if
        <
            Params,
            srs::spar::detail::is_param_tr<srs::spar::detail::datum_traits>::pred
        >::type,
    bool IsFound = geometry::tuples::is_found<Param>::value
>
struct pj_datum_find_datum_static
{
    template <typename T>
    static const pj_datums_type<T>* apply(Params const& )
    {
        const pj_datums_type<T>* pj_datums = pj_get_datums<T>().first;
        const int n = pj_get_datums<T>().second;
        const int i = srs::spar::detail::datum_traits<Param>::id;
        if (i >= 0 && i < n)
        {
            return pj_datums + i;
        }
        else
        {
            // TODO: Implemnt as BOOST_GEOMETRY_STATIC_ASSERT instead
            BOOST_THROW_EXCEPTION( projection_exception(error_unknown_ellp_param) );
        }
    }
};
template <typename Params, typename Param>
struct pj_datum_find_datum_static<Params, Param, false>
{
    template <typename T>
    static const pj_datums_type<T>* apply(Params const& )
    {
        return NULL;
    }
};

template <typename T, typename ...Ps>
inline const pj_datums_type<T>* pj_datum_find_datum(srs::spar::parameters<Ps...> const& params)
{
    return pj_datum_find_datum_static
        <
            srs::spar::parameters<Ps...>
        >::template apply<T>(params);
}

/************************************************************************/
/*                        pj_datum_find_nadgrids()                      */
/************************************************************************/

inline bool pj_datum_find_nadgrids(srs::detail::proj4_parameters const& params,
                                   srs::detail::nadgrids & out)
{
    std::string snadgrids = pj_get_param_s(params, "nadgrids");
    if (! snadgrids.empty())
    {
        for (std::string::size_type i = 0 ; i < snadgrids.size() ; )
        {
            std::string::size_type end = snadgrids.find(',', i);
            std::string name = snadgrids.substr(i, end - i);
                
            i = end;
            if (end != std::string::npos)
                ++i;

            if (! name.empty())
                out.push_back(name);
        }
    }

    return ! out.empty();
}

template <typename T>
inline bool pj_datum_find_nadgrids(srs::dpar::parameters<T> const& params,
                                   srs::detail::nadgrids & out)
{
    typename srs::dpar::parameters<T>::const_iterator
        it = pj_param_find(params, srs::dpar::nadgrids);
    if (it != params.end())
    {
        out = it->template get_value<srs::detail::nadgrids>();
    }
    
    return ! out.empty();
}

template
<
    typename Params,
    int I = geometry::tuples::find_index_if
        <
            Params,
            srs::spar::detail::is_param<srs::spar::nadgrids>::pred
        >::value,
    int N = geometry::tuples::size<Params>::value
>
struct pj_datum_find_nadgrids_static
{
    static void apply(Params const& params, srs::detail::nadgrids & out)
    {
        out = geometry::tuples::get<I>(params);
    }
};
template <typename Params, int N>
struct pj_datum_find_nadgrids_static<Params, N, N>
{
    static void apply(Params const& , srs::detail::nadgrids & )
    {}
};

template <typename ...Ps>
inline bool pj_datum_find_nadgrids(srs::spar::parameters<Ps...> const& params,
                                   srs::detail::nadgrids & out)
{
    pj_datum_find_nadgrids_static
        <
            srs::spar::parameters<Ps...>
        >::apply(params, out);

    return ! out.empty();
}

/************************************************************************/
/*                        pj_datum_find_towgs84()                       */
/************************************************************************/

template <typename T>
inline bool pj_datum_find_towgs84(srs::detail::proj4_parameters const& params,
                                  srs::detail::towgs84<T> & out)
{
    std::string towgs84 = pj_get_param_s(params, "towgs84");
    if(! towgs84.empty())
    {
        std::vector<std::string> parm;
        boost::split(parm, towgs84, boost::is_any_of(" ,"));

        std::size_t n = (std::min<std::size_t>)(parm.size(), 7);
        std::size_t z = n <= 3 ? 3 : 7;

        /* parse out the pvalues */
        for (std::size_t i = 0 ; i < n; ++i)
        {
            out.push_back(geometry::str_cast<T>(parm[i]));
        }
        for (std::size_t i = out.size() ; i < z; ++i)
        {
            out.push_back(T(0));
        }
    }

    return ! out.empty();
}

template <typename T>
inline bool pj_datum_find_towgs84(srs::dpar::parameters<T> const& params,
                                  srs::detail::towgs84<T> & out)
{
    typename srs::dpar::parameters<T>::const_iterator
        it = pj_param_find(params, srs::dpar::towgs84);
    
    if (it != params.end())
    {
        srs::detail::towgs84<T> const&
            towgs84 = it->template get_value<srs::detail::towgs84<T> >();

        std::size_t n = (std::min<std::size_t>)(towgs84.size(), 7u);
        std::size_t z = n <= 3 ? 3 : 7;

        for (std::size_t i = 0 ; i < n; ++i)
        {
            out.push_back(towgs84[i]);
        }
        for (std::size_t i = out.size() ; i < z; ++i)
        {
            out.push_back(T(0));
        }
    }

    return ! out.empty();
}

template
<
    typename Params,
    int I = geometry::tuples::find_index_if
        <
            Params,
            srs::spar::detail::is_param_t<srs::spar::towgs84>::pred
        >::value,
    int N = geometry::tuples::size<Params>::value
>
struct pj_datum_find_towgs84_static
{
    template <typename T>
    static void apply(Params const& params, srs::detail::towgs84<T> & out)
    {
        typename geometry::tuples::element<I, Params>::type const&
            towgs84 = geometry::tuples::get<I>(params);

        std::size_t n = (std::min<std::size_t>)(towgs84.size(), 7u);
        std::size_t z = n <= 3 ? 3 : 7;

        for (std::size_t i = 0 ; i < n; ++i)
        {
            out.push_back(towgs84[i]);
        }
        for (std::size_t i = out.size() ; i < z; ++i)
        {
            out.push_back(T(0));
        }
    }
};
template <typename Params, int N>
struct pj_datum_find_towgs84_static<Params, N, N>
{
    template <typename T>
    static void apply(Params const& , srs::detail::towgs84<T> & )
    {}
};

template <typename T, typename ...Ps>
inline bool pj_datum_find_towgs84(srs::spar::parameters<Ps...> const& params,
                                  srs::detail::towgs84<T> & out)
{
    pj_datum_find_towgs84_static
        <
            srs::spar::parameters<Ps...>
        >::apply(params, out);

    return ! out.empty();
}

/************************************************************************/
/*                        pj_datum_prepare_towgs84()                    */
/************************************************************************/

template <typename T>
inline bool pj_datum_prepare_towgs84(srs::detail::towgs84<T> & towgs84)
{
    if( towgs84.size() == 7
     && (towgs84[3] != 0.0
      || towgs84[4] != 0.0
      || towgs84[5] != 0.0
      || towgs84[6] != 0.0) )
    {
        static const T sec_to_rad = detail::sec_to_rad<T>();

        /* transform from arc seconds to radians */
        towgs84[3] *= sec_to_rad;
        towgs84[4] *= sec_to_rad;
        towgs84[5] *= sec_to_rad;
        /* transform from parts per million to scaling factor */
        towgs84[6] = (towgs84[6]/1000000.0) + 1;
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/
/*                            pj_datum_init()                           */
/************************************************************************/

// This function works differently than the original pj_datum_set().
// It doesn't push parameters defined in datum into params list.
// Instead it tries to use nadgrids and towgs84 and only then
// falls back to nadgrid or towgs84 defiend in datum parameter.
template <typename Params, typename T>
inline void pj_datum_init(Params const& params, parameters<T> & projdef)
{
    projdef.datum_type = datum_unknown;

    // Check for nadgrids parameter.
    if(pj_datum_find_nadgrids(params, projdef.nadgrids))
    {
        // NOTE: It's different than in the original proj4.
        // Nadgrids names are stored in projection definition.

        projdef.datum_type = datum_gridshift;
    }
    // Check for towgs84 parameter.
    else if(pj_datum_find_towgs84(params, projdef.datum_params))
    {
        if (pj_datum_prepare_towgs84(projdef.datum_params))
        {
            projdef.datum_type = datum_7param;
        }
        else
        {
            projdef.datum_type = datum_3param;
        }

        /* Note that pj_init() will later switch datum_type to
           PJD_WGS84 if shifts are all zero, and ellipsoid is WGS84 or GRS80 */
    }
    // Check for datum parameter.
    else
    {
        const pj_datums_type<T>* datum = pj_datum_find_datum<T>(params);
        if (datum != NULL)
        {
            if (! datum->nadgrids.empty())
            {
                projdef.nadgrids = datum->nadgrids;
                projdef.datum_type = datum_gridshift;
            }
            else if ( ! datum->towgs84.empty() )
            {
                projdef.datum_params = datum->towgs84;
                if (pj_datum_prepare_towgs84(projdef.datum_params))
                {
                    projdef.datum_type = datum_7param;
                }
                else
                {
                    projdef.datum_type = datum_3param;
                }
            }
        }
    }
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP
