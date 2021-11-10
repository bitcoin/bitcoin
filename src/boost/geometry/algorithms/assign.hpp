// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP


#include <cstddef>

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

namespace boost { namespace geometry
{

/*!
\brief Assign a range of points to a linestring, ring or polygon
\note The point-type of the range might be different from the point-type of the geometry
\ingroup assign
\tparam Geometry \tparam_geometry
\tparam Range \tparam_range_point
\param geometry \param_geometry
\param range \param_range_point

\qbk{
[heading Notes]
[note Assign automatically clears the geometry before assigning (use append if you don't want that)]
[heading Example]
[assign_points] [assign_points_output]

[heading See also]
\* [link geometry.reference.algorithms.append append]
}
 */
template <typename Geometry, typename Range>
inline void assign_points(Geometry& geometry, Range const& range)
{
    concepts::check<Geometry>();

    clear(geometry);
    geometry::append(geometry, range, -1, 0);
}


/*!
\brief assign to a box inverse infinite
\details The assign_inverse function initialize a 2D or 3D box with large coordinates, the
min corner is very large, the max corner is very small. This is a convenient starting point to
collect the minimum bounding box of a geometry.
\ingroup assign
\tparam Geometry \tparam_geometry
\param geometry \param_geometry

\qbk{
[heading Example]
[assign_inverse] [assign_inverse_output]

[heading See also]
\* [link geometry.reference.algorithms.make.make_inverse make_inverse]
}
 */
template <typename Geometry>
inline void assign_inverse(Geometry& geometry)
{
    concepts::check<Geometry>();

    dispatch::assign_inverse
        <
            typename tag<Geometry>::type,
            Geometry
        >::apply(geometry);
}

/*!
\brief assign zero values to a box, point
\ingroup assign
\details The assign_zero function initializes a 2D or 3D point or box with coordinates of zero
\tparam Geometry \tparam_geometry
\param geometry \param_geometry

 */
template <typename Geometry>
inline void assign_zero(Geometry& geometry)
{
    concepts::check<Geometry>();

    dispatch::assign_zero
        <
            typename tag<Geometry>::type,
            Geometry
        >::apply(geometry);
}

/*!
\brief Assign two coordinates to a geometry (usually a 2D point)
\ingroup assign
\tparam Geometry \tparam_geometry
\tparam Type \tparam_numeric to specify the coordinates
\param geometry \param_geometry
\param c1 \param_x
\param c2 \param_y

\qbk{distinguish, 2 coordinate values}
\qbk{
[heading Example]
[assign_2d_point] [assign_2d_point_output]

[heading See also]
\* [link geometry.reference.algorithms.make.make_2_2_coordinate_values make]
}
 */
template <typename Geometry, typename Type>
inline void assign_values(Geometry& geometry, Type const& c1, Type const& c2)
{
    concepts::check<Geometry>();

    dispatch::assign
        <
            typename tag<Geometry>::type,
            Geometry,
            geometry::dimension<Geometry>::type::value
        >::apply(geometry, c1, c2);
}

/*!
\brief Assign three values to a geometry (usually a 3D point)
\ingroup assign
\tparam Geometry \tparam_geometry
\tparam Type \tparam_numeric to specify the coordinates
\param geometry \param_geometry
\param c1 \param_x
\param c2 \param_y
\param c3 \param_z

\qbk{distinguish, 3 coordinate values}
\qbk{
[heading Example]
[assign_3d_point] [assign_3d_point_output]

[heading See also]
\* [link geometry.reference.algorithms.make.make_3_3_coordinate_values make]
}
 */
template <typename Geometry, typename Type>
inline void assign_values(Geometry& geometry,
            Type const& c1, Type const& c2, Type const& c3)
{
    concepts::check<Geometry>();

    dispatch::assign
        <
            typename tag<Geometry>::type,
            Geometry,
            geometry::dimension<Geometry>::type::value
        >::apply(geometry, c1, c2, c3);
}

/*!
\brief Assign four values to a geometry (usually a box or segment)
\ingroup assign
\tparam Geometry \tparam_geometry
\tparam Type \tparam_numeric to specify the coordinates
\param geometry \param_geometry
\param c1 First coordinate (usually x1)
\param c2 Second coordinate (usually y1)
\param c3 Third coordinate (usually x2)
\param c4 Fourth coordinate (usually y2)

\qbk{distinguish, 4 coordinate values}
 */
template <typename Geometry, typename Type>
inline void assign_values(Geometry& geometry,
                Type const& c1, Type const& c2, Type const& c3, Type const& c4)
{
    concepts::check<Geometry>();

    dispatch::assign
        <
            typename tag<Geometry>::type,
            Geometry,
            geometry::dimension<Geometry>::type::value
        >::apply(geometry, c1, c2, c3, c4);
}



namespace resolve_variant
{

template <typename Geometry1, typename Geometry2>
struct assign
{
    static inline void
    apply(Geometry1& geometry1, const Geometry2& geometry2)
    {
        concepts::check<Geometry1>();
        concepts::check<Geometry2 const>();
        concepts::check_concepts_and_equal_dimensions<Geometry1, Geometry2 const>();
            
        static bool const same_point_order
            = point_order<Geometry1>::value == point_order<Geometry2>::value;
        BOOST_GEOMETRY_STATIC_ASSERT(
            same_point_order,
            "Assign is not supported for different point orders.",
            Geometry1, Geometry2);
        static bool const same_closure
            = closure<Geometry1>::value == closure<Geometry2>::value;
        BOOST_GEOMETRY_STATIC_ASSERT(
            same_closure,
            "Assign is not supported for different closures.",
            Geometry1, Geometry2);
            
        dispatch::convert<Geometry2, Geometry1>::apply(geometry2, geometry1);
    }
};
    
    
template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct assign<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    struct visitor: static_visitor<void>
    {
        Geometry2 const& m_geometry2;
            
        visitor(Geometry2 const& geometry2)
        : m_geometry2(geometry2)
        {}
            
        template <typename Geometry1>
        result_type operator()(Geometry1& geometry1) const
        {
            return assign
            <
                Geometry1,
                Geometry2
            >::apply
            (geometry1, m_geometry2);
        }
    };
        
    static inline void
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)>& geometry1,
          Geometry2 const& geometry2)
    {
        return boost::apply_visitor(visitor(geometry2), geometry1);
    }
};
    
    
template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct assign<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor: static_visitor<void>
    {
        Geometry1& m_geometry1;
            
        visitor(Geometry1 const& geometry1)
        : m_geometry1(geometry1)
        {}
            
        template <typename Geometry2>
        result_type operator()(Geometry2 const& geometry2) const
        {
            return assign
            <
                Geometry1,
                Geometry2
            >::apply
            (m_geometry1, geometry2);
        }
    };
        
    static inline void
    apply(Geometry1& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2)
    {
        return boost::apply_visitor(visitor(geometry1), geometry2);
    }
};
    
    
template <BOOST_VARIANT_ENUM_PARAMS(typename T1), BOOST_VARIANT_ENUM_PARAMS(typename T2)>
struct assign<variant<BOOST_VARIANT_ENUM_PARAMS(T1)>, variant<BOOST_VARIANT_ENUM_PARAMS(T2)> >
{
    struct visitor: static_visitor<void>
    {
        template <typename Geometry1, typename Geometry2>
        result_type operator()(
                                Geometry1& geometry1,
                                Geometry2 const& geometry2) const
        {
            return assign
            <
                Geometry1,
                Geometry2
            >::apply
            (geometry1, geometry2);
        }
    };
        
    static inline void
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T1)>& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2)
    {
        return boost::apply_visitor(visitor(), geometry1, geometry2);
    }
};
    
} // namespace resolve_variant
    

/*!
\brief Assigns one geometry to another geometry
\details The assign algorithm assigns one geometry, e.g. a BOX, to another
geometry, e.g. a RING. This only works if it is possible and applicable.
\ingroup assign
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry (target)
\param geometry2 \param_geometry (source)

\qbk{
[heading Example]
[assign] [assign_output]

[heading See also]
\* [link geometry.reference.algorithms.convert convert]
}
 */
template <typename Geometry1, typename Geometry2>
inline void assign(Geometry1& geometry1, Geometry2 const& geometry2)
{
    resolve_variant::assign<Geometry1, Geometry2>::apply(geometry1, geometry2);
}


}} // namespace boost::geometry



#endif // BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP
