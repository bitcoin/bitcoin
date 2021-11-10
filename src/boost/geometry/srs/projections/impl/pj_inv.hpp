// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2019.
// Modifications copyright (c) 2017-2019, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_PJ_INV_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PJ_INV_HPP



#include <boost/geometry/srs/projections/impl/adjlon.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/util/math.hpp>

/* general inverse projection */

namespace boost { namespace geometry { namespace projections
{

namespace detail
{

 /* inverse projection entry */
template <typename PRJ, typename LL, typename XY, typename PAR>
inline void pj_inv(PRJ const& prj, PAR const& par, XY const& xy, LL& ll)
{
    typedef typename PAR::type calc_t;
    static const calc_t EPS = 1.0e-12;

    /* can't do as much preliminary checking as with forward */
    /* descale and de-offset */
    calc_t xy_x = (geometry::get<0>(xy) * par.to_meter - par.x0) * par.ra;
    calc_t xy_y = (geometry::get<1>(xy) * par.to_meter - par.y0) * par.ra;
    calc_t lon = 0, lat = 0;

    prj.inv(par, xy_x, xy_y, lon, lat); /* inverse project */
    
    lon += par.lam0; /* reduce from del lp.lam */
    if (!par.over)
        lon = adjlon(lon); /* adjust longitude to CM */
    if (par.geoc && geometry::math::abs(geometry::math::abs(lat)-geometry::math::half_pi<calc_t>()) > EPS)
        lat = atan(par.one_es * tan(lat));

    geometry::set_from_radian<0>(ll, lon);
    geometry::set_from_radian<1>(ll, lat);
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif
