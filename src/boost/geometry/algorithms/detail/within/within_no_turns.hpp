// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2013.
// Modifications copyright (c) 2013, Oracle and/or its affiliates.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_WITHIN_NO_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_WITHIN_NO_TURNS_HPP

#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/within/point_in_geometry.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail_dispatch { namespace within {

// returns true if G1 is within G2
// this function should be called only if there are no intersection points
// otherwise it may return invalid result
// e.g. when non-first point of G1 is outside G2 or when some rings of G1 are the same as rings of G2

template <typename Geometry1,
          typename Geometry2,
          typename Tag1 = typename geometry::tag<Geometry1>::type,
          typename Tag2 = typename geometry::tag<Geometry2>::type>
struct within_no_turns
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        typedef typename geometry::point_type<Geometry1>::type point1_type;
        point1_type p;
        if ( !geometry::point_on_border(p, geometry1) )
            return false;

        return detail::within::point_in_geometry(p, geometry2, strategy) >= 0;
    }
};

template <typename Geometry1, typename Geometry2>
struct within_no_turns<Geometry1, Geometry2, ring_tag, polygon_tag>
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        typedef typename geometry::point_type<Geometry1>::type point1_type;
        typedef typename geometry::point_type<Geometry2>::type point2_type;
        point1_type p;
        if ( !geometry::point_on_border(p, geometry1) )
            return false;
        // check if one of ring points is outside the polygon
        if ( detail::within::point_in_geometry(p, geometry2, strategy) < 0 )
            return false;
        // Now check if holes of G2 aren't inside G1
        typedef typename boost::range_const_iterator
            <
                typename geometry::interior_type<Geometry2>::type
            >::type iterator;
        for ( iterator it = boost::begin(geometry::interior_rings(geometry2)) ;
              it != boost::end(geometry::interior_rings(geometry2)) ;
              ++it )
        {
            point2_type p;
            if ( !geometry::point_on_border(p, *it) )
                return false;
            if ( detail::within::point_in_geometry(p, geometry1, strategy) > 0 )
                return false;
        }
        return true;
    }
};

template <typename Geometry1, typename Geometry2>
struct within_no_turns<Geometry1, Geometry2, polygon_tag, polygon_tag>
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        typedef typename geometry::point_type<Geometry1>::type point1_type;
        typedef typename geometry::point_type<Geometry2>::type point2_type;
        point1_type p;
        if ( !geometry::point_on_border(p, geometry1) )
            return false;
        // check if one of ring points is outside the polygon
        if ( detail::within::point_in_geometry(p, geometry2, strategy) < 0 )
            return false;
        // Now check if holes of G2 aren't inside G1
        typedef typename boost::range_const_iterator
            <
                typename geometry::interior_type<Geometry2>::type
            >::type iterator2;
        for ( iterator2 it = boost::begin(geometry::interior_rings(geometry2)) ;
              it != boost::end(geometry::interior_rings(geometry2)) ;
              ++it )
        {
            point2_type p2;
            if ( !geometry::point_on_border(p2, *it) )
                return false;
            // if the hole of G2 is inside G1
            if ( detail::within::point_in_geometry(p2, geometry1, strategy) > 0 )
            {
                // if it's also inside one of the G1 holes, it's ok
                bool ok = false;
                typedef typename boost::range_const_iterator
                    <
                        typename geometry::interior_type<Geometry1>::type
                    >::type iterator1;
                for ( iterator1 it1 = boost::begin(geometry::interior_rings(geometry1)) ;
                      it1 != boost::end(geometry::interior_rings(geometry1)) ;
                      ++it1 )
                {
                    if ( detail::within::point_in_geometry(p2, *it1, strategy) < 0 )
                    {
                        ok = true;
                        break;
                    }
                }
                if ( !ok )
                    return false;
            }
        }
        return true;
    }
};

template <typename Geometry1,
          typename Geometry2,
          typename Tag1 = typename geometry::tag<Geometry1>::type,
          typename Tag2 = typename geometry::tag<Geometry2>::type,
          bool IsMulti1 = boost::is_base_of<geometry::multi_tag, Tag1>::value,
          bool IsMulti2 = boost::is_base_of<geometry::multi_tag, Tag2>::value>
struct within_no_turns_multi
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        return within_no_turns<Geometry1, Geometry2>::apply(geometry1, geometry2, strategy);
    }
};

template <typename Geometry1, typename Geometry2, typename Tag1, typename Tag2>
struct within_no_turns_multi<Geometry1, Geometry2, Tag1, Tag2, true, false>
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        // All values of G1 must be inside G2
        typedef typename boost::range_value<Geometry1>::type subgeometry1;
        typedef typename boost::range_const_iterator<Geometry1>::type iterator;
        for ( iterator it = boost::begin(geometry1) ; it != boost::end(geometry1) ; ++it )
        {
            if ( !within_no_turns<subgeometry1, Geometry2>::apply(*it, geometry2, strategy) )
                return false;
        }
        return true;
    }
};

template <typename Geometry1, typename Geometry2, typename Tag1, typename Tag2>
struct within_no_turns_multi<Geometry1, Geometry2, Tag1, Tag2, false, true>
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        // G1 must be within at least one value of G2
        typedef typename boost::range_value<Geometry2>::type subgeometry2;
        typedef typename boost::range_const_iterator<Geometry2>::type iterator;
        for ( iterator it = boost::begin(geometry2) ; it != boost::end(geometry2) ; ++it )
        {
            if ( within_no_turns<Geometry1, subgeometry2>::apply(geometry1, *it, strategy) )
                return true;
        }
        return false;
    }
};

template <typename Geometry1, typename Geometry2, typename Tag1, typename Tag2>
struct within_no_turns_multi<Geometry1, Geometry2, Tag1, Tag2, true, true>
{
    template <typename Strategy> static inline
    bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        // each value of G1 must be inside at least one value of G2
        typedef typename boost::range_value<Geometry1>::type subgeometry1;
        typedef typename boost::range_const_iterator<Geometry1>::type iterator;
        for ( iterator it = boost::begin(geometry1) ; it != boost::end(geometry1) ; ++it )
        {
            if ( !within_no_turns_multi<subgeometry1, Geometry2>::apply(*it, geometry2, strategy) )
                return false;
        }
        return true;
    }
};

}} // namespace detail_dispatch::within

namespace detail { namespace within {

template <typename Geometry1, typename Geometry2, typename Strategy>
inline bool within_no_turns(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
{
    return detail_dispatch::within::within_no_turns_multi<Geometry1, Geometry2>::apply(geometry1, geometry2, strategy);
}

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_WITHIN_NO_TURNS_HPP
