// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_TAG_HPP
#define BOOST_GEOMETRY_CORE_TAG_HPP


#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry
{

namespace traits
{

/*!
\brief Traits class to attach a tag to a geometry
\details All geometries should implement a traits::tag<G>::type metafunction to indicate their
    own geometry type.
\ingroup traits
\par Geometries:
    - all geometries
\par Specializations should provide:
    - typedef XXX_tag type; (point_tag, box_tag, ...)
\tparam Geometry geometry
*/
template <typename Geometry, typename Enable = void>
struct tag
{
    typedef void type;
};

} // namespace traits



/*!
\brief \brief_meta{type, tag, \meta_geometry_type}
\details With Boost.Geometry, tags are the driving force of the tag dispatching
    mechanism. The tag metafunction is therefore used in every free function.
\tparam Geometry \tparam_geometry
\ingroup core

\qbk{[include reference/core/tag.qbk]}
*/
template <typename Geometry>
struct tag
{
    typedef typename traits::tag
        <
            typename util::remove_cptrref<Geometry>::type
        >::type type;
};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_TAG_HPP
