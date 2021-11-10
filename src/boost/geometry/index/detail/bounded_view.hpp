// Boost.Geometry Index
//
// This view makes possible to treat some simple primitives as its bounding geometry
// e.g. box, nsphere, etc.
//
// Copyright (c) 2014-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_BOUNDED_VIEW_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_BOUNDED_VIEW_HPP


#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/index/services.hpp>


namespace boost { namespace geometry {

namespace index { namespace detail {


template <typename Geometry, typename BoundingGeometry, typename Strategy>
struct bounded_view_base_cs_tag
{
    typedef typename Strategy::cs_tag type;
};

template <typename Geometry, typename BoundingGeometry>
struct bounded_view_base_cs_tag<Geometry, BoundingGeometry, default_strategy>
    : geometry::cs_tag<Geometry>
{};


template
<
    typename Geometry,
    typename BoundingGeometry,
    typename Strategy,
    typename Tag = typename geometry::tag<Geometry>::type,
    typename BoundingTag = typename geometry::tag<BoundingGeometry>::type,
    typename CSTag = typename bounded_view_base_cs_tag
                        <
                            Geometry, BoundingGeometry, Strategy
                        >::type
>
struct bounded_view_base
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these Geometries.",
        Geometry, BoundingGeometry, Strategy, Tag, BoundingTag, CSTag);
};


// Segment -> Box

template <typename Segment, typename Box, typename Strategy>
struct bounded_view_base<Segment, Box, Strategy, segment_tag, box_tag, cartesian_tag>
{
public:
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    bounded_view_base(Segment const& segment, Strategy const& )
        : m_segment(segment)
    {}
    
    template <std::size_t Dimension>
    inline coordinate_type get_min() const
    {
        return boost::numeric_cast<coordinate_type>(
                (std::min)( geometry::get<0, Dimension>(m_segment),
                            geometry::get<1, Dimension>(m_segment) ) );
    }

    template <std::size_t Dimension>
    inline coordinate_type get_max() const
    {
        return boost::numeric_cast<coordinate_type>(
                (std::max)( geometry::get<0, Dimension>(m_segment),
                            geometry::get<1, Dimension>(m_segment) ) );
    }

private:
    Segment const& m_segment;
};

template <typename Segment, typename Box, typename Strategy, typename CSTag>
struct bounded_view_base<Segment, Box, Strategy, segment_tag, box_tag, CSTag>
{
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    bounded_view_base(Segment const& segment, Strategy const& strategy)
    {
        geometry::envelope(segment, m_box, strategy);
    }

    template <std::size_t Dimension>
    inline coordinate_type get_min() const
    {
        return geometry::get<min_corner, Dimension>(m_box);
    }

    template <std::size_t Dimension>
    inline coordinate_type get_max() const
    {
        return geometry::get<max_corner, Dimension>(m_box);
    }

private:
    Box m_box;
};

// Box -> Box

template <typename BoxIn, typename Box, typename Strategy, typename CSTag>
struct bounded_view_base<BoxIn, Box, Strategy, box_tag, box_tag, CSTag>
{
public:
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    bounded_view_base(BoxIn const& box, Strategy const& )
        : m_box(box)
    {}

    template <std::size_t Dimension>
    inline coordinate_type get_min() const
    {
        return boost::numeric_cast<coordinate_type>(
                geometry::get<min_corner, Dimension>(m_box) );
    }

    template <std::size_t Dimension>
    inline coordinate_type get_max() const
    {
        return boost::numeric_cast<coordinate_type>(
                geometry::get<max_corner, Dimension>(m_box) );
    }

private:
    BoxIn const& m_box;
};

// Point -> Box

template <typename Point, typename Box, typename Strategy, typename CSTag>
struct bounded_view_base<Point, Box, Strategy, point_tag, box_tag, CSTag>
{
public:
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    bounded_view_base(Point const& point, Strategy const& )
        : m_point(point)
    {}

    template <std::size_t Dimension>
    inline coordinate_type get_min() const
    {
        return boost::numeric_cast<coordinate_type>(
                geometry::get<Dimension>(m_point) );
    }

    template <std::size_t Dimension>
    inline coordinate_type get_max() const
    {
        return boost::numeric_cast<coordinate_type>(
                geometry::get<Dimension>(m_point) );
    }

private:
    Point const& m_point;
};


template <typename Geometry,
          typename BoundingGeometry,
          typename Strategy,
          typename Tag = typename geometry::tag<Geometry>::type,
          typename BoundingTag = typename geometry::tag<BoundingGeometry>::type>
struct bounded_view
    : bounded_view_base<Geometry, BoundingGeometry, Strategy>
{
    typedef bounded_view_base<Geometry, BoundingGeometry, Strategy> base_type;

    bounded_view(Geometry const& geometry, Strategy const& strategy)
        : base_type(geometry, strategy)
    {}
};

template <typename Geometry,
          typename BoundingGeometry,
          typename Tag,
          typename BoundingTag>
struct bounded_view<Geometry, BoundingGeometry, default_strategy, Tag, BoundingTag>
    : bounded_view_base
        <
            Geometry,
            BoundingGeometry,
            typename strategies::index::services::default_strategy<Geometry>::type
        >
{
    typedef typename strategies::index::services::default_strategy
        <
            Geometry
        >::type strategy_type;

    typedef bounded_view_base
        <
            Geometry,
            BoundingGeometry,
            strategy_type
        > base_type;

    explicit bounded_view(Geometry const& geometry, default_strategy const& )
        : base_type(geometry, strategy_type())
    {}
};


}} // namespace index::detail

// XXX -> Box

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Geometry, typename Box, typename Strategy, typename Tag>
struct tag< index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag> >
{
    typedef box_tag type;
};

template <typename Geometry, typename Box, typename Strategy, typename Tag>
struct point_type< index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag> >
{
    typedef typename point_type<Box>::type type;
};

template <typename Geometry, typename Box, typename Strategy, typename Tag, std::size_t Dimension>
struct indexed_access<index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag>,
                      min_corner, Dimension>
{
    typedef index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag> box_type;
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    static inline coordinate_type get(box_type const& b)
    {
        return b.template get_min<Dimension>();
    }

    //static inline void set(box_type & b, coordinate_type const& value)
    //{
    //    BOOST_GEOMETRY_INDEX_ASSERT(false, "unable to modify a box through view");
    //}
};

template <typename Geometry, typename Box, typename Strategy, typename Tag, std::size_t Dimension>
struct indexed_access<index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag>,
                      max_corner, Dimension>
{
    typedef index::detail::bounded_view<Geometry, Box, Strategy, Tag, box_tag> box_type;
    typedef typename geometry::coordinate_type<Box>::type coordinate_type;

    static inline coordinate_type get(box_type const& b)
    {
        return b.template get_max<Dimension>();
    }

    //static inline void set(box_type & b, coordinate_type const& value)
    //{
    //    BOOST_GEOMETRY_INDEX_ASSERT(false, "unable to modify a box through view");
    //}
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_INDEX_DETAIL_BOUNDED_VIEW_HPP
