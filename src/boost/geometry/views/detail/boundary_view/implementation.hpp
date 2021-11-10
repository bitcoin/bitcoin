// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_VIEWS_DETAIL_BOUNDARY_VIEW_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_VIEWS_DETAIL_BOUNDARY_VIEW_IMPLEMENTATION_HPP

#include <cstddef>
#include <algorithm>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/core/addressof.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>

#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/iterators/flatten_iterator.hpp>
#include <boost/geometry/util/range.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/detail/boundary_view/interface.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace boundary_views
{


template
<
    typename Polygon,
    typename Value = typename ring_type<Polygon>::type,
    typename Reference = typename ring_return_type<Polygon>::type,
    typename Difference = typename boost::range_difference
        <
            typename std::remove_reference
                <
                    typename interior_return_type<Polygon>::type
                >::type
        >::type
>
class polygon_rings_iterator
    : public boost::iterator_facade
        <
            polygon_rings_iterator<Polygon, Value, Reference, Difference>,
            Value,
            boost::random_access_traversal_tag,
            Reference,
            Difference
        >
{
    typedef typename boost::range_size
        <
            typename std::remove_reference
                <
                    typename interior_return_type<Polygon>::type
                >::type
        >::type size_type;

public:
    // default constructor
    polygon_rings_iterator()
        : m_polygon(NULL)
        , m_index(0)
    {}

    // for begin
    polygon_rings_iterator(Polygon& polygon)
        : m_polygon(boost::addressof(polygon))
        , m_index(0)
    {}

    // for end
    polygon_rings_iterator(Polygon& polygon, bool)
        : m_polygon(boost::addressof(polygon))
        , m_index(static_cast<size_type>(num_rings(polygon)))
    {}

    template
    <
        typename OtherPolygon,
        typename OtherValue,
        typename OtherReference,
        typename OtherDifference
    >
    polygon_rings_iterator(polygon_rings_iterator
                           <
                               OtherPolygon,
                               OtherValue,
                               OtherReference,
                               OtherDifference
                           > const& other)
        : m_polygon(other.m_polygon)
        , m_index(other.m_index)
    {
        static const bool is_convertible
            = std::is_convertible<OtherPolygon, Polygon>::value;

        BOOST_GEOMETRY_STATIC_ASSERT((is_convertible),
            "OtherPolygon has to be convertible to Polygon.",
            OtherPolygon, Polygon);
    }

private:
    friend class boost::iterator_core_access;

    template
    <
        typename OtherPolygon,
        typename OtherValue,
        typename OtherReference,
        typename OtherDifference
    >
    friend class polygon_rings_iterator;


    static inline std::size_t num_rings(Polygon const& polygon)
    {
        return geometry::num_interior_rings(polygon) + 1;
    }

    inline Reference dereference() const
    {
        if (m_index == 0)
        {
            return exterior_ring(*m_polygon);
        }
        return range::at(interior_rings(*m_polygon), m_index - 1);
    }

    template
    <
        typename OtherPolygon,
        typename OtherValue,
        typename OtherReference,
        typename OtherDifference
    >
    inline bool equal(polygon_rings_iterator
                      <
                          OtherPolygon,
                          OtherValue,
                          OtherReference,
                          OtherDifference
                      > const& other) const
    {
        BOOST_GEOMETRY_ASSERT(m_polygon == other.m_polygon);
        return m_index == other.m_index;
    }

    inline void increment()
    {
        ++m_index;
    }

    inline void decrement()
    {
        --m_index;
    }

    template
    <
        typename OtherPolygon,
        typename OtherValue,
        typename OtherReference,
        typename OtherDifference
    >
    inline Difference distance_to(polygon_rings_iterator
                                  <
                                      OtherPolygon,
                                      OtherValue,
                                      OtherReference,
                                      OtherDifference
                                  > const& other) const
    {
        return static_cast<Difference>(other.m_index)
            - static_cast<Difference>(m_index);
    }

    inline void advance(Difference n)
    {
        m_index += n;
    }

private:
    Polygon* m_polygon;
    size_type m_index;
};


template <typename Ring>
class ring_boundary : closeable_view<Ring, closure<Ring>::value>::type
{
private:
    typedef typename closeable_view<Ring, closure<Ring>::value>::type base_type;

public:
    typedef typename base_type::iterator iterator;
    typedef typename base_type::const_iterator const_iterator;
    
    typedef linestring_tag tag_type;

    explicit ring_boundary(Ring& ring)
        : base_type(ring) {}

    iterator begin() { return base_type::begin(); }
    iterator end() { return base_type::end(); }
    const_iterator begin() const { return base_type::begin(); }
    const_iterator end() const { return base_type::end(); }
};


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct num_rings
{};

template <typename Polygon>
struct num_rings<Polygon, polygon_tag>
{
    static inline std::size_t apply(Polygon const& polygon)
    {
        return geometry::num_interior_rings(polygon) + 1;
    }
};

template <typename MultiPolygon>
struct num_rings<MultiPolygon, multi_polygon_tag>
{
    static inline std::size_t apply(MultiPolygon const& multipolygon)
    {
        return geometry::num_interior_rings(multipolygon)
            + static_cast<std::size_t>(boost::size(multipolygon));
    }
};


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct views_container_initializer
{};

template <typename Polygon>
struct views_container_initializer<Polygon, polygon_tag>
{
    template <typename BoundaryView>
    static inline void apply(Polygon const& polygon, BoundaryView* views)
    {
        typedef polygon_rings_iterator<Polygon> rings_iterator_type;

        std::uninitialized_copy(rings_iterator_type(polygon),
                                rings_iterator_type(polygon, true),
                                views);
    }
};

template <typename MultiPolygon>
class views_container_initializer<MultiPolygon, multi_polygon_tag>
{
    typedef std::conditional_t
        <
            std::is_const<MultiPolygon>::value,
            typename boost::range_value<MultiPolygon>::type const,
            typename boost::range_value<MultiPolygon>::type
        > polygon_type;

    typedef polygon_rings_iterator<polygon_type> inner_iterator_type;

    struct polygon_rings_begin
    {
        static inline inner_iterator_type apply(polygon_type& polygon)
        {
            return inner_iterator_type(polygon);
        }
    };

    struct polygon_rings_end
    {
        static inline inner_iterator_type apply(polygon_type& polygon)
        {
            return inner_iterator_type(polygon, true);
        }
    };

    typedef flatten_iterator
        <
            typename boost::range_iterator<MultiPolygon>::type,
            inner_iterator_type,
            typename std::iterator_traits<inner_iterator_type>::value_type,
            polygon_rings_begin,
            polygon_rings_end,
            typename std::iterator_traits<inner_iterator_type>::reference
        > rings_iterator_type;

public:
    template <typename BoundaryView>
    static inline void apply(MultiPolygon const& multipolygon,
                             BoundaryView* views)
    {
        rings_iterator_type first(boost::begin(multipolygon),
                                  boost::end(multipolygon));
        rings_iterator_type last(boost::end(multipolygon));

        std::uninitialized_copy(first, last, views);
    }
};


template <typename Areal>
class areal_boundary
{
    typedef boundary_view<typename ring_type<Areal>::type> boundary_view_type;
    typedef views_container_initializer<Areal> exception_safe_initializer;

    template <typename T>
    struct automatic_deallocator
    {
        automatic_deallocator(T* ptr) : m_ptr(ptr) {}

        ~automatic_deallocator()
        {
            operator delete(m_ptr);
        }

        inline void release() { m_ptr = NULL; }

        T* m_ptr;
    };

    inline void initialize_views(Areal const& areal)
    {
        // initialize number of rings
        std::size_t n_rings = num_rings<Areal>::apply(areal);

        if (n_rings == 0)
        {
            return;
        }

        // allocate dynamic memory
        boundary_view_type* views_ptr = static_cast
            <
                boundary_view_type*
            >(operator new(sizeof(boundary_view_type) * n_rings));

        // initialize; if exceptions are thrown by constructors
        // they are handled automatically by automatic_deallocator
        automatic_deallocator<boundary_view_type> deallocator(views_ptr);
        exception_safe_initializer::apply(areal, views_ptr);
        deallocator.release();

        // now initialize member variables safely
        m_views = views_ptr;
        m_num_rings = n_rings;
    }

    // disallow copies and/or assignments
    areal_boundary(areal_boundary const&);
    areal_boundary& operator=(areal_boundary const&);

public:
    typedef boundary_view_type* iterator;
    typedef boundary_view_type const* const_iterator;

    typedef multi_linestring_tag tag_type;

    explicit areal_boundary(Areal& areal)
        : m_views(NULL)
        , m_num_rings(0)
    {
        initialize_views(areal);
    }

    ~areal_boundary()
    {
        boundary_view_type* last = m_views + m_num_rings;
        for (boundary_view_type* it = m_views; it != last; ++it)
        {
            it->~boundary_view_type();
        }
        operator delete(m_views);
    }

    inline iterator begin() { return m_views; }
    inline iterator end() { return m_views + m_num_rings; }
    inline const_iterator begin() const { return m_views; }
    inline const_iterator end() const { return m_views + m_num_rings; }

private:
    boundary_view_type* m_views;
    std::size_t m_num_rings;
};


}} // namespace detail::boundary_view
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace detail_dispatch
{


template <typename Ring>
struct boundary_view<Ring, ring_tag>
    : detail::boundary_views::ring_boundary<Ring>
{
    explicit boundary_view(Ring& ring)
        : detail::boundary_views::ring_boundary<Ring>(ring)
    {}
};

template <typename Polygon>
struct boundary_view<Polygon, polygon_tag>
    : detail::boundary_views::areal_boundary<Polygon>
{
    explicit boundary_view(Polygon& polygon)
        : detail::boundary_views::areal_boundary<Polygon>(polygon)
    {}
};

template <typename MultiPolygon>
struct boundary_view<MultiPolygon, multi_polygon_tag>
    : detail::boundary_views::areal_boundary<MultiPolygon>
{
    explicit boundary_view(MultiPolygon& multipolygon)
        : detail::boundary_views::areal_boundary
            <
                MultiPolygon
            >(multipolygon)
    {}
};


} // namespace detail_dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_VIEWS_DETAIL_BOUNDARY_VIEW_IMPLEMENTATION_HPP
