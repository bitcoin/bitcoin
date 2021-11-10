// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2019-2019 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_AS_SUBRANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_AS_SUBRANGE_HPP


#include <cstddef>
#include <map>

#include <boost/geometry/core/access.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Segment>
struct segment_as_subrange
{
    segment_as_subrange(Segment const& s)
        : m_segment(s)
    {
        geometry::set<0>(m_p1, geometry::get<0, 0>(m_segment));
        geometry::set<1>(m_p1, geometry::get<0, 1>(m_segment));
        geometry::set<0>(m_p2, geometry::get<1, 0>(m_segment));
        geometry::set<1>(m_p2, geometry::get<1, 1>(m_segment));
    }

    typedef typename geometry::point_type<Segment>::type point_type;

    point_type const& at(std::size_t index) const
    {
        return index == 0 ? m_p1 : m_p2;
    }

    static inline bool is_last_segment()
    {
        return true;
    }

    point_type m_p1, m_p2;

    Segment const& m_segment;
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_AS_SUBRANGE_HPP
