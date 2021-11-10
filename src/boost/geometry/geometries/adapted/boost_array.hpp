// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2010 Alfredo Correa
// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ARRAY_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ARRAY_HPP


#ifdef BOOST_GEOMETRY_ADAPTED_BOOST_ARRAY_TAG_DEFINED
#error Include either "boost_array_as_point" or \
    "boost_array_as_linestring" or "boost_array_as_ring" \
    or "boost_array_as_multi_point" to adapt a boost_array
#endif

#define BOOST_GEOMETRY_ADAPTED_BOOST_ARRAY_TAG_DEFINED


#include <cstddef>
#include <type_traits>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/array.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


// Create class and specialization to indicate the tag
// for normal cases and the case that the type of the c-array is arithmetic
template <bool>
struct boost_array_tag
{
    typedef geometry_not_recognized_tag type;
};


template <>
struct boost_array_tag<true>
{
    typedef point_tag type;
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


// Assign the point-tag, preventing arrays of points getting a point-tag
template <typename CoordinateType, std::size_t DimensionCount>
struct tag<boost::array<CoordinateType, DimensionCount> >
    : detail::boost_array_tag<std::is_arithmetic<CoordinateType>::value> {};


template <typename CoordinateType, std::size_t DimensionCount>
struct coordinate_type<boost::array<CoordinateType, DimensionCount> >
{
    typedef CoordinateType type;
};


template <typename CoordinateType, std::size_t DimensionCount>
struct dimension<boost::array<CoordinateType, DimensionCount> >
    : std::integral_constant<std::size_t, DimensionCount>
{};


template <typename CoordinateType, std::size_t DimensionCount, std::size_t Dimension>
struct access<boost::array<CoordinateType, DimensionCount>, Dimension>
{
    static inline CoordinateType get(boost::array<CoordinateType, DimensionCount> const& a)
    {
        return a[Dimension];
    }

    static inline void set(boost::array<CoordinateType, DimensionCount>& a,
        CoordinateType const& value)
    {
        a[Dimension] = value;
    }
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#define BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(CoordinateSystem) \
    namespace boost { namespace geometry { namespace traits { \
    template <class T, std::size_t N> \
    struct coordinate_system<boost::array<T, N> > \
    { \
        typedef CoordinateSystem type; \
    }; \
    }}}


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ARRAY_HPP

