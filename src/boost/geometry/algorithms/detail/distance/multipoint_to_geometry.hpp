// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTIPOINT_TO_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTIPOINT_TO_GEOMETRY_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/distance/range_to_geometry_rtree.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/tags.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename MultiPoint1, typename MultiPoint2, typename Strategies>
struct multipoint_to_multipoint
{
    typedef distance::return_t<MultiPoint1, MultiPoint2, Strategies> return_type;

    static inline return_type apply(MultiPoint1 const& multipoint1,
                                    MultiPoint2 const& multipoint2,
                                    Strategies const& strategies)
    {
        if (boost::size(multipoint2) < boost::size(multipoint1))

        {
            return point_or_segment_range_to_geometry_rtree
                <
                    typename boost::range_iterator<MultiPoint2 const>::type,
                    MultiPoint1,
                    Strategies
                >::apply(boost::begin(multipoint2),
                         boost::end(multipoint2),
                         multipoint1,
                         strategies);
        }

        return point_or_segment_range_to_geometry_rtree
            <
                typename boost::range_iterator<MultiPoint1 const>::type,
                MultiPoint2,
                Strategies
            >::apply(boost::begin(multipoint1),
                     boost::end(multipoint1),
                     multipoint2,
                     strategies);
    }
};


template <typename MultiPoint, typename Linear, typename Strategies>
struct multipoint_to_linear
{
    static inline auto apply(MultiPoint const& multipoint,
                             Linear const& linear,
                             Strategies const& strategies)
    {
        return detail::distance::point_or_segment_range_to_geometry_rtree
            <
                typename boost::range_iterator<MultiPoint const>::type,
                Linear,
                Strategies
            >::apply(boost::begin(multipoint),
                     boost::end(multipoint),
                     linear,
                     strategies);
    }

    static inline auto apply(Linear const& linear,
                             MultiPoint const& multipoint,
                             Strategies const& strategies)
    {
        return apply(multipoint, linear, strategies);
    }
};


template <typename MultiPoint, typename Areal, typename Strategies>
class multipoint_to_areal
{
private:
    struct not_covered_by_areal
    {
        not_covered_by_areal(Areal const& areal, Strategies const& strategy)
            : m_areal(areal), m_strategy(strategy)
        {}

        template <typename Point>
        inline bool apply(Point const& point) const
        {
            return !geometry::covered_by(point, m_areal, m_strategy);
        }

        Areal const& m_areal;
        Strategies const& m_strategy;
    };

public:
    typedef distance::return_t<MultiPoint, Areal, Strategies> return_type;

    static inline return_type apply(MultiPoint const& multipoint,
                                    Areal const& areal,
                                    Strategies const& strategies)
    {
        not_covered_by_areal predicate(areal, strategies);

        if (check_iterator_range
                <
                    not_covered_by_areal, false
                >::apply(boost::begin(multipoint),
                         boost::end(multipoint),
                         predicate))
        {
            return detail::distance::point_or_segment_range_to_geometry_rtree
                <
                    typename boost::range_iterator<MultiPoint const>::type,
                    Areal,
                    Strategies
                >::apply(boost::begin(multipoint),
                         boost::end(multipoint),
                         areal,
                         strategies);
        }
        return return_type(0);
    }

    static inline return_type apply(Areal const& areal,
                                    MultiPoint const& multipoint,
                                    Strategies const& strategies)
    {
        return apply(multipoint, areal, strategies);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiPoint1, typename MultiPoint2, typename Strategy>
struct distance
    <
        MultiPoint1, MultiPoint2, Strategy,
        multi_point_tag, multi_point_tag,
        strategy_tag_distance_point_point, false
    > : detail::distance::multipoint_to_multipoint
        <
            MultiPoint1, MultiPoint2, Strategy
        >
{};


template <typename MultiPoint, typename Linear, typename Strategy>
struct distance
    <
         MultiPoint, Linear, Strategy, multi_point_tag, linear_tag,
         strategy_tag_distance_point_segment, false
    > : detail::distance::multipoint_to_linear<MultiPoint, Linear, Strategy>
{};


template <typename Linear, typename MultiPoint, typename Strategy>
struct distance
    <
         Linear, MultiPoint, Strategy, linear_tag, multi_point_tag,
         strategy_tag_distance_point_segment, false
    > : detail::distance::multipoint_to_linear<MultiPoint, Linear, Strategy>
{};


template <typename MultiPoint, typename Areal, typename Strategy>
struct distance
    <
         MultiPoint, Areal, Strategy, multi_point_tag, areal_tag,
         strategy_tag_distance_point_segment, false
    > : detail::distance::multipoint_to_areal<MultiPoint, Areal, Strategy>
{};


template <typename Areal, typename MultiPoint, typename Strategy>
struct distance
    <
         Areal, MultiPoint, Strategy, areal_tag, multi_point_tag,
         strategy_tag_distance_point_segment, false
    > : detail::distance::multipoint_to_areal<MultiPoint, Areal, Strategy>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTIPOINT_TO_GEOMETRY_HPP
