// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_CORE_RADIUS_HPP
#define BOOST_GEOMETRY_CORE_RADIUS_HPP


#include <cstddef>

#include <boost/static_assert.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry
{

namespace traits
{

/*!
    \brief Traits class to get/set radius of a circle/sphere/(ellipse)
    \details the radius access meta-functions give read/write access to the radius of a circle or a sphere,
    or to the major/minor axis or an ellipse, or to one of the 3 equatorial radii of an ellipsoid.

    It should be specialized per geometry, in namespace core_dispatch. Those specializations should
    forward the call via traits to the geometry class, which could be specified by the user.

    There is a corresponding generic radius_get and radius_set function
    \par Geometries:
        - n-sphere (circle,sphere)
        - upcoming ellipse
    \par Specializations should provide:
        - inline static T get(Geometry const& geometry)
        - inline static void set(Geometry& geometry, T const& radius)
    \ingroup traits
*/
template <typename Geometry, std::size_t Dimension>
struct radius_access {};


/*!
    \brief Traits class indicating the type (double,float,...) of the radius of a circle or a sphere
    \par Geometries:
        - n-sphere (circle,sphere)
        - upcoming ellipse
    \par Specializations should provide:
        - typedef T type (double,float,int,etc)
    \ingroup traits
*/
template <typename Geometry>
struct radius_type {};

} // namespace traits


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Tag, typename Geometry>
struct radius_type
{
    //typedef core_dispatch_specialization_required type;
};

/*!
    \brief radius access meta-functions, used by concept n-sphere and upcoming ellipse.
*/
template <typename Tag,
          typename Geometry,
          std::size_t Dimension,
          typename IsPointer>
struct radius_access
{
    //static inline CoordinateType get(Geometry const& ) {}
    //static inline void set(Geometry& g, CoordinateType const& value) {}
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Metafunction to get the type of radius of a circle / sphere / ellipse / etc.
    \ingroup access
    \tparam Geometry the type of geometry
*/
template <typename Geometry>
struct radius_type
{
    typedef typename core_dispatch::radius_type
                        <
                            typename tag<Geometry>::type,
                            typename util::remove_cptrref<Geometry>::type
                        >::type type;
};

/*!
    \brief Function to get radius of a circle / sphere / ellipse / etc.
    \return radius The radius for a given axis
    \ingroup access
    \param geometry the geometry to get the radius from
    \tparam I index of the axis
*/
template <std::size_t I, typename Geometry>
inline typename radius_type<Geometry>::type get_radius(Geometry const& geometry)
{
    return core_dispatch::radius_access
            <
                typename tag<Geometry>::type,
                typename util::remove_cptrref<Geometry>::type,
                I,
                typename std::is_pointer<Geometry>::type
            >::get(geometry);
}

/*!
    \brief Function to set the radius of a circle / sphere / ellipse / etc.
    \ingroup access
    \tparam I index of the axis
    \param geometry the geometry to change
    \param radius the radius to set
*/
template <std::size_t I, typename Geometry>
inline void set_radius(Geometry& geometry,
                       typename radius_type<Geometry>::type const& radius)
{
    core_dispatch::radius_access
        <
            typename tag<Geometry>::type,
            typename util::remove_cptrref<Geometry>::type,
            I,
            typename std::is_pointer<Geometry>::type
        >::set(geometry, radius);
}



#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Tag, typename Geometry, std::size_t Dimension>
struct radius_access
{
    static inline typename radius_type<Geometry>::type get(Geometry const& geometry)
    {
        return traits::radius_access<Geometry, Dimension>::get(geometry);
    }
    static inline void set(Geometry& geometry,
                           typename radius_type<Geometry>::type const& value)
    {
        traits::radius_access<Geometry, Dimension>::set(geometry, value);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Tag,
          typename Geometry,
          std::size_t Dimension>
struct radius_access<Tag, Geometry, Dimension, std::true_type>
{
    typedef typename geometry::radius_type<Geometry>::type radius_type;

    static inline radius_type get(const Geometry * geometry)
    {
        return radius_access
                <
                    Tag,
                    Geometry,
                    Dimension,
                    typename std::is_pointer<Geometry>::type
                >::get(*geometry);
    }

    static inline void set(Geometry * geometry, radius_type const& value)
    {
        return radius_access
                <
                    Tag,
                    Geometry,
                    Dimension,
                    typename std::is_pointer<Geometry>::type
                >::set(*geometry, value);
    }
};


template <typename Geometry>
struct radius_type<srs_sphere_tag, Geometry>
{
    typedef typename traits::radius_type<Geometry>::type type;
};

template <typename Geometry, std::size_t Dimension>
struct radius_access<srs_sphere_tag, Geometry, Dimension, std::false_type>
    : detail::radius_access<srs_sphere_tag, Geometry, Dimension>
{
    //BOOST_STATIC_ASSERT(Dimension == 0);
    BOOST_STATIC_ASSERT(Dimension < 3);
};

template <typename Geometry>
struct radius_type<srs_spheroid_tag, Geometry>
{
    typedef typename traits::radius_type<Geometry>::type type;
};

template <typename Geometry, std::size_t Dimension>
struct radius_access<srs_spheroid_tag, Geometry, Dimension, std::false_type>
    : detail::radius_access<srs_spheroid_tag, Geometry, Dimension>
{
    //BOOST_STATIC_ASSERT(Dimension == 0 || Dimension == 2);
    BOOST_STATIC_ASSERT(Dimension < 3);
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_RADIUS_HPP
