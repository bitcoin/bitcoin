// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_VALUE_TYPE_HPP
#define BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_VALUE_TYPE_HPP

#include <iterator>

#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/pointing_segment.hpp>
#include <boost/geometry/iterators/point_iterator.hpp>
#include <boost/geometry/util/type_traits_std.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace segment_iterator
{

template <typename Geometry>
struct value_type
{
    typedef typename std::iterator_traits
        <
            geometry::point_iterator<Geometry>
        >::reference point_iterator_reference_type;

    typedef typename detail::point_iterator::value_type
        <
            Geometry
        >::type point_iterator_value_type;

    // If the reference type of the point iterator is not really a
    // reference, then dereferencing a point iterator would create
    // a temporary object.
    // In this case using a pointing_segment to represent the
    // dereferenced value of the segment iterator cannot be used, as
    // it would store pointers to temporary objects. Instead we use a
    // segment, which does a full copy of the temporary objects
    // returned by the point iterator.
    typedef std::conditional_t
        <
            std::is_reference<point_iterator_reference_type>::value,
            geometry::model::pointing_segment<point_iterator_value_type>,
            geometry::model::segment
                <
                    typename util::remove_cptrref
                        <
                            point_iterator_value_type
                        >::type
                >
        > type;
};

}} // namespace detail::segment_iterator
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_VALUE_TYPE_HPP
