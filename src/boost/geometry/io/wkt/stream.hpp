// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKT_STREAM_HPP
#define BOOST_GEOMETRY_IO_WKT_STREAM_HPP

#include <boost/geometry/io/wkt/write.hpp>

// This short file contains only one manipulator, streaming as WKT
// Don't include this in any standard-included header file.

// Don't use namespace boost::geometry, to enable the library to stream custom
// geometries which are living outside the namespace boost::geometry

// This is currently not documented on purpose: the Doxygen 2 QBK generator
// should be updated w.r.t. << which in the end ruins the DocBook XML
template<typename Char, typename Traits, typename Geometry>
inline std::basic_ostream<Char, Traits>& operator<<
    (
        std::basic_ostream<Char, Traits> &os,
        Geometry const& geom
    )
{
    os << boost::geometry::wkt(geom);
    return os;
}

#endif // BOOST_GEOMETRY_IO_WKT_STREAM_HPP
