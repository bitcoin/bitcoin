// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CONCEPT_TYPE_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CONCEPT_TYPE_HPP


#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>


namespace boost { namespace geometry { namespace concepts
{

template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct concept_type
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE("Not implemented for this Tag.", Tag);
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CONCEPT_TYPE_HPP
