// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_BOUNDARY_CHECKER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_BOUNDARY_CHECKER_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/sub_range.hpp>
#include <boost/geometry/algorithms/num_points.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/util/has_nan_coordinate.hpp>
#include <boost/geometry/util/range.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace relate {

enum boundary_query { boundary_front, boundary_back, boundary_any };

template
<
    typename Geometry,
    typename Strategy,
    typename Tag = typename geometry::tag<Geometry>::type
>
class boundary_checker {};

template <typename Geometry, typename Strategy>
class boundary_checker<Geometry, Strategy, linestring_tag>
{
    typedef typename point_type<Geometry>::type point_type;

public:
    boundary_checker(Geometry const& g, Strategy const& s)
        : m_has_boundary( boost::size(g) >= 2
                       && ! detail::equals::equals_point_point(range::front(g),
                                                               range::back(g),
                                                               s) )
#ifdef BOOST_GEOMETRY_DEBUG_RELATE_BOUNDARY_CHECKER
        , m_geometry(g)
#endif
        , m_strategy(s)
    {}

    template <boundary_query BoundaryQuery>
    bool is_endpoint_boundary(point_type const& pt) const
    {
        boost::ignore_unused(pt);
#ifdef BOOST_GEOMETRY_DEBUG_RELATE_BOUNDARY_CHECKER
        // may give false positives for INT
        BOOST_GEOMETRY_ASSERT( (BoundaryQuery == boundary_front || BoundaryQuery == boundary_any)
                   && detail::equals::equals_point_point(pt, range::front(m_geometry), m_strategy)
                   || (BoundaryQuery == boundary_back || BoundaryQuery == boundary_any)
                   && detail::equals::equals_point_point(pt, range::back(m_geometry), m_strategy) );
#endif
        return m_has_boundary;
    }

    Strategy const& strategy() const
    {
        return m_strategy;
    }

private:
    bool m_has_boundary;
#ifdef BOOST_GEOMETRY_DEBUG_RELATE_BOUNDARY_CHECKER
    Geometry const& m_geometry;
#endif
    Strategy const& m_strategy;
};

template <typename Geometry, typename Strategy>
class boundary_checker<Geometry, Strategy, multi_linestring_tag>
{
    typedef typename point_type<Geometry>::type point_type;

public:
    boundary_checker(Geometry const& g, Strategy const& s)
        : m_is_filled(false), m_geometry(g), m_strategy(s)
    {}

    // First call O(NlogN)
    // Each next call O(logN)
    template <boundary_query BoundaryQuery>
    bool is_endpoint_boundary(point_type const& pt) const
    {
        typedef geometry::less<point_type, -1, typename Strategy::cs_tag> less_type;

        typedef typename boost::range_size<Geometry>::type size_type;
        size_type multi_count = boost::size(m_geometry);

        if ( multi_count < 1 )
            return false;

        if ( ! m_is_filled )
        {
            //boundary_points.clear();
            m_boundary_points.reserve(multi_count * 2);

            typedef typename boost::range_iterator<Geometry const>::type multi_iterator;
            for ( multi_iterator it = boost::begin(m_geometry) ;
                  it != boost::end(m_geometry) ; ++ it )
            {
                typename boost::range_reference<Geometry const>::type
                    ls = *it;

                // empty or point - no boundary
                if (boost::size(ls) < 2)
                {
                    continue;
                }

                typedef typename boost::range_reference
                    <
                        typename boost::range_value<Geometry const>::type const
                    >::type point_reference;

                point_reference front_pt = range::front(ls);
                point_reference back_pt = range::back(ls);

                // linear ring or point - no boundary
                if (! equals::equals_point_point(front_pt, back_pt, m_strategy))
                {
                    // do not add points containing NaN coordinates
                    // because they cannot be reasonably compared, e.g. with MSVC
                    // an assertion failure is reported in std::equal_range()
                    if (! geometry::has_nan_coordinate(front_pt))
                    {
                        m_boundary_points.push_back(front_pt);
                    }
                    if (! geometry::has_nan_coordinate(back_pt))
                    {
                        m_boundary_points.push_back(back_pt);
                    }
                }
            }

            std::sort(m_boundary_points.begin(),
                      m_boundary_points.end(),
                      less_type());

            m_is_filled = true;
        }

        std::size_t equal_points_count
            = boost::size(
                std::equal_range(m_boundary_points.begin(),
                                 m_boundary_points.end(),
                                 pt,
                                 less_type())
            );

        return equal_points_count % 2 != 0;// && equal_points_count > 0; // the number is odd and > 0
    }

    Strategy const& strategy() const
    {
        return m_strategy;
    }

private:
    mutable bool m_is_filled;
    // TODO: store references/pointers instead of points?
    mutable std::vector<point_type> m_boundary_points;

    Geometry const& m_geometry;
    Strategy const& m_strategy;
};

}} // namespace detail::relate
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_BOUNDARY_CHECKER_HPP
