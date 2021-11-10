// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_AREAL_AREAL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_AREAL_AREAL_HPP


#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/detail/intersection/interface.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{


template
<
    typename GeometryOut,
    typename OutTag = typename geometry::detail::setop_insert_output_tag
                        <
                            typename geometry::detail::output_geometry_value
                                <
                                    GeometryOut
                                >::type
                        >::type
>
struct intersection_areal_areal_
{
    template
    <
        typename Areal1,
        typename Areal2,
        typename RobustPolicy,
        typename Strategy
    >
    static inline void apply(Areal1 const& areal1,
                             Areal2 const& areal2,
                             RobustPolicy const& robust_policy,
                             GeometryOut& geometry_out,
                             Strategy const& strategy)
    {
        geometry::dispatch::intersection_insert
            <
                Areal1, Areal2,
                typename boost::range_value<GeometryOut>::type,
                overlay_intersection
            >::apply(areal1, areal2, robust_policy,
                     geometry::range::back_inserter(geometry_out),
                     strategy);
    }
};

// TODO: Ideally this should be done in one call of intersection_insert
//   just like it's done for all other combinations
template <typename TupledOut>
struct intersection_areal_areal_<TupledOut, tupled_output_tag>
{
    template
    <
        typename Areal1,
        typename Areal2,
        typename RobustPolicy,
        typename Strategy
    >
    static inline void apply(Areal1 const& areal1,
                             Areal2 const& areal2,
                             RobustPolicy const& robust_policy,
                             TupledOut& geometry_out,
                             Strategy const& strategy)
    {
        typedef typename geometry::detail::output_geometry_value
            <
                TupledOut
            >::type single_out;

        boost::ignore_unused
            <
                geometry::detail::expect_output
                    <
                        Areal1, Areal2, single_out,
                        point_tag, linestring_tag, polygon_tag
                    >
            >();

        typedef geometry::detail::output_geometry_access
            <
                single_out, polygon_tag, polygon_tag
            > areal;
        typedef geometry::detail::output_geometry_access
            <
                single_out, linestring_tag, linestring_tag
            > linear;
        typedef geometry::detail::output_geometry_access
            <
                single_out, point_tag, point_tag
            > pointlike;

        typedef typename geometry::tuples::element
            <
                areal::index, TupledOut
            >::type areal_out_type;

        // NOTE: The same robust_policy is used in each call of
        //   intersection_insert. Is that correct?

        // A * A -> A
        call_intersection(areal1, areal2, robust_policy,
                          areal::get(geometry_out),
                          strategy);

        bool const is_areal_empty = boost::empty(areal::get(geometry_out));
        TupledOut temp_out;

        // L * L -> (L, P)
        call_intersection(geometry::detail::boundary_view<Areal1 const>(areal1),
                          geometry::detail::boundary_view<Areal2 const>(areal2),
                          robust_policy,
                          ! is_areal_empty
                            ? temp_out
                            : geometry_out,
                          strategy);

        if (! is_areal_empty)
        {
            // NOTE: the original areal geometry could be used instead of boundary here
            //   however this results in static assert failure related to rescale policy
            typedef geometry::detail::boundary_view
                <
                    areal_out_type const
                > areal_out_boundary_type;

            areal_out_boundary_type areal_out_boundary(areal::get(geometry_out));

            // L - L -> L
            call_difference(linear::get(temp_out),
                            areal_out_boundary,
                            robust_policy,
                            linear::get(geometry_out),
                            strategy);

            // P - L -> P
            call_difference(pointlike::get(temp_out),
                            areal_out_boundary,
                            robust_policy,
                            pointlike::get(geometry_out),
                            strategy);
        }
        
        return;
    }

private:
    template
    <
        typename Geometry1,
        typename Geometry2,
        typename RobustPolicy,
        typename GeometryOut,
        typename Strategy
    >
    static inline void call_intersection(Geometry1 const& geometry1,
                                         Geometry2 const& geometry2,
                                         RobustPolicy const& robust_policy,
                                         GeometryOut& geometry_out,
                                         Strategy const& strategy)
    {
        geometry::dispatch::intersection_insert
            <
                Geometry1,
                Geometry2,
                typename geometry::detail::output_geometry_value
                    <
                        GeometryOut
                    >::type,
                overlay_intersection
            >::apply(geometry1,
                     geometry2,
                     robust_policy,
                     geometry::detail::output_geometry_back_inserter(geometry_out),
                     strategy);
    }

    template
    <
        typename Geometry1,
        typename Geometry2,
        typename RobustPolicy,
        typename GeometryOut,
        typename Strategy
    >
    static inline void call_difference(Geometry1 const& geometry1,
                                       Geometry2 const& geometry2,
                                       RobustPolicy const& robust_policy,
                                       GeometryOut& geometry_out,
                                       Strategy const& strategy)
    {
        geometry::dispatch::intersection_insert
            <
                Geometry1,
                Geometry2,
                typename boost::range_value<GeometryOut>::type,
                overlay_difference
            >::apply(geometry1,
                     geometry2,
                     robust_policy,
                     geometry::range::back_inserter(geometry_out),
                     strategy);
    }
};


struct intersection_areal_areal
{
    template
    <
        typename Areal1,
        typename Areal2,
        typename RobustPolicy,
        typename GeometryOut,
        typename Strategy
    >
    static inline bool apply(Areal1 const& areal1,
                             Areal2 const& areal2,
                             RobustPolicy const& robust_policy,
                             GeometryOut& geometry_out,
                             Strategy const& strategy)
    {
        intersection_areal_areal_
            <
                GeometryOut
            >::apply(areal1, areal2, robust_policy, geometry_out, strategy);

        return true;
    }
};


}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Polygon1, typename Polygon2
>
struct intersection
    <
        Polygon1, Polygon2,
        polygon_tag, polygon_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};

template
<
    typename Polygon, typename Ring
>
struct intersection
    <
        Polygon, Ring,
        polygon_tag, ring_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};

template
<
    typename Ring1, typename Ring2
>
struct intersection
    <
        Ring1, Ring2,
        ring_tag, ring_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};

template
<
    typename Polygon, typename MultiPolygon
>
struct intersection
    <
        Polygon, MultiPolygon,
        polygon_tag, multi_polygon_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};

template
<
    typename MultiPolygon, typename Ring
>
struct intersection
    <
        MultiPolygon, Ring,
        multi_polygon_tag, ring_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};

template
<
    typename MultiPolygon1, typename MultiPolygon2
>
struct intersection
    <
        MultiPolygon1, MultiPolygon2,
        multi_polygon_tag, multi_polygon_tag,
        false
    >
    : detail::intersection::intersection_areal_areal
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_AREAL_AREAL_HPP
