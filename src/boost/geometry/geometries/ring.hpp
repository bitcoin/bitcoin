// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_RING_HPP
#define BOOST_GEOMETRY_GEOMETRIES_RING_HPP

#include <memory>
#include <vector>

#include <boost/concept/assert.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

#include <boost/config.hpp>
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

namespace boost { namespace geometry
{

namespace model
{
/*!
\brief A ring (aka linear ring) is a closed line which should not be selfintersecting
\ingroup geometries
\tparam Point point type
\tparam ClockWise true for clockwise direction,
            false for CounterClockWise direction
\tparam Closed true for closed polygons (last point == first point),
            false open points
\tparam Container container type, for example std::vector, std::deque
\tparam Allocator container-allocator-type

\qbk{[include reference/geometries/ring.qbk]}
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_ring Ring Concept]
}
*/
template
<
    typename Point,
    bool ClockWise = true, bool Closed = true,
    template<typename, typename> class Container = std::vector,
    template<typename> class Allocator = std::allocator
>
class ring : public Container<Point, Allocator<Point> >
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    typedef Container<Point, Allocator<Point> > base_type;

public :
    /// \constructor_default{ring}
    inline ring()
        : base_type()
    {}

    /// \constructor_begin_end{ring}
    template <typename Iterator>
    inline ring(Iterator begin, Iterator end)
        : base_type(begin, end)
    {}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

    /// \constructor_initializer_list{ring}
    inline ring(std::initializer_list<Point> l)
        : base_type(l.begin(), l.end())
    {}

// Commented out for now in order to support Boost.Assign
// Without this assignment operator first the object should be created
//   from initializer list, then it shoudl be moved.
//// Without this workaround in MSVC the assignment operator is ambiguous
//#ifndef BOOST_MSVC
//    /// \assignment_initializer_list{ring}
//    inline ring & operator=(std::initializer_list<Point> l)
//    {
//        base_type::assign(l.begin(), l.end());
//        return *this;
//    }
//#endif

#endif
};

} // namespace model


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename Point,
    bool ClockWise, bool Closed,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct tag<model::ring<Point, ClockWise, Closed, Container, Allocator> >
{
    typedef ring_tag type;
};


template
<
    typename Point,
    bool Closed,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct point_order<model::ring<Point, false, Closed, Container, Allocator> >
{
    static const order_selector value = counterclockwise;
};


template
<
    typename Point,
    bool Closed,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct point_order<model::ring<Point, true, Closed, Container, Allocator> >
{
    static const order_selector value = clockwise;
};

template
<
    typename Point,
    bool PointOrder,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct closure<model::ring<Point, PointOrder, true, Container, Allocator> >
{
    static const closure_selector value = closed;
};

template
<
    typename Point,
    bool PointOrder,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct closure<model::ring<Point, PointOrder, false, Container, Allocator> >
{
    static const closure_selector value = open;
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_RING_HPP
