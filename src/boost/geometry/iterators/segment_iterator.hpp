// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_SEGMENT_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_SEGMENT_ITERATOR_HPP


#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/iterators/detail/point_iterator/inner_range_type.hpp>
#include <boost/geometry/iterators/detail/segment_iterator/iterator_type.hpp>

#include <boost/geometry/iterators/dispatch/segment_iterator.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// specializations for segments_begin


template <typename Linestring>
struct segments_begin<Linestring, linestring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            Linestring
        >::type return_type;

    static inline return_type apply(Linestring& linestring)
    {
        return return_type(linestring);
    }
};


template <typename Ring>
struct segments_begin<Ring, ring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            Ring
        >::type return_type;

    static inline return_type apply(Ring& ring)
    {
        return return_type(ring);
    }
};


template <typename Polygon>
struct segments_begin<Polygon, polygon_tag>
{
    typedef typename detail::point_iterator::inner_range_type
        <
            Polygon
        >::type inner_range;

    typedef typename detail::segment_iterator::iterator_type
        <
            Polygon
        >::type return_type;

    static inline return_type apply(Polygon& polygon)
    {
        typedef typename return_type::second_iterator_type flatten_iterator;

        return return_type
            (segments_begin
                 <
                     inner_range
                 >::apply(geometry::exterior_ring(polygon)),
             segments_end
                 <
                     inner_range
                 >::apply(geometry::exterior_ring(polygon)),
             flatten_iterator(boost::begin(geometry::interior_rings(polygon)),
                              boost::end(geometry::interior_rings(polygon))
                              ),
             flatten_iterator(boost::begin(geometry::interior_rings(polygon)),
                              boost::end(geometry::interior_rings(polygon))
                              )
             );
    }
};


template <typename MultiLinestring>
struct segments_begin<MultiLinestring, multi_linestring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            MultiLinestring
        >::type return_type;

    static inline return_type apply(MultiLinestring& multilinestring)
    {
        return return_type(boost::begin(multilinestring),
                           boost::end(multilinestring));
    }
};


template <typename MultiPolygon>
struct segments_begin<MultiPolygon, multi_polygon_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            MultiPolygon
        >::type return_type;

    static inline return_type apply(MultiPolygon& multipolygon)
    {
        return return_type(boost::begin(multipolygon),
                           boost::end(multipolygon));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH





#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// specializations for segments_end


template <typename Linestring>
struct segments_end<Linestring, linestring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            Linestring
        >::type return_type;

    static inline return_type apply(Linestring& linestring)
    {
        return return_type(linestring, true);
    }
};


template <typename Ring>
struct segments_end<Ring, ring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            Ring
        >::type return_type;

    static inline return_type apply(Ring& ring)
    {
        return return_type(ring, true);
    }
};


template <typename Polygon>
struct segments_end<Polygon, polygon_tag>
{
    typedef typename detail::point_iterator::inner_range_type
        <
            Polygon
        >::type inner_range;

    typedef typename detail::segment_iterator::iterator_type
        <
            Polygon
        >::type return_type;

    static inline return_type apply(Polygon& polygon)
    {
        typedef typename return_type::second_iterator_type flatten_iterator;

        return return_type
            (segments_end
                 <
                     inner_range
                 >::apply(geometry::exterior_ring(polygon)),
             flatten_iterator(boost::begin(geometry::interior_rings(polygon)),
                              boost::end(geometry::interior_rings(polygon))
                              ),
             flatten_iterator( boost::end(geometry::interior_rings(polygon)) )
             );
    }
};


template <typename MultiLinestring>
struct segments_end<MultiLinestring, multi_linestring_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            MultiLinestring
        >::type return_type;

    static inline return_type apply(MultiLinestring& multilinestring)
    {
        return return_type(boost::end(multilinestring));
    }
};


template <typename MultiPolygon>
struct segments_end<MultiPolygon, multi_polygon_tag>
{
    typedef typename detail::segment_iterator::iterator_type
        <
            MultiPolygon
        >::type return_type;

    static inline return_type apply(MultiPolygon& multipolygon)
    {
        return return_type(boost::end(multipolygon));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


// MK:: need to add doc here
template <typename Geometry>
class segment_iterator
    : public detail::segment_iterator::iterator_type<Geometry>::type
{
private:
    typedef typename detail::segment_iterator::iterator_type
        <
            Geometry
        >::type base;

    inline base const* base_ptr() const
    {
        return this;
    }

    template <typename OtherGeometry> friend class segment_iterator;

    template <typename G>
    friend inline segment_iterator<G const> segments_begin(G const&);

    template <typename G>
    friend inline segment_iterator<G const> segments_end(G const&);

    inline segment_iterator(base const& base_it) : base(base_it) {}

public:
    // The following typedef is needed for this iterator to be
    // bidirectional.
    // Normally we would not have to define this. However, due to the
    // fact that the value type of the iterator is not a reference,
    // the iterator_facade framework (used to define the base class of
    // this iterator) degrades automatically the iterator's category
    // to input iterator. With the following typedef we recover the
    // correct iterator category.
    typedef std::bidirectional_iterator_tag iterator_category;

    inline segment_iterator() = default;

    template
    <
        typename OtherGeometry,
        std::enable_if_t
            <
                std::is_convertible
                    <
                        typename detail::segment_iterator::iterator_type<OtherGeometry>::type,
                        typename detail::segment_iterator::iterator_type<Geometry>::type
                    >::value,
                int
            > = 0
    >
    inline segment_iterator(segment_iterator<OtherGeometry> const& other)
        : base(*other.base_ptr())
    {}

    inline segment_iterator& operator++() // prefix
    {
        base::operator++();
        return *this;
    }

    inline segment_iterator& operator--() // prefix
    {
        base::operator--();
        return *this;
    }

    inline segment_iterator operator++(int) // postfix
    {
        segment_iterator copy(*this);
        base::operator++();
        return copy;
    }

    inline segment_iterator operator--(int) // postfix
    {
        segment_iterator copy(*this);
        base::operator--();
        return copy;
    }
};


// MK:: need to add doc here
template <typename Geometry>
inline segment_iterator<Geometry const>
segments_begin(Geometry const& geometry)
{
    return dispatch::segments_begin<Geometry const>::apply(geometry);
}


// MK:: need to add doc here
template <typename Geometry>
inline segment_iterator<Geometry const>
segments_end(Geometry const& geometry)
{
    return dispatch::segments_end<Geometry const>::apply(geometry);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ITERATORS_SEGMENT_ITERATOR_HPP
