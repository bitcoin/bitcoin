// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_REGISTER_POINT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_REGISTER_POINT_HPP


#include <cstddef>
#include <type_traits>


#ifndef DOXYGEN_NO_SPECIALIZATIONS

// Starting point, specialize basic traits necessary to register a point
#define BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, Dim, CoordinateType, CoordinateSystem) \
    template<> struct tag<Point> { typedef point_tag type; }; \
    template<> struct dimension<Point> : std::integral_constant<std::size_t, Dim> {}; \
    template<> struct coordinate_type<Point> { typedef CoordinateType type; }; \
    template<> struct coordinate_system<Point> { typedef CoordinateSystem type; };

// Specialize access class per dimension
#define BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, Dim, CoordinateType, Get, Set) \
    template<> struct access<Point, Dim> \
    { \
        static inline CoordinateType get(Point const& p) { return p. Get; } \
        static inline void set(Point& p, CoordinateType const& value) { p. Set = value; } \
    };

// Const version
#define BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, Dim, CoordinateType, Get) \
    template<> struct access<Point, Dim> \
    { \
        static inline CoordinateType get(Point const& p) { return p. Get; } \
    };


// Getter/setter version
#define BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, Dim, CoordinateType, Get, Set) \
    template<> struct access<Point, Dim> \
    { \
        static inline CoordinateType get(Point const& p) \
        { return  p. Get (); } \
        static inline void set(Point& p, CoordinateType const& value) \
        { p. Set ( value ); } \
    };

#endif // DOXYGEN_NO_SPECIALIZATIONS


/*!
\brief \brief_macro{2D point type}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_2D, two-dimensional point type}
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Field0 \param_macro_member{\macro_x}
\param Field1 \param_macro_member{\macro_y}

\qbk{[include reference/geometries/register/point.qbk]}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_2D(Point, CoordinateType, CoordinateSystem, Field0, Field1) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 2, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, 0, CoordinateType, Field0, Field0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, 1, CoordinateType, Field1, Field1) \
}}}

/*!
\brief \brief_macro{3D point type}
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_3D, three-dimensional point type}
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Field0 \param_macro_member{\macro_x}
\param Field1 \param_macro_member{\macro_y}
\param Field2 \param_macro_member{\macro_z}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_3D(Point, CoordinateType, CoordinateSystem, Field0, Field1, Field2) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 3, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, 0, CoordinateType, Field0, Field0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, 1, CoordinateType, Field1, Field1) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS(Point, 2, CoordinateType, Field2, Field2) \
}}}

/*!
\brief \brief_macro{2D point type} \brief_macro_const
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_2D_CONST, two-dimensional point type}. \details_macro_const
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Field0 \param_macro_member{\macro_x}
\param Field1 \param_macro_member{\macro_y}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_2D_CONST(Point, CoordinateType, CoordinateSystem, Field0, Field1) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 2, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, 0, CoordinateType, Field0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, 1, CoordinateType, Field1) \
}}}

/*!
\brief \brief_macro{3D point type} \brief_macro_const
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_3D_CONST, three-dimensional point type}. \details_macro_const
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Field0 \param_macro_member{\macro_x}
\param Field1 \param_macro_member{\macro_y}
\param Field2 \param_macro_member{\macro_z}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_3D_CONST(Point, CoordinateType, CoordinateSystem, Field0, Field1, Field2) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 3, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, 0, CoordinateType, Field0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, 1, CoordinateType, Field1) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_CONST(Point, 2, CoordinateType, Field2) \
}}}

/*!
\brief \brief_macro{2D point type} \brief_macro_getset
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET, two-dimensional point type}. \details_macro_getset
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Get0 \param_macro_getset{get, \macro_x}
\param Get1 \param_macro_getset{get, \macro_y}
\param Set0 \param_macro_getset{set, \macro_x}
\param Set1 \param_macro_getset{set, \macro_y}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(Point, CoordinateType, CoordinateSystem, Get0, Get1, Set0, Set1) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 2, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, 0, CoordinateType, Get0, Set0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, 1, CoordinateType, Get1, Set1) \
}}}

/*!
\brief \brief_macro{3D point type} \brief_macro_getset
\ingroup register
\details \details_macro{BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET, three-dimensional point type}. \details_macro_getset
\param Point \param_macro_type{Point}
\param CoordinateType \param_macro_coortype{point}
\param CoordinateSystem \param_macro_coorsystem
\param Get0 \param_macro_getset{get, \macro_x}
\param Get1 \param_macro_getset{get, \macro_y}
\param Get2 \param_macro_getset{get, \macro_z}
\param Set0 \param_macro_getset{set, \macro_x}
\param Set1 \param_macro_getset{set, \macro_y}
\param Set2 \param_macro_getset{set, \macro_z}
*/
#define BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET(Point, CoordinateType, CoordinateSystem, Get0, Get1, Get2, Set0, Set1, Set2) \
namespace boost { namespace geometry { namespace traits {  \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(Point, 3, CoordinateType, CoordinateSystem) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, 0, CoordinateType, Get0, Set0) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, 1, CoordinateType, Get1, Set1) \
    BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_ACCESS_GET_SET(Point, 2, CoordinateType, Get2, Set2) \
}}}

#endif // BOOST_GEOMETRY_GEOMETRIES_REGISTER_POINT_HPP
