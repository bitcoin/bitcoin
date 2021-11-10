// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLE_ITERATOR_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLE_ITERATOR_HPP

// Adapts Geometries from Boost.Polygon for usage in Boost.Geometry
// boost::polygon::polygon_with_holes_data -> boost::geometry::polygon
//   hole_iterator  -> returning ring_proxy's instead of normal polygon_data

#include <boost/polygon/polygon.hpp>

#include <boost/iterator/iterator_facade.hpp>


namespace boost { namespace geometry
{

namespace adapt { namespace bp
{


template <typename Polygon, typename RingProxy>
class hole_iterator
    : public ::boost::iterator_facade
        <
            hole_iterator<Polygon, RingProxy>,
            RingProxy, // value type
            boost::forward_traversal_tag,
            RingProxy // reference type
        >
{
public :
    typedef typename boost::polygon::polygon_with_holes_traits
        <
            Polygon
        >::iterator_holes_type ith_type;

    explicit inline hole_iterator(Polygon& polygon, ith_type const it)
        : m_polygon(polygon)
        , m_base(it)
    {
    }

    typedef std::ptrdiff_t difference_type;

private:
    friend class boost::iterator_core_access;

    inline RingProxy dereference() const
    {
        return RingProxy(m_polygon, this->m_base);
    }

    inline void increment() { ++m_base; }
    inline void decrement() { --m_base; }
    inline void advance(difference_type n)
    {
        for (int i = 0; i < n; i++)
        {
            ++m_base;
        }
    }

    inline bool equal(hole_iterator<Polygon, RingProxy> const& other) const
    {
        return this->m_base == other.m_base;
    }

    Polygon& m_polygon;
    ith_type m_base;
};


}}}}

#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_POLYGON_HOLE_ITERATOR_HPP

