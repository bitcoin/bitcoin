// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP
#define BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_type.hpp>


#include <boost/geometry/util/math.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template<std::size_t Dimension, typename Geometry>
struct degree_radian_converter
{
    typedef typename fp_coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return boost::numeric_cast
            <
                coordinate_type
            >(geometry::get<Dimension>(geometry)
              * math::d2r<coordinate_type>());
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Dimension>(geometry, boost::numeric_cast
            <
                coordinate_type
            >(radians * math::r2d<coordinate_type>()));
    }

};


// Default, radian (or any other coordinate system) just works like "get"
template <std::size_t Dimension, typename Geometry, typename DegreeOrRadian>
struct radian_access
{
    typedef typename fp_coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return geometry::get<Dimension>(geometry);
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Dimension>(geometry, radians);
    }
};

// Specialize, any "degree" coordinate system will be converted to radian
// but only for dimension 0,1 (so: dimension 2 and heigher are untouched)

template
<
    typename Geometry,
    template<typename> class CoordinateSystem
>
struct radian_access<0, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter<0, Geometry>
{};


template
<
    typename Geometry,
    template<typename> class CoordinateSystem
>
struct radian_access<1, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter<1, Geometry>
{};


template<std::size_t Index, std::size_t Dimension, typename Geometry>
struct degree_radian_converter_box_segment
{
    typedef typename fp_coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return boost::numeric_cast
            <
                coordinate_type
            >(geometry::get<Index, Dimension>(geometry)
              * math::d2r<coordinate_type>());
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Index, Dimension>(geometry, boost::numeric_cast
            <
                coordinate_type
            >(radians * math::r2d<coordinate_type>()));
    }

};


// Default, radian (or any other coordinate system) just works like "get"
template <std::size_t Index, std::size_t Dimension, typename Geometry, typename DegreeOrRadian>
struct radian_access_box_segment
{
    typedef typename fp_coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return geometry::get<Index, Dimension>(geometry);
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Index, Dimension>(geometry, radians);
    }
};

// Specialize, any "degree" coordinate system will be converted to radian
// but only for dimension 0,1 (so: dimension 2 and heigher are untouched)

template
<
    typename Geometry,
    template<typename> class CoordinateSystem,
    std::size_t Index
>
struct radian_access_box_segment<Index, 0, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter_box_segment<Index, 0, Geometry>
{};


template
<
    typename Geometry,
    template<typename> class CoordinateSystem,
    std::size_t Index
>
struct radian_access_box_segment<Index, 1, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter_box_segment<Index, 1, Geometry>
{};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief get coordinate value of a point, result is in Radian
\details Result is in Radian, even if source coordinate system
    is in Degrees
\return coordinate value
\ingroup get
\tparam Dimension dimension
\tparam Geometry geometry
\param geometry geometry to get coordinate value from
\note Only applicable to coordinate systems templatized by units,
    e.g. spherical or geographic coordinate systems
*/
template <std::size_t Dimension, typename Geometry>
inline typename fp_coordinate_type<Geometry>::type get_as_radian(Geometry const& geometry)
{
    return detail::radian_access<Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::get(geometry);
}

/*!
\brief set coordinate value (in radian) to a point
\details Coordinate value will be set correctly, if coordinate system of
    point is in Degree, Radian value will be converted to Degree
\ingroup set
\tparam Dimension dimension
\tparam Geometry geometry
\param geometry geometry to assign coordinate to
\param radians coordinate value to assign
\note Only applicable to coordinate systems templatized by units,
    e.g. spherical or geographic coordinate systems
*/
template <std::size_t Dimension, typename Geometry>
inline void set_from_radian(Geometry& geometry,
            typename fp_coordinate_type<Geometry>::type const& radians)
{
    detail::radian_access<Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::set(geometry, radians);
}

/*!
\brief get coordinate value of a segment or box, result is in Radian
\details Result is in Radian, even if source coordinate system
    is in Degrees
\return coordinate value
\ingroup get
\tparam Index index
\tparam Dimension dimension
\tparam Geometry geometry
\param geometry geometry to get coordinate value from
\note Only applicable to coordinate systems templatized by units,
    e.g. spherical or geographic coordinate systems
*/
template <std::size_t Index, std::size_t Dimension, typename Geometry>
inline typename fp_coordinate_type<Geometry>::type get_as_radian(Geometry const& geometry)
{
    return detail::radian_access_box_segment<Index, Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::get(geometry);
}

/*!
\brief set coordinate value (in radian) to a segment or box
\details Coordinate value will be set correctly, if coordinate system of
    point is in Degree, Radian value will be converted to Degree
\ingroup set
\tparam Index index
\tparam Dimension dimension
\tparam Geometry geometry
\param geometry geometry to assign coordinate to
\param radians coordinate value to assign
\note Only applicable to coordinate systems templatized by units,
    e.g. spherical or geographic coordinate systems
*/
template <std::size_t Index, std::size_t Dimension, typename Geometry>
inline void set_from_radian(Geometry& geometry,
            typename fp_coordinate_type<Geometry>::type const& radians)
{
    detail::radian_access_box_segment<Index, Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::set(geometry, radians);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP
