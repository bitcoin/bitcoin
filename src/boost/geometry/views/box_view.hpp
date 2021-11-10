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

#ifndef BOOST_GEOMETRY_VIEWS_BOX_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_BOX_VIEW_HPP


#include <array>

#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>


namespace boost { namespace geometry
{

// NOTE: This is equivalent to the previous implementation with detail::points_view.
//       Technically this should not be called a view because it owns the elements.
//       It's also not a borrowed_range because of dangling iterators after the
//       destruction.
//       It's a container or more specifically a ring of some sort, e.g. static_ring.
// NOTE: It would be possible to implement a borrowed_range or a view.
//       The iterators would have to store copies of points.
//       Another possibility is to store the original Box or reference/pointer
//       to Box and index. But then the reference would be the value type
//       so technically they would be InputIterators not RandomAccessIterators.
// NOTE: This object can not represent a Box correctly in all coordinates systems.
//       It's correct only in cartesian CS so maybe it should be removed entirely.


/*!
\brief Makes a box behave like a ring or a range
\details Adapts a box to the Boost.Range concept, enabling the user to iterating
    box corners. The box_view is registered as a Ring Concept
\tparam Box \tparam_geometry{Box}
\tparam Clockwise If true, walks in clockwise direction, otherwise
    it walks in counterclockwise direction
\ingroup views

\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_ring Ring Concept]
}

\qbk{[include reference/views/box_view.qbk]}
*/
template <typename Box, bool Clockwise = true>
struct box_view
{
    using array_t = std::array<typename geometry::point_type<Box>::type, 5>;

    using iterator = typename array_t::const_iterator;
    using const_iterator = typename array_t::const_iterator;

    /// Constructor accepting the box to adapt
    explicit box_view(Box const& box)
    {
        detail::assign_box_corners_oriented<!Clockwise>(box, m_array);
        m_array[4] = m_array[0];
    }

    const_iterator begin() const noexcept { return m_array.begin(); }
    const_iterator end() const noexcept { return m_array.end(); }

private:
    array_t m_array;
};


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS

// All views on boxes are handled as rings
namespace traits
{

template<typename Box, bool Clockwise>
struct tag<box_view<Box, Clockwise> >
{
    typedef ring_tag type;
};

template<typename Box>
struct point_order<box_view<Box, false> >
{
    static order_selector const value = counterclockwise;
};


template<typename Box>
struct point_order<box_view<Box, true> >
{
    static order_selector const value = clockwise;
};

}

#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_BOX_VIEW_HPP
