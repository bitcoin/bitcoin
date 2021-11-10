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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELLPS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELLPS_HPP

#include <string>

namespace boost { namespace geometry { namespace projections {

namespace detail {

// Originally defined in projects.h
template <typename T>
struct pj_ellps_type
{
    std::string id;    /* ellipse keyword name */
    //std::string major; /* a's value */
    //std::string ell;   /* elliptical parameter value */
    //bool is_rf;        /* rf or b? */
    T a;
    T b;
    //std::string name;  /* comments */
};

inline double b_from_a_rf(double a, double rf)
{
    return a * (1.0 - 1.0 / rf);
}

#define BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF(ID, A, RF, NAME) \
    {ID, /*#A, #RF, true,*/ A, b_from_a_rf(A, RF), /*NAME*/}

#define BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B(ID, A, B, NAME) \
    {ID, /*#A, #B, false,*/ A, B, /*NAME*/}

template <typename T>
inline std::pair<const pj_ellps_type<T>*, int> pj_get_ellps()
{
    static const pj_ellps_type<T> pj_ellps[] =
    {
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("MERIT",     6378137.0,   298.257,           "MERIT 1983"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("SGS85",     6378136.0,   298.257,           "Soviet Geodetic System 85"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("GRS80",     6378137.0,   298.257222101,     "GRS 1980(IUGG, 1980)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("IAU76",     6378140.0,   298.257,           "IAU 1976"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("airy",      6377563.396, 6356256.910,       "Airy 1830"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("APL4.9",    6378137.0,   298.25,            "Appl. Physics. 1965"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("NWL9D",     6378145.0,   298.25,            "Naval Weapons Lab., 1965"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("mod_airy",  6377340.189, 6356034.446,       "Modified Airy"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("andrae",    6377104.43,  300.0,             "Andrae 1876 (Den., Iclnd.)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("aust_SA",   6378160.0,   298.25,            "Australian Natl & S. Amer. 1969"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("GRS67",     6378160.0,   298.2471674270,    "GRS 67(IUGG 1967)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("bessel",    6377397.155, 299.1528128,       "Bessel 1841"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("bess_nam",  6377483.865, 299.1528128,       "Bessel 1841 (Namibia)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("clrk66",    6378206.4,   6356583.8,         "Clarke 1866"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("clrk80",    6378249.145, 293.4663,          "Clarke 1880 mod."),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("clrk80ign", 6378249.2,   293.4660212936269, "Clarke 1880 (IGN)."),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("CPM",       6375738.7,   334.29,            "Comm. des Poids et Mesures 1799"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("delmbr",    6376428.0,   311.5,             "Delambre 1810 (Belgium)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("engelis",   6378136.05,  298.2566,          "Engelis 1985"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("evrst30",   6377276.345, 300.8017,          "Everest 1830"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("evrst48",   6377304.063, 300.8017,          "Everest 1948"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("evrst56",   6377301.243, 300.8017,          "Everest 1956"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("evrst69",   6377295.664, 300.8017,          "Everest 1969"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("evrstSS",   6377298.556, 300.8017,          "Everest (Sabah & Sarawak)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("fschr60",   6378166.0,   298.3,             "Fischer (Mercury Datum) 1960"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("fschr60m",  6378155.0,   298.3,             "Modified Fischer 1960"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("fschr68",   6378150.0,   298.3,             "Fischer 1968"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("helmert",   6378200.0,   298.3,             "Helmert 1906"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("hough",     6378270.0,   297.0,             "Hough"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("intl",      6378388.0,   297.0,             "International 1909 (Hayford)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("krass",     6378245.0,   298.3,             "Krassovsky, 1942"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("kaula",     6378163.0,   298.24,            "Kaula 1961"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("lerch",     6378139.0,   298.257,           "Lerch 1979"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("mprts",     6397300.0,   191.0,             "Maupertius 1738"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("new_intl",  6378157.5,   6356772.2,         "New International 1967"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("plessis",   6376523.0,   6355863.0,         "Plessis 1817 (France)"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("SEasia",    6378155.0,   6356773.3205,      "Southeast Asia"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("walbeck",   6376896.0,   6355834.8467,      "Walbeck"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("WGS60",     6378165.0,   298.3,             "WGS 60"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("WGS66",     6378145.0,   298.25,            "WGS 66"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_RF("WGS72",     6378135.0,   298.26,            "WGS 72"),
        // This has to be consistent with default spheroid and values in pj_datum_transform
        // TODO: Define in one place
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("WGS84",     6378137.0,   6356752.3142451793, "WGS 84"),
        BOOST_GEOMETRY_PROJECTIONS_DETAIL_PJ_ELLPS_B ("sphere",    6370997.0,   6370997.0,         "Normal Sphere (r=6370997)")
    };

    return std::make_pair(pj_ellps, (int)(sizeof(pj_ellps) / sizeof(pj_ellps[0])));
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELLPS_HPP
