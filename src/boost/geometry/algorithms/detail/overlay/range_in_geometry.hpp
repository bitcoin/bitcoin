// Boost.Geometry

// Copyright (c) 2017-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RANGE_IN_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RANGE_IN_GEOMETRY_HPP


#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/iterators/point_iterator.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template
<
    typename Geometry,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct points_range
{
    typedef geometry::point_iterator<Geometry const> iterator_type;

    explicit points_range(Geometry const& geometry)
        : m_geometry(geometry)
    {}

    iterator_type begin() const
    {
        return geometry::points_begin(m_geometry);
    }

    iterator_type end() const
    {
        return geometry::points_end(m_geometry);
    }

    Geometry const& m_geometry;
};
// Specialized because point_iterator doesn't support boxes
template <typename Box>
struct points_range<Box, box_tag>
{
    typedef typename geometry::point_type<Box>::type point_type;
    typedef const point_type * iterator_type;

    explicit points_range(Box const& box)
    {
        detail::assign_box_corners(box,
            m_corners[0], m_corners[1], m_corners[2], m_corners[3]);
    }

    iterator_type begin() const
    {
        return m_corners;
    }

    iterator_type end() const
    {
        return m_corners + 4;
    }

    point_type m_corners[4];
};

template
<
    typename Geometry,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct point_in_geometry_helper
{
    template <typename Point, typename Strategy>
    static inline int apply(Point const& point, Geometry const& geometry,
                            Strategy const& strategy)
    {
        return detail::within::point_in_geometry(point, geometry, strategy);
    }
};
// Specialized because point_in_geometry doesn't support Boxes
template <typename Box>
struct point_in_geometry_helper<Box, box_tag>
{
    template <typename Point, typename Strategy>
    static inline int apply(Point const& point, Box const& box,
                            Strategy const& strategy)
    {
        return geometry::covered_by(point, box, strategy) ? 1 : -1;
    }
};

// This function returns
// when it finds a point of geometry1 inside or outside geometry2
template <typename Geometry1, typename Geometry2, typename Strategy>
static inline int range_in_geometry(Geometry1 const& geometry1,
                                    Geometry2 const& geometry2,
                                    Strategy const& strategy,
                                    bool skip_first = false)
{
    int result = 0;
    points_range<Geometry1> points(geometry1);
    typedef typename points_range<Geometry1>::iterator_type iterator_type;
    iterator_type const end = points.end();
    iterator_type it = points.begin();
    if (it == end)
    {
        return result;
    }
    else if (skip_first)
    {
        ++it;
    }

    for ( ; it != end; ++it)
    {
        result = point_in_geometry_helper<Geometry2>::apply(*it, geometry2, strategy);
        if (result != 0)
        {
            return result;
        }
    }
    // all points contained entirely by the boundary
    return result;
}

// This function returns if first_point1 is inside or outside geometry2 or
// when it finds a point of geometry1 inside or outside geometry2
template <typename Point1, typename Geometry1, typename Geometry2, typename Strategy>
inline int range_in_geometry(Point1 const& first_point1,
                             Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
{
    // check a point on border of geometry1 first
    int result = point_in_geometry_helper<Geometry2>::apply(first_point1, geometry2, strategy);
    if (result == 0)
    {
        // if a point is on boundary of geometry2
        // check points of geometry1 until point inside/outside is found
        // NOTE: skip first point because it should be already tested above
        result = range_in_geometry(geometry1, geometry2, strategy, true);
    }    
    return result;
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RANGE_IN_GEOMETRY_HPP
