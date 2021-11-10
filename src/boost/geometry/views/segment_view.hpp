// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_VIEWS_SEGMENT_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_SEGMENT_VIEW_HPP


#include <array>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>


namespace boost { namespace geometry
{

// NOTE: This is equivalent to the previous implementation with detail::points_view.
//       Technically this should not be called a view because it owns the elements.
//       It's also not a borrowed_range because of dangling iterators after the
//       destruction.
//       It's a container or more specifically a linestring of some sort, e.g. static_linestring.
// NOTE: It would be possible to implement a borrowed_range or a view.
//       The iterators would have to store copies of points.
//       Another possibility is to store the original Segment or reference/pointer
//       to Segment and index. But then the reference would be the value type
//       so technically they would be InputIterators not RandomAccessIterators.


/*!
\brief Makes a segment behave like a linestring or a range
\details Adapts a segment to the Boost.Range concept, enabling the user to
    iterate the two segment points. The segment_view is registered as a LineString Concept
\tparam Segment \tparam_geometry{Segment}
\ingroup views

\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_linestring LineString Concept]
}

\qbk{[include reference/views/segment_view.qbk]}

*/
template <typename Segment>
struct segment_view
{
    using array_t = std::array<typename geometry::point_type<Segment>::type, 2>;

    using iterator = typename array_t::const_iterator;
    using const_iterator = typename array_t::const_iterator;

    /// Constructor accepting the segment to adapt
    explicit segment_view(Segment const& segment)
    {
        geometry::detail::assign_point_from_index<0>(segment, m_array[0]);
        geometry::detail::assign_point_from_index<1>(segment, m_array[1]);
    }

    const_iterator begin() const noexcept { return m_array.begin(); }
    const_iterator end() const noexcept { return m_array.end(); }

private:
    array_t m_array;
};


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS

// All segment ranges can be handled as linestrings
namespace traits
{

template<typename Segment>
struct tag<segment_view<Segment> >
{
    typedef linestring_tag type;
};

}

#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_SEGMENT_VIEW_HPP
