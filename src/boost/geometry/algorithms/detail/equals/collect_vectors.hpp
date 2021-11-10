// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP


#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/detail/normalize.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/formulas/spherical.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/range.hpp>

#include <boost/geometry/views/detail/closed_clockwise_view.hpp>

#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>
#include <boost/geometry/strategies/normalize.hpp>


namespace boost { namespace geometry
{

// Since these vectors (though ray would be a better name) are used in the
// implementation of equals() for Areal geometries the internal representation
// should be consistent with the side strategy.
template
<
    typename T,
    typename Geometry,
    typename SideStrategy,
    typename CSTag = typename cs_tag<Geometry>::type
>
struct collected_vector
    : nyi::not_implemented_tag
{};

// compatible with side_by_triangle cartesian strategy
template <typename T, typename Geometry, typename CT, typename CSTag>
struct collected_vector
    <
        T, Geometry, strategy::side::side_by_triangle<CT>, CSTag
    >
{
    typedef T type;
    
    inline collected_vector()
    {}

    inline collected_vector(T const& px, T const& py,
                            T const& pdx, T const& pdy)
        : x(px)
        , y(py)
        , dx(pdx)
        , dy(pdy)
        //, dx_0(dx)
        //, dy_0(dy)
    {}

    template <typename Point>
    inline collected_vector(Point const& p1, Point const& p2)
        : x(get<0>(p1))
        , y(get<1>(p1))
        , dx(get<0>(p2) - x)
        , dy(get<1>(p2) - y)
        //, dx_0(dx)
        //, dy_0(dy)
    {}

    bool normalize()
    {
        T magnitude = math::sqrt(
            boost::numeric_cast<T>(dx * dx + dy * dy));

        // NOTE: shouldn't here math::equals() be called?
        if (magnitude > 0)
        {
            dx /= magnitude;
            dy /= magnitude;
            return true;
        }

        return false;
    }

    // For sorting
    inline bool operator<(collected_vector const& other) const
    {
        if (math::equals(x, other.x))
        {
            if (math::equals(y, other.y))
            {
                if (math::equals(dx, other.dx))
                {
                    return dy < other.dy;
                }
                return dx < other.dx;
            }
            return y < other.y;
        }
        return x < other.x;
    }

    inline bool next_is_collinear(collected_vector const& other) const
    {
        return same_direction(other);
    }

    // For std::equals
    inline bool operator==(collected_vector const& other) const
    {
        return math::equals(x, other.x)
            && math::equals(y, other.y)
            && same_direction(other);
    }

private:
    inline bool same_direction(collected_vector const& other) const
    {
        // For high precision arithmetic, we have to be
        // more relaxed then using ==
        // Because 2/sqrt( (0,0)<->(2,2) ) == 1/sqrt( (0,0)<->(1,1) )
        // is not always true (at least, not for some user defined types)
        return math::equals_with_epsilon(dx, other.dx)
            && math::equals_with_epsilon(dy, other.dy);
    }

    T x, y;
    T dx, dy;
    //T dx_0, dy_0;
};

// Compatible with spherical_side_formula which currently
// is the default spherical_equatorial and geographic strategy
// so CSTag is spherical_equatorial_tag or geographic_tag
template <typename T, typename Geometry, typename CT, typename CSTag>
struct collected_vector
    <
        T, Geometry, strategy::side::spherical_side_formula<CT>, CSTag
    >
{
    typedef T type;
    
    typedef typename geometry::detail::cs_angular_units<Geometry>::type units_type;
    typedef model::point<T, 2, cs::spherical_equatorial<units_type> > point_type;
    typedef model::point<T, 3, cs::cartesian> vector_type;

    collected_vector()
    {}

    template <typename Point>
    collected_vector(Point const& p1, Point const& p2)
        : origin(get<0>(p1), get<1>(p1))
    {
        origin = detail::return_normalized<point_type>(
                    origin,
                    strategy::normalize::spherical_point());

        using namespace geometry::formula;
        prev = sph_to_cart3d<vector_type>(p1);
        next = sph_to_cart3d<vector_type>(p2);
        direction = cross_product(prev, next);
    }

    bool normalize()
    {
        T magnitude_sqr = dot_product(direction, direction);

        // NOTE: shouldn't here math::equals() be called?
        if (magnitude_sqr > 0)
        {
            divide_value(direction, math::sqrt(magnitude_sqr));
            return true;
        }

        return false;
    }

    bool operator<(collected_vector const& other) const
    {
        if (math::equals(get<0>(origin), get<0>(other.origin)))
        {
            if (math::equals(get<1>(origin), get<1>(other.origin)))
            {
                if (math::equals(get<0>(direction), get<0>(other.direction)))
                {
                    if (math::equals(get<1>(direction), get<1>(other.direction)))
                    {
                        return get<2>(direction) < get<2>(other.direction);
                    }
                    return get<1>(direction) < get<1>(other.direction);
                }
                return get<0>(direction) < get<0>(other.direction);
            }
            return get<1>(origin) < get<1>(other.origin);
        }
        return get<0>(origin) < get<0>(other.origin);
    }

    // For consistency with side and intersection strategies used by relops
    // IMPORTANT: this method should be called for previous vector
    // and next vector should be passed as parameter
    bool next_is_collinear(collected_vector const& other) const
    {
        return formula::sph_side_value(direction, other.next) == 0;
    }

    // For std::equals
    bool operator==(collected_vector const& other) const
    {
        return math::equals(get<0>(origin), get<0>(other.origin))
            && math::equals(get<1>(origin), get<1>(other.origin))
            && is_collinear(other);
    }

private:
    // For consistency with side and intersection strategies used by relops
    bool is_collinear(collected_vector const& other) const
    {
        return formula::sph_side_value(direction, other.prev) == 0
            && formula::sph_side_value(direction, other.next) == 0;
    }
    
    /*bool same_direction(collected_vector const& other) const
    {
        return math::equals_with_epsilon(get<0>(direction), get<0>(other.direction))
            && math::equals_with_epsilon(get<1>(direction), get<1>(other.direction))
            && math::equals_with_epsilon(get<2>(direction), get<2>(other.direction));
    }*/

    point_type origin; // used for sorting and equality check
    vector_type direction; // used for sorting, only in operator<
    vector_type prev; // used for collinearity check, only in operator==
    vector_type next; // used for collinearity check
};

// Specialization for spherical polar
template <typename T, typename Geometry, typename CT>
struct collected_vector
    <
        T, Geometry,
        strategy::side::spherical_side_formula<CT>,
        spherical_polar_tag
    >
    : public collected_vector
        <
            T, Geometry,
            strategy::side::spherical_side_formula<CT>,
            spherical_equatorial_tag
        >
{
    typedef collected_vector
        <
            T, Geometry,
            strategy::side::spherical_side_formula<CT>,
            spherical_equatorial_tag
        > base_type;

    collected_vector() {}

    template <typename Point>
    collected_vector(Point const& p1, Point const& p2)
        : base_type(to_equatorial(p1), to_equatorial(p2))
    {}

private:
    template <typename Point>
    Point to_equatorial(Point const& p)
    {
        typedef typename coordinate_type<Point>::type coord_type;

        typedef math::detail::constants_on_spheroid
            <
                coord_type,
                typename coordinate_system<Point>::type::units
            > constants;

        coord_type const pi_2 = constants::half_period() / 2;

        Point res = p;
        set<1>(res, pi_2 - get<1>(p));
        return res;
    }
};


// TODO: specialize collected_vector for geographic_tag


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace collect_vectors
{


template <typename Range, typename Collection>
struct range_collect_vectors
{
    typedef typename boost::range_value<Collection>::type item_type;
    typedef typename item_type::type calculation_type;

    static inline void apply(Collection& collection, Range const& range)
    {
        apply_impl(collection,
                   detail::closed_clockwise_view<Range const>(range));
    }

private:
    template <typename ClosedClockwiseRange>
    static inline void apply_impl(Collection& collection, ClosedClockwiseRange const& range)
    {
        if (boost::size(range) < 2)
        {
            return;
        }

        typedef typename boost::range_size<Collection>::type collection_size_t;
        collection_size_t c_old_size = boost::size(collection);

        typedef typename boost::range_iterator<ClosedClockwiseRange const>::type iterator;

        bool is_first = true;
        iterator it = boost::begin(range);

        for (iterator prev = it++; it != boost::end(range); prev = it++)
        {
            typename boost::range_value<Collection>::type v(*prev, *it);

            // Normalize the vector -> this results in points+direction
            // and is comparible between geometries
            // Avoid non-duplicate points (AND division by zero)
            if (v.normalize())
            {
                // Avoid non-direction changing points
                if (is_first || ! collection.back().next_is_collinear(v))
                {
                    collection.push_back(v);
                }
                is_first = false;
            }
        }

        // If first one has same direction as last one, remove first one
        collection_size_t collected_count = boost::size(collection) - c_old_size;
        if ( collected_count > 1 )
        {
            typedef typename boost::range_iterator<Collection>::type c_iterator;
            c_iterator first = range::pos(collection, c_old_size);

            if (collection.back().next_is_collinear(*first) )
            {
                //collection.erase(first);
                // O(1) instead of O(N)
                *first = collection.back();
                collection.pop_back();
            }
        }
    }
};


// Default version (cartesian)
template <typename Box, typename Collection, typename CSTag = typename cs_tag<Box>::type>
struct box_collect_vectors
{
    // Calculate on coordinate type, but if it is integer,
    // then use double
    typedef typename boost::range_value<Collection>::type item_type;
    typedef typename item_type::type calculation_type;

    static inline void apply(Collection& collection, Box const& box)
    {
        typename point_type<Box>::type lower_left, lower_right,
            upper_left, upper_right;
        geometry::detail::assign_box_corners(box, lower_left, lower_right,
            upper_left, upper_right);

        typedef typename boost::range_value<Collection>::type item;

        collection.push_back(item(get<0>(lower_left), get<1>(lower_left), 0, 1));
        collection.push_back(item(get<0>(upper_left), get<1>(upper_left), 1, 0));
        collection.push_back(item(get<0>(upper_right), get<1>(upper_right), 0, -1));
        collection.push_back(item(get<0>(lower_right), get<1>(lower_right), -1, 0));
    }
};

// NOTE: This is not fully correct because Box in spherical and geographic
// cordinate systems cannot be seen as Polygon
template <typename Box, typename Collection>
struct box_collect_vectors<Box, Collection, spherical_equatorial_tag>
{
    static inline void apply(Collection& collection, Box const& box)
    {
        typename point_type<Box>::type lower_left, lower_right,
                upper_left, upper_right;
        geometry::detail::assign_box_corners(box, lower_left, lower_right,
                upper_left, upper_right);

        typedef typename boost::range_value<Collection>::type item;

        collection.push_back(item(lower_left, upper_left));
        collection.push_back(item(upper_left, upper_right));
        collection.push_back(item(upper_right, lower_right));
        collection.push_back(item(lower_right, lower_left));
    }
};

template <typename Box, typename Collection>
struct box_collect_vectors<Box, Collection, spherical_polar_tag>
    : box_collect_vectors<Box, Collection, spherical_equatorial_tag>
{};

template <typename Box, typename Collection>
struct box_collect_vectors<Box, Collection, geographic_tag>
    : box_collect_vectors<Box, Collection, spherical_equatorial_tag>
{};


template <typename Polygon, typename Collection>
struct polygon_collect_vectors
{
    static inline void apply(Collection& collection, Polygon const& polygon)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef range_collect_vectors<ring_type, Collection> per_range;
        per_range::apply(collection, exterior_ring(polygon));

        typename interior_return_type<Polygon const>::type
            rings = interior_rings(polygon);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            per_range::apply(collection, *it);
        }
    }
};


template <typename MultiGeometry, typename Collection, typename SinglePolicy>
struct multi_collect_vectors
{
    static inline void apply(Collection& collection, MultiGeometry const& multi)
    {
        for (typename boost::range_iterator<MultiGeometry const>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            SinglePolicy::apply(collection, *it);
        }
    }
};


}} // namespace detail::collect_vectors
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename Collection,
    typename Geometry
>
struct collect_vectors
{
    static inline void apply(Collection&, Geometry const&)
    {}
};


template <typename Collection, typename Box>
struct collect_vectors<box_tag, Collection, Box>
    : detail::collect_vectors::box_collect_vectors<Box, Collection>
{};



template <typename Collection, typename Ring>
struct collect_vectors<ring_tag, Collection, Ring>
    : detail::collect_vectors::range_collect_vectors<Ring, Collection>
{};


template <typename Collection, typename LineString>
struct collect_vectors<linestring_tag, Collection, LineString>
    : detail::collect_vectors::range_collect_vectors<LineString, Collection>
{};


template <typename Collection, typename Polygon>
struct collect_vectors<polygon_tag, Collection, Polygon>
    : detail::collect_vectors::polygon_collect_vectors<Polygon, Collection>
{};


template <typename Collection, typename MultiPolygon>
struct collect_vectors<multi_polygon_tag, Collection, MultiPolygon>
    : detail::collect_vectors::multi_collect_vectors
        <
            MultiPolygon,
            Collection,
            detail::collect_vectors::polygon_collect_vectors
            <
                typename boost::range_value<MultiPolygon>::type,
                Collection
            >
        >
{};



} // namespace dispatch
#endif


/*!
    \ingroup collect_vectors
    \tparam Collection Collection type, should be e.g. std::vector<>
    \tparam Geometry geometry type
    \param collection the collection of vectors
    \param geometry the geometry to make collect_vectors
*/
template <typename Collection, typename Geometry>
inline void collect_vectors(Collection& collection, Geometry const& geometry)
{
    concepts::check<Geometry const>();

    dispatch::collect_vectors
        <
            typename tag<Geometry>::type,
            Collection,
            Geometry
        >::apply(collection, geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP
