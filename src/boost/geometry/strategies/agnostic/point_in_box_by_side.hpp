// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2018-2020.
// Modifications copyright (c) 2018-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP
#define BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP

#include <boost/array.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>
#include <boost/geometry/strategies/geographic/side.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>


namespace boost { namespace geometry { namespace strategy
{

namespace within
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct decide_within
{
    static inline bool apply(int side, bool& result)
    {
        if (side != 1)
        {
            result = false;
            return false;
        }
        return true; // continue
    }
};

struct decide_covered_by
{
    static inline bool apply(int side, bool& result)
    {
        if (side != 1)
        {
            result = side >= 0;
            return false;
        }
        return true; // continue
    }
};


// WARNING
// This strategy is not suitable for boxes in non-cartesian CSes having edges
// longer than 180deg because e.g. the SSF formula picks the side of the closer
// longitude, so for long edges the side is the opposite.
// Actually it is not suitable for shorter edges either because the edges of
// boxes are defined by meridians and parallels, not great circles or geodesics.
template <typename Decide, typename Point, typename Box, typename Strategy>
inline bool point_in_box_by_side(Point const& point, Box const& box,
                                 Strategy const& strategy)
{
    boost::ignore_unused(strategy);

    // Create (counterclockwise) array of points, the fifth one closes it
    // Every point should be on the LEFT side (=1), or ON the border (=0),
    // So >= 1 or >= 0
    boost::array<typename point_type<Box>::type, 5> bp;
    geometry::detail::assign_box_corners_oriented<true>(box, bp);
    bp[4] = bp[0];

    bool result = true;
    for (int i = 1; i < 5; i++)
    {
        int const side = strategy.apply(point, bp[i - 1], bp[i]);
        if (! Decide::apply(side, result))
        {
            return result;
        }
    }

    return result;
}


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


// There should probably be another category of geometry different than box,
// e.g. rectangle or convex_ring. This strategy should probably be an
// algorithm calling side strategy.

template <typename CalculationType = void>
struct cartesian_point_box_by_side
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_within
            >(point, box,
              strategy::side::side_by_triangle<CalculationType>());
    }
};

template <typename CalculationType = void>
struct spherical_point_box_by_side
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_within
            >(point, box,
              strategy::side::spherical_side_formula<CalculationType>());
    }
};

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
struct geographic_point_box_by_side
{
    geographic_point_box_by_side() = default;

    explicit geographic_point_box_by_side(Spheroid const& spheroid)
        : m_side(spheroid)
    {}

    template <typename Point, typename Box>
    bool apply(Point const& point, Box const& box) const
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_within
            >(point, box, m_side);
    }

    Spheroid const& model() const
    {
        return m_side.model();
    }

private:
    strategy::side::geographic
        <
            FormulaPolicy, Spheroid, CalculationType
        > m_side;
};


} // namespace within


namespace covered_by
{


template <typename CalculationType = void>
struct cartesian_point_box_by_side
{
    template <typename Point, typename Box>
    static bool apply(Point const& point, Box const& box)
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_covered_by
            >(point, box,
              strategy::side::side_by_triangle<CalculationType>());
    }
};

template <typename CalculationType = void>
struct spherical_point_box_by_side
{
    template <typename Point, typename Box>
    static bool apply(Point const& point, Box const& box)
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_covered_by
            >(point, box,
              strategy::side::spherical_side_formula<CalculationType>());
    }
};

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
struct geographic_point_box_by_side
{
    geographic_point_box_by_side() = default;

    explicit geographic_point_box_by_side(Spheroid const& spheroid)
        : m_side(spheroid)
    {}

    template <typename Point, typename Box>
    bool apply(Point const& point, Box const& box) const
    {
        return within::detail::point_in_box_by_side
            <
                within::detail::decide_covered_by
            >(point, box, m_side);
    }

    Spheroid const& model() const
    {
        return m_side.model();
    }

private:
    strategy::side::geographic
        <
            FormulaPolicy, Spheroid, CalculationType
        > m_side;
};


}


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP
