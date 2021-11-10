// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_COORDINATE_CAST_HPP
#define BOOST_GEOMETRY_UTIL_COORDINATE_CAST_HPP

#include <cstdlib>
#include <string>
#include <boost/lexical_cast.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

/*!
\brief cast coordinates from a string to a coordinate type
\detail By default it uses lexical_cast. However, lexical_cast seems not to support
    See also "define_pi" where the same issue is solved
*/
template <typename CoordinateType>
struct coordinate_cast
{
    static inline CoordinateType apply(std::string const& source)
    {
#if defined(BOOST_GEOMETRY_NO_LEXICAL_CAST)
        return atof(source.c_str());
#else
        return boost::lexical_cast<CoordinateType>(source);
#endif
    }
};


} // namespace detail
#endif



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_COORDINATE_CAST_HPP
