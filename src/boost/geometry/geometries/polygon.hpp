// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2018 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_POLYGON_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POLYGON_HPP

#include <memory>
#include <vector>

#include <boost/concept/assert.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/ring.hpp>

#include <boost/config.hpp>
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

namespace boost { namespace geometry
{

namespace model
{

/*!
\brief The polygon contains an outer ring and zero or more inner rings.
\ingroup geometries
\tparam Point point type
\tparam ClockWise true for clockwise direction,
            false for CounterClockWise direction
\tparam Closed true for closed polygons (last point == first point),
            false open points
\tparam PointList container type for points,
            for example std::vector, std::deque
\tparam RingList container type for inner rings,
            for example std::vector, std::deque
\tparam PointAlloc container-allocator-type, for the points
\tparam RingAlloc container-allocator-type, for the rings
\note The container collecting the points in the rings can be different
    from the container collecting the inner rings. They all default to vector.

\qbk{[include reference/geometries/polygon.qbk]}
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_polygon Polygon Concept]
}


*/
template
<
    typename Point,
    bool ClockWise = true,
    bool Closed = true,
    template<typename, typename> class PointList = std::vector,
    template<typename, typename> class RingList = std::vector,
    template<typename> class PointAlloc = std::allocator,
    template<typename> class RingAlloc = std::allocator
>
class polygon
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

public:

    // Member types
    typedef Point point_type;
    typedef ring<Point, ClockWise, Closed, PointList, PointAlloc> ring_type;
    typedef RingList<ring_type , RingAlloc<ring_type > > inner_container_type;

    inline ring_type const& outer() const { return m_outer; }
    inline inner_container_type const& inners() const { return m_inners; }

    inline ring_type& outer() { return m_outer; }
    inline inner_container_type & inners() { return m_inners; }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

    // default constructor definition is required only
    // if the constructor taking std::initializer_list is defined

    /// \constructor_default{polygon}
    inline polygon()
        : m_outer()
        , m_inners()
    {}

    /// \constructor_initializer_list{polygon}
    inline polygon(std::initializer_list<ring_type> l)
        : m_outer(l.size() > 0 ? *l.begin() : ring_type())
        , m_inners(l.size() > 0 ? l.begin() + 1 : l.begin(), l.end())
    {}

// Commented out for now in order to support Boost.Assign
// Without this assignment operator first the object should be created
//   from initializer list, then it shoudl be moved.
//// Without this workaround in MSVC the assignment operator is ambiguous
//#ifndef BOOST_MSVC
//    /// \assignment_initializer_list{polygon}
//    inline polygon & operator=(std::initializer_list<ring_type> l)
//    {
//        if ( l.size() > 0 )
//        {
//            m_outer = *l.begin();
//            m_inners.assign(l.begin() + 1, l.end());
//        }
//        else
//        {
//            m_outer.clear();
//            m_inners.clear();
//        }
//        return *this;
//    }
//#endif

#endif

    /// Utility method, clears outer and inner rings
    inline void clear()
    {
        m_outer.clear();
        m_inners.clear();
    }

private:

    ring_type m_outer;
    inner_container_type m_inners;
};


} // namespace model


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct tag
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList, PointAlloc, RingAlloc
        >
>
{
    typedef polygon_tag type;
};

template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct ring_const_type
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList, PointAlloc, RingAlloc
        >
>
{
    typedef typename model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >::ring_type const& type;
};


template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct ring_mutable_type
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList, PointAlloc, RingAlloc
        >
>
{
    typedef typename model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >::ring_type& type;
};

template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct interior_const_type
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >
>
{
    typedef typename model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >::inner_container_type const& type;
};


template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct interior_mutable_type
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >
>
{
    typedef typename model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >::inner_container_type& type;
};


template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct exterior_ring
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList, PointAlloc, RingAlloc
        >
>
{
    typedef model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        > polygon_type;

    static inline typename polygon_type::ring_type& get(polygon_type& p)
    {
        return p.outer();
    }

    static inline typename polygon_type::ring_type const& get(
                    polygon_type const& p)
    {
        return p.outer();
    }
};

template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class PointList,
    template<typename, typename> class RingList,
    template<typename> class PointAlloc,
    template<typename> class RingAlloc
>
struct interior_rings
<
    model::polygon
        <
            Point, ClockWise, Closed,
            PointList, RingList,
            PointAlloc, RingAlloc
        >
>
{
    typedef model::polygon
        <
            Point, ClockWise, Closed, PointList, RingList,
            PointAlloc, RingAlloc
        > polygon_type;

    static inline typename polygon_type::inner_container_type& get(
                    polygon_type& p)
    {
        return p.inners();
    }

    static inline typename polygon_type::inner_container_type const& get(
                    polygon_type const& p)
    {
        return p.inners();
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POLYGON_HPP
