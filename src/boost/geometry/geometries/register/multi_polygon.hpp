// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_REGISTER_MULTI_POLYGON_HPP
#define BOOST_GEOMETRY_GEOMETRIES_REGISTER_MULTI_POLYGON_HPP

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

/*!
\brief \brief_macro{multi_polygon}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_MULTI_POLYGON, multi_polygon} The
    multi_polygon may contain template parameters, which must be specified then.
\param MultiPolygon \param_macro_type{multi_polygon}

\qbk{
[heading Example]
[register_multi_polygon]
[register_multi_polygon_output]
}
*/
#define BOOST_GEOMETRY_REGISTER_MULTI_POLYGON(MultiPolygon) \
namespace boost { namespace geometry { namespace traits {  \
    template<> struct tag<MultiPolygon> { typedef multi_polygon_tag type; }; \
}}}


/*!
\brief \brief_macro{templated multi_polygon}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_MULTI_POLYGON_TEMPLATED, templated multi_polygon}
    \details_macro_templated{multi_polygon, polygon}
\param MultiPolygon \param_macro_type{multi_polygon (without template parameters)}

\qbk{
[heading Example]
[register_multi_polygon_templated]
[register_multi_polygon_templated_output]
}
*/
#define BOOST_GEOMETRY_REGISTER_MULTI_POLYGON_TEMPLATED(MultiPolygon) \
namespace boost { namespace geometry { namespace traits {  \
    template<typename Polygon> struct tag< MultiPolygon<Polygon> > { typedef multi_polygon_tag type; }; \
}}}


#endif // BOOST_GEOMETRY_GEOMETRIES_REGISTER_MULTI_POLYGON_HPP
