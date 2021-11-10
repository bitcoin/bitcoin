// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_GRAHAM_ANDREW_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_GRAHAM_ANDREW_HPP


#include <cstddef>
#include <algorithm>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/strategies/convex_hull/cartesian.hpp>
#include <boost/geometry/strategies/convex_hull/geographic.hpp>
#include <boost/geometry/strategies/convex_hull/spherical.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{

template <typename Geometry, typename Point, typename Less>
inline void get_extremes(Geometry const& geometry,
                         Point& left, Point& right,
                         Less const& less)
{
    bool first = true;
    geometry::detail::for_each_range(geometry, [&](auto const& range)
    {
        if (boost::empty(range))
        {
            return;
        }

        // First iterate through this range
        // (this two-stage approach avoids many point copies,
        //  because iterators are kept in memory. Because iterators are
        //  not persistent (in MSVC) this approach is not applicable
        //  for more ranges together)

        auto left_it = boost::begin(range);
        auto right_it = boost::begin(range);

        auto it = boost::begin(range);
        for (++it; it != boost::end(range); ++it)
        {
            if (less(*it, *left_it))
            {
                left_it = it;
            }

            if (less(*right_it, *it))
            {
                right_it = it;
            }
        }

        // Then compare with earlier
        if (first)
        {
            // First time, assign left/right
            left = *left_it;
            right = *right_it;
            first = false;
        }
        else
        {
            // Next time, check if this range was left/right from
            // the extremes already collected
            if (less(*left_it, left))
            {
                left = *left_it;
            }

            if (less(right, *right_it))
            {
                right = *right_it;
            }
        }
    });
}


template
<
    typename Geometry,
    typename Point,
    typename Container,
    typename SideStrategy
>
inline void assign_ranges(Geometry const& geometry,
                          Point const& most_left, Point const& most_right,
                          Container& lower_points, Container& upper_points,
                          SideStrategy const& side)
{
    geometry::detail::for_each_range(geometry, [&](auto const& range)
    {
        // Put points in one of the two output sequences
        for (auto it = boost::begin(range); it != boost::end(range); ++it)
        {
            // check if it is lying most_left or most_right from the line

            int dir = side.apply(most_left, most_right, *it);
            switch(dir)
            {
                case 1 : // left side
                    upper_points.push_back(*it);
                    break;
                case -1 : // right side
                    lower_points.push_back(*it);
                    break;

                // 0: on line most_left-most_right,
                //    or most_left, or most_right,
                //    -> all never part of hull
            }
        }
    });
}


template <typename Range, typename Less>
inline void sort(Range& range, Less const& less)
{
    std::sort(boost::begin(range), boost::end(range), less);
}

} // namespace convex_hull


/*!
\brief Graham scan algorithm to calculate convex hull
 */
template <typename InputGeometry, typename OutputPoint>
class graham_andrew
{
public :
    typedef OutputPoint point_type;
    typedef InputGeometry geometry_type;

private:

    typedef typename cs_tag<point_type>::type cs_tag;

    typedef typename std::vector<point_type> container_type;
    typedef typename std::vector<point_type>::const_iterator iterator;
    typedef typename std::vector<point_type>::const_reverse_iterator rev_iterator;


    class partitions
    {
        friend class graham_andrew;

        container_type m_lower_hull;
        container_type m_upper_hull;
        container_type m_copied_input;
    };


public:
    typedef partitions state_type;

    template <typename Strategy>
    inline void apply(InputGeometry const& geometry,
                      partitions& state,
                      Strategy& strategy) const
    {
        // First pass.
        // Get min/max (in most cases left / right) points
        // This makes use of the geometry::less/greater predicates

        // For the left boundary it is important that multiple points
        // are sorted from bottom to top. Therefore the less predicate
        // does not take the x-only template parameter (this fixes ticket #6019.
        // For the right boundary it is not necessary (though also not harmful),
        // because points are sorted from bottom to top in a later stage.
        // For symmetry and to get often more balanced lower/upper halves
        // we keep it.

        typedef typename geometry::point_type<InputGeometry>::type point_type;

        point_type most_left, most_right;

        // TODO: User-defined CS-specific less-compare
        geometry::less<point_type> less;

        detail::convex_hull::get_extremes(geometry, most_left, most_right, less);

        container_type lower_points, upper_points;

        auto const side_strategy = strategy.side();

        // Bounding left/right points
        // Second pass, now that extremes are found, assign all points
        // in either lower, either upper
        detail::convex_hull::assign_ranges(geometry, most_left, most_right,
                              lower_points, upper_points,
                              side_strategy);

        // Sort both collections, first on x(, then on y)
        detail::convex_hull::sort(lower_points, less);
        detail::convex_hull::sort(upper_points, less);

        // And decide which point should be in the final hull
        build_half_hull<-1>(lower_points, state.m_lower_hull,
                            most_left, most_right,
                            side_strategy);
        build_half_hull<1>(upper_points, state.m_upper_hull,
                           most_left, most_right,
                           side_strategy);
    }


    template <typename OutputIterator>
    inline void result(partitions const& state,
                       OutputIterator out,
                       bool clockwise,
                       bool closed) const
    {
        if (clockwise)
        {
            output_ranges(state.m_upper_hull, state.m_lower_hull, out, closed);
        }
        else
        {
            output_ranges(state.m_lower_hull, state.m_upper_hull, out, closed);
        }
    }


private:

    template <int Factor, typename SideStrategy>
    static inline void build_half_hull(container_type const& input,
            container_type& output,
            point_type const& left, point_type const& right,
            SideStrategy const& side)
    {
        output.push_back(left);
        for(iterator it = input.begin(); it != input.end(); ++it)
        {
            add_to_hull<Factor>(*it, output, side);
        }
        add_to_hull<Factor>(right, output, side);
    }


    template <int Factor, typename SideStrategy>
    static inline void add_to_hull(point_type const& p, container_type& output,
                                   SideStrategy const& side)
    {
        output.push_back(p);
        std::size_t output_size = output.size();
        while (output_size >= 3)
        {
            rev_iterator rit = output.rbegin();
            point_type const last = *rit++;
            point_type const& last2 = *rit++;

            if (Factor * side.apply(*rit, last, last2) <= 0)
            {
                // Remove last two points from stack, and add last again
                // This is much faster then erasing the one but last.
                output.pop_back();
                output.pop_back();
                output.push_back(last);
                output_size--;
            }
            else
            {
                return;
            }
        }
    }


    template <typename OutputIterator>
    static inline void output_ranges(container_type const& first,
                                     container_type const& second,
                                     OutputIterator out,
                                     bool closed)
    {
        std::copy(boost::begin(first), boost::end(first), out);

        BOOST_GEOMETRY_ASSERT(closed ? !boost::empty(second) : boost::size(second) > 1);
        std::copy(++boost::rbegin(second), // skip the first Point
                  closed ? boost::rend(second) : --boost::rend(second), // skip the last Point if open
                  out);

        typedef typename boost::range_size<container_type>::type size_type;
        size_type const count = boost::size(first) + boost::size(second) - 1;
        // count describes a closed case but comparison with min size of closed
        // gives the result compatible also with open
        // here core_detail::closure::minimum_ring_size<closed> could be used
        if (count < 4)
        {
            // there should be only one missing
            *out++ = *boost::begin(first);
        }
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_GRAHAM_ANDREW_HPP
