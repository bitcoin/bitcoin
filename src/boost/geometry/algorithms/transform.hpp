// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_TRANSFORM_HPP
#define BOOST_GEOMETRY_ALGORITHMS_TRANSFORM_HPP

#include <cmath>
#include <iterator>
#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/num_interior_rings.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/transform.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace transform
{

struct transform_point
{
    template <typename Point1, typename Point2, typename Strategy>
    static inline bool apply(Point1 const& p1, Point2& p2,
                Strategy const& strategy)
    {
        return strategy.apply(p1, p2);
    }
};


struct transform_box
{
    template <typename Box1, typename Box2, typename Strategy>
    static inline bool apply(Box1 const& b1, Box2& b2,
                Strategy const& strategy)
    {
        typedef typename point_type<Box1>::type point_type1;
        typedef typename point_type<Box2>::type point_type2;

        point_type1 lower_left, upper_right;
        geometry::detail::assign::assign_box_2d_corner<min_corner, min_corner>(
                    b1, lower_left);
        geometry::detail::assign::assign_box_2d_corner<max_corner, max_corner>(
                    b1, upper_right);

        point_type2 p1, p2;
        if (strategy.apply(lower_left, p1) && strategy.apply(upper_right, p2))
        {
            // Create a valid box and therefore swap if necessary
            typedef typename coordinate_type<point_type2>::type coordinate_type;
            coordinate_type x1 = geometry::get<0>(p1)
                    , y1  = geometry::get<1>(p1)
                    , x2  = geometry::get<0>(p2)
                    , y2  = geometry::get<1>(p2);

            if (x1 > x2) { std::swap(x1, x2); }
            if (y1 > y2) { std::swap(y1, y2); }

            geometry::set<min_corner, 0>(b2, x1);
            geometry::set<min_corner, 1>(b2, y1);
            geometry::set<max_corner, 0>(b2, x2);
            geometry::set<max_corner, 1>(b2, y2);

            return true;
        }
        return false;
    }
};

struct transform_box_or_segment
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& source, Geometry2& target,
                Strategy const& strategy)
    {
        typedef typename point_type<Geometry1>::type point_type1;
        typedef typename point_type<Geometry2>::type point_type2;

        point_type1 source_point[2];
        geometry::detail::assign_point_from_index<0>(source, source_point[0]);
        geometry::detail::assign_point_from_index<1>(source, source_point[1]);

        point_type2 target_point[2];
        if (strategy.apply(source_point[0], target_point[0])
            && strategy.apply(source_point[1], target_point[1]))
        {
            geometry::detail::assign_point_to_index<0>(target_point[0], target);
            geometry::detail::assign_point_to_index<1>(target_point[1], target);
            return true;
        }
        return false;
    }
};


template
<
    typename PointOut,
    typename OutputIterator,
    typename Range,
    typename Strategy
>
inline bool transform_range_out(Range const& range,
    OutputIterator out, Strategy const& strategy)
{
    PointOut point_out;
    for(typename boost::range_iterator<Range const>::type
        it = boost::begin(range);
        it != boost::end(range);
        ++it)
    {
        if (! transform_point::apply(*it, point_out, strategy))
        {
            return false;
        }
        *out++ = point_out;
    }
    return true;
}


struct transform_polygon
{
    template <typename Polygon1, typename Polygon2, typename Strategy>
    static inline bool apply(Polygon1 const& poly1, Polygon2& poly2,
                Strategy const& strategy)
    {
        typedef typename point_type<Polygon2>::type point2_type;

        geometry::clear(poly2);

        if (!transform_range_out<point2_type>(geometry::exterior_ring(poly1),
                    range::back_inserter(geometry::exterior_ring(poly2)), strategy))
        {
            return false;
        }

        // Note: here a resizeable container is assumed.
        traits::resize
            <
                typename std::remove_reference
                <
                    typename traits::interior_mutable_type<Polygon2>::type
                >::type
            >::apply(geometry::interior_rings(poly2),
                     geometry::num_interior_rings(poly1));

        typename geometry::interior_return_type<Polygon1 const>::type
            rings1 = geometry::interior_rings(poly1);
        typename geometry::interior_return_type<Polygon2>::type
            rings2 = geometry::interior_rings(poly2);

        typename detail::interior_iterator<Polygon1 const>::type
            it1 = boost::begin(rings1);
        typename detail::interior_iterator<Polygon2>::type
            it2 = boost::begin(rings2);
        for ( ; it1 != boost::end(rings1); ++it1, ++it2)
        {
            if ( ! transform_range_out<point2_type>(*it1,
                                                    range::back_inserter(*it2),
                                                    strategy) )
            {
                return false;
            }
        }

        return true;
    }
};


template <typename Point1, typename Point2>
struct select_strategy
{
    typedef typename strategy::transform::services::default_strategy
        <
            typename cs_tag<Point1>::type,
            typename cs_tag<Point2>::type,
            typename coordinate_system<Point1>::type,
            typename coordinate_system<Point2>::type,
            dimension<Point1>::type::value,
            dimension<Point2>::type::value,
            typename point_type<Point1>::type,
            typename point_type<Point2>::type
        >::type type;
};

struct transform_range
{
    template <typename Range1, typename Range2, typename Strategy>
    static inline bool apply(Range1 const& range1,
            Range2& range2, Strategy const& strategy)
    {
        typedef typename point_type<Range2>::type point_type;

        // Should NOT be done here!
        // geometry::clear(range2);
        return transform_range_out<point_type>(range1,
                range::back_inserter(range2), strategy);
    }
};


/*!
    \brief Is able to transform any multi-geometry, calling the single-version as policy
*/
template <typename Policy>
struct transform_multi
{
    template <typename Multi1, typename Multi2, typename S>
    static inline bool apply(Multi1 const& multi1, Multi2& multi2, S const& strategy)
    {
        traits::resize<Multi2>::apply(multi2, boost::size(multi1));

        typename boost::range_iterator<Multi1 const>::type it1
                = boost::begin(multi1);
        typename boost::range_iterator<Multi2>::type it2
                = boost::begin(multi2);

        for (; it1 != boost::end(multi1); ++it1, ++it2)
        {
            if (! Policy::apply(*it1, *it2, strategy))
            {
                return false;
            }
        }

        return true;
    }
};


}} // namespace detail::transform
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1, typename Geometry2,
    typename Tag1 = typename tag_cast<typename tag<Geometry1>::type, multi_tag>::type,
    typename Tag2 = typename tag_cast<typename tag<Geometry2>::type, multi_tag>::type
>
struct transform {};

template <typename Point1, typename Point2>
struct transform<Point1, Point2, point_tag, point_tag>
    : detail::transform::transform_point
{
};


template <typename Linestring1, typename Linestring2>
struct transform
    <
        Linestring1, Linestring2,
        linestring_tag, linestring_tag
    >
    : detail::transform::transform_range
{
};

template <typename Range1, typename Range2>
struct transform<Range1, Range2, ring_tag, ring_tag>
    : detail::transform::transform_range
{
};

template <typename Polygon1, typename Polygon2>
struct transform<Polygon1, Polygon2, polygon_tag, polygon_tag>
    : detail::transform::transform_polygon
{
};

template <typename Box1, typename Box2>
struct transform<Box1, Box2, box_tag, box_tag>
    : detail::transform::transform_box
{
};

template <typename Segment1, typename Segment2>
struct transform<Segment1, Segment2, segment_tag, segment_tag>
    : detail::transform::transform_box_or_segment
{
};

template <typename Multi1, typename Multi2>
struct transform
    <
        Multi1, Multi2,
        multi_tag, multi_tag
    >
    : detail::transform::transform_multi
        <
            dispatch::transform
                <
                    typename boost::range_value<Multi1>::type,
                    typename boost::range_value<Multi2>::type
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

struct transform
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2& geometry2,
                             Strategy const& strategy)
    {
        concepts::check<Geometry1 const>();
        concepts::check<Geometry2>();

        return dispatch::transform<Geometry1, Geometry2>::apply(
            geometry1,
            geometry2,
            strategy
        );
    }

    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2& geometry2,
                             default_strategy)
    {
        return apply(
            geometry1,
            geometry2,
            typename detail::transform::select_strategy<Geometry1, Geometry2>::type()
        );
    }
};

} // namespace resolve_strategy


namespace resolve_variant {

template <typename Geometry1, typename Geometry2>
struct transform
{
    template <typename Strategy>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2& geometry2,
                             Strategy const& strategy)
    {
        return resolve_strategy::transform::apply(
            geometry1,
            geometry2,
            strategy
        );
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct transform<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    template <typename Strategy>
    struct visitor: static_visitor<bool>
    {
        Geometry2& m_geometry2;
        Strategy const& m_strategy;

        visitor(Geometry2& geometry2, Strategy const& strategy)
            : m_geometry2(geometry2)
            , m_strategy(strategy)
        {}

        template <typename Geometry1>
        inline bool operator()(Geometry1 const& geometry1) const
        {
            return transform<Geometry1, Geometry2>::apply(
                geometry1,
                m_geometry2,
                m_strategy
            );
        }
    };

    template <typename Strategy>
    static inline bool apply(
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
        Geometry2& geometry2,
        Strategy const& strategy
    )
    {
        return boost::apply_visitor(visitor<Strategy>(geometry2, strategy), geometry1);
    }
};

} // namespace resolve_variant


/*!
\brief Transforms from one geometry to another geometry  \brief_strategy
\ingroup transform
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy strategy
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy The strategy to be used for transformation
\return True if the transformation could be done

\qbk{distinguish,with strategy}

\qbk{[include reference/algorithms/transform_with_strategy.qbk]}
 */
template <typename Geometry1, typename Geometry2, typename Strategy>
inline bool transform(Geometry1 const& geometry1, Geometry2& geometry2,
            Strategy const& strategy)
{
    return resolve_variant::transform<Geometry1, Geometry2>
                          ::apply(geometry1, geometry2, strategy);
}


/*!
\brief Transforms from one geometry to another geometry using a strategy
\ingroup transform
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return True if the transformation could be done

\qbk{[include reference/algorithms/transform.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline bool transform(Geometry1 const& geometry1, Geometry2& geometry2)
{
    return geometry::transform(geometry1, geometry2, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_TRANSFORM_HPP
