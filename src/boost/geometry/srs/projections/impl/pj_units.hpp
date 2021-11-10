// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2018.
// Modifications copyright (c) 2017-2018, Oracle and/or its affiliates.
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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_UNITS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_UNITS_HPP

#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry { namespace projections {
namespace detail {

// Originally defined in projects.h
struct pj_units_type
{
    std::string id;       /* units keyword */
    std::string to_meter; /* multiply by value to get meters */
    double numerator;
    double denominator;
    std::string name;     /* comments */
};

/* Field 2 that contains the multiplier to convert named units to meters
** may be expressed by either a simple floating point constant or a
** numerator/denomenator values (e.g. 1/1000) */

static const pj_units_type pj_units[] =
{
    { "km",     "1000.",    1000.0, 1.0, "Kilometer" },
    { "m",      "1.",       1.0,    1.0, "Meter" },
    { "dm",     "1/10",     1.0, 10.0, "Decimeter" },
    { "cm",     "1/100",    1.0, 100.0, "Centimeter" },
    { "mm",     "1/1000",   1.0, 1000.0, "Millimeter" },
    { "kmi",    "1852.",   1852.0, 1.0,     "International Nautical Mile" },
    { "in",     "0.0254",   0.0254, 1.0, "International Inch" },
    { "ft",     "0.3048",   0.3048, 1.0, "International Foot" },
    { "yd",     "0.9144",   0.9144, 1.0, "International Yard" },
    { "mi",     "1609.344", 1609.344, 1.0, "International Statute Mile" },
    { "fath",   "1.8288",   1.8288, 1.0, "International Fathom" },
    { "ch",     "20.1168",  20.1168, 1.0, "International Chain" },
    { "link",   "0.201168", 0.201168, 1.0, "International Link" },
    { "us-in",  "1./39.37", 1.0, 39.37, "U.S. Surveyor's Inch" },
    { "us-ft",  "0.304800609601219", 0.304800609601219, 1.0, "U.S. Surveyor's Foot" },
    { "us-yd",  "0.914401828803658", 0.914401828803658, 1.0, "U.S. Surveyor's Yard" },
    { "us-ch",  "20.11684023368047", 20.11684023368047, 1.0, "U.S. Surveyor's Chain" },
    { "us-mi",  "1609.347218694437", 1609.347218694437, 1.0, "U.S. Surveyor's Statute Mile" },
    { "ind-yd", "0.91439523",        0.91439523, 1.0, "Indian Yard" },
    { "ind-ft", "0.30479841",        0.30479841, 1.0, "Indian Foot" },
    { "ind-ch", "20.11669506",       20.11669506, 1.0, "Indian Chain" }
};

} // detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_UNITS_HPP
