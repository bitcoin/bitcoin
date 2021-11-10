// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_IS_EMPTY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_IS_EMPTY_HPP

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_empty
{

struct always_not_empty
{
    template <typename Geometry>
    static inline bool apply(Geometry const&)
    {
        return false;
    }
};

struct range_is_empty
{
    template <typename Range>
    static inline bool apply(Range const& range)
    {
        return boost::empty(range);
    }
};

class polygon_is_empty
{
    template <typename InteriorRings>
    static inline bool check_interior_rings(InteriorRings const& interior_rings)
    {
        return check_iterator_range
            <
                range_is_empty, true // allow empty range
            >::apply(boost::begin(interior_rings), boost::end(interior_rings));
    }

public:
    template <typename Polygon>
    static inline bool apply(Polygon const& polygon)
    {
        return boost::empty(exterior_ring(polygon))
            && check_interior_rings(interior_rings(polygon));
    }
};

template <typename Policy = range_is_empty>
struct multi_is_empty
{
    template <typename MultiGeometry>
    static inline bool apply(MultiGeometry const& multigeometry)
    {
        return check_iterator_range
            <
                Policy, true // allow empty range
            >::apply(boost::begin(multigeometry), boost::end(multigeometry));
    }
    
};

}} // namespace detail::is_empty
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry, typename Tag =  typename tag<Geometry>::type>
struct is_empty : not_implemented<Tag>
{};

template <typename Geometry>
struct is_empty<Geometry, point_tag>
    : detail::is_empty::always_not_empty
{};

template <typename Geometry>
struct is_empty<Geometry, box_tag>
    : detail::is_empty::always_not_empty
{};

template <typename Geometry>
struct is_empty<Geometry, segment_tag>
    : detail::is_empty::always_not_empty
{};

template <typename Geometry>
struct is_empty<Geometry, linestring_tag>
    : detail::is_empty::range_is_empty
{};

template <typename Geometry>
struct is_empty<Geometry, ring_tag>
    : detail::is_empty::range_is_empty
{};

template <typename Geometry>
struct is_empty<Geometry, polygon_tag>
    : detail::is_empty::polygon_is_empty
{};

template <typename Geometry>
struct is_empty<Geometry, multi_point_tag>
    : detail::is_empty::range_is_empty
{};

template <typename Geometry>
struct is_empty<Geometry, multi_linestring_tag>
    : detail::is_empty::multi_is_empty<>
{};

template <typename Geometry>
struct is_empty<Geometry, multi_polygon_tag>
    : detail::is_empty::multi_is_empty<detail::is_empty::polygon_is_empty>
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_variant
{

template <typename Geometry>
struct is_empty
{
    static inline bool apply(Geometry const& geometry)
    {
        concepts::check<Geometry const>();

        return dispatch::is_empty<Geometry>::apply(geometry);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct is_empty<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor : boost::static_visitor<bool>
    {
        template <typename Geometry>
        inline bool operator()(Geometry const& geometry) const
        {
            return is_empty<Geometry>::apply(geometry);
        }
    };

    static bool
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry)
    {
        return boost::apply_visitor(visitor(), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_check{is the empty set}
\ingroup is_empty
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is the empty set}

\qbk{[include reference/algorithms/is_empty.qbk]}
*/
template <typename Geometry>
inline bool is_empty(Geometry const& geometry)
{
    return resolve_variant::is_empty<Geometry>::apply(geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_IS_EMPTY_HPP
