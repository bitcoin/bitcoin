// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_REGISTER_RING_HPP
#define BOOST_GEOMETRY_GEOMETRIES_REGISTER_RING_HPP


#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

/*!
\brief \brief_macro{ring}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_RING, ring} The
    ring may contain template parameters, which must be specified then.
\param Ring \param_macro_type{ring}

\qbk{
[heading Example]
[register_ring]
[register_ring_output]
}
*/
#define BOOST_GEOMETRY_REGISTER_RING(Ring) \
namespace boost { namespace geometry { namespace traits {  \
    template<> struct tag<Ring> { typedef ring_tag type; }; \
}}}


/*!
\brief \brief_macro{templated ring}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_RING_TEMPLATED, templated ring}
    \details_macro_templated{ring, point}
\param Ring \param_macro_type{ring (without template parameters)}

\qbk{
[heading Example]
[register_ring_templated]
[register_ring_templated_output]
}
*/
#define BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(Ring) \
namespace boost { namespace geometry { namespace traits {  \
    template<typename P> struct tag< Ring<P> > { typedef ring_tag type; }; \
}}}


#endif // BOOST_GEOMETRY_GEOMETRIES_REGISTER_RING_HPP
