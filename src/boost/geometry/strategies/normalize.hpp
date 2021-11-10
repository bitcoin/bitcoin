// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_NORMALIZE_HPP
#define BOOST_GEOMETRY_STRATEGIES_NORMALIZE_HPP

#include <cstddef>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/normalize_spheroidal_box_coordinates.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace normalize
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct do_nothing
{
    template <typename GeometryIn, typename GeometryOut>
    static inline void apply(GeometryIn const&, GeometryOut&)
    {
    }
};


template <std::size_t Dimension, std::size_t DimensionCount>
struct assign_loop
{
    template <typename CoordinateType, typename PointIn, typename PointOut>
    static inline void apply(CoordinateType const& longitude,
                             CoordinateType const& latitude,
                             PointIn const& point_in,
                             PointOut& point_out)
    {
        geometry::set<Dimension>(point_out, boost::numeric_cast
            <
                typename coordinate_type<PointOut>::type
            >(geometry::get<Dimension>(point_in)));

        assign_loop
            <
                Dimension + 1, DimensionCount
            >::apply(longitude, latitude, point_in, point_out);
    }
};

template <std::size_t DimensionCount>
struct assign_loop<DimensionCount, DimensionCount>
{
    template <typename CoordinateType, typename PointIn, typename PointOut>
    static inline void apply(CoordinateType const&,
                             CoordinateType const&,
                             PointIn const&,
                             PointOut&)
    {
    }
};

template <std::size_t DimensionCount>
struct assign_loop<0, DimensionCount>
{
    template <typename CoordinateType, typename PointIn, typename PointOut>
    static inline void apply(CoordinateType const& longitude,
                             CoordinateType const& latitude,
                             PointIn const& point_in,
                             PointOut& point_out)
    {
        geometry::set<0>(point_out, boost::numeric_cast
            <
                typename coordinate_type<PointOut>::type
            >(longitude));

        assign_loop
            <
                1, DimensionCount
            >::apply(longitude, latitude, point_in, point_out);
    }
};

template <std::size_t DimensionCount>
struct assign_loop<1, DimensionCount>
{
    template <typename CoordinateType, typename PointIn, typename PointOut>
    static inline void apply(CoordinateType const& longitude,
                             CoordinateType const& latitude,
                             PointIn const& point_in,
                             PointOut& point_out)
    {
        geometry::set<1>(point_out, boost::numeric_cast
            <
                typename coordinate_type<PointOut>::type
            >(latitude));

        assign_loop
            <
                2, DimensionCount
            >::apply(longitude, latitude, point_in, point_out);
    }
};


template <typename PointIn, typename PointOut, bool IsEquatorial = true>
struct normalize_point
{
    static inline void apply(PointIn const& point_in, PointOut& point_out)
    {
        typedef typename coordinate_type<PointIn>::type in_coordinate_type;

        in_coordinate_type longitude = geometry::get<0>(point_in);
        in_coordinate_type latitude = geometry::get<1>(point_in);

        math::normalize_spheroidal_coordinates
            <
                typename geometry::detail::cs_angular_units<PointIn>::type,
                IsEquatorial,
                in_coordinate_type
            >(longitude, latitude);

        assign_loop
            <
                0, dimension<PointIn>::value
            >::apply(longitude, latitude, point_in, point_out);
    }
};


template <typename BoxIn, typename BoxOut, bool IsEquatorial = true>
class normalize_box
{
    template <typename UnitsIn, typename UnitsOut, typename CoordinateInType>
    static inline void apply_to_coordinates(CoordinateInType& lon_min,
                                            CoordinateInType& lat_min,
                                            CoordinateInType& lon_max,
                                            CoordinateInType& lat_max,
                                            BoxIn const& box_in,
                                            BoxOut& box_out)
    {
        geometry::detail::indexed_point_view<BoxOut, min_corner> p_min_out(box_out);
        assign_loop
            <
                0, dimension<BoxIn>::value
            >::apply(lon_min,
                     lat_min,
                     geometry::detail::indexed_point_view
                         <
                             BoxIn const, min_corner
                         >(box_in),
                     p_min_out);

        geometry::detail::indexed_point_view<BoxOut, max_corner> p_max_out(box_out);
        assign_loop
            <
                0, dimension<BoxIn>::value
            >::apply(lon_max,
                     lat_max,
                     geometry::detail::indexed_point_view
                         <
                             BoxIn const, max_corner
                         >(box_in),
                     p_max_out);
    }

public:
    static inline void apply(BoxIn const& box_in, BoxOut& box_out)
    {
        typedef typename coordinate_type<BoxIn>::type in_coordinate_type;

        in_coordinate_type lon_min = geometry::get<min_corner, 0>(box_in);
        in_coordinate_type lat_min = geometry::get<min_corner, 1>(box_in);
        in_coordinate_type lon_max = geometry::get<max_corner, 0>(box_in);
        in_coordinate_type lat_max = geometry::get<max_corner, 1>(box_in);

        math::normalize_spheroidal_box_coordinates
            <
                typename geometry::detail::cs_angular_units<BoxIn>::type,
                IsEquatorial,
                in_coordinate_type
            >(lon_min, lat_min, lon_max, lat_max);

        apply_to_coordinates
            <
                typename geometry::detail::cs_angular_units<BoxIn>::type,
                typename geometry::detail::cs_angular_units<BoxOut>::type
            >(lon_min, lat_min, lon_max, lat_max, box_in, box_out);
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

struct cartesian_point
    : detail::do_nothing
{};

struct cartesian_box
    : detail::do_nothing
{};

struct spherical_point
{
    template <typename PointIn, typename PointOut>
    static inline void apply(PointIn const& point_in, PointOut& point_out)
    {
        detail::normalize_point
            <
                PointIn, PointOut,
                (! std::is_same
                    <
                        typename cs_tag<PointIn>::type,
                        spherical_polar_tag
                    >::value)
            >::apply(point_in, point_out);
    }
};

struct spherical_box
{
    template <typename BoxIn, typename BoxOut>
    static inline void apply(BoxIn const& box_in, BoxOut& box_out)
    {
        detail::normalize_box
            <
                BoxIn, BoxOut,
                (! std::is_same
                    <
                        typename cs_tag<BoxIn>::type,
                        spherical_polar_tag
                    >::value)
            >::apply(box_in, box_out);
    }
};

}} // namespace strategy::normalize

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_NORMALIZE_HPP
