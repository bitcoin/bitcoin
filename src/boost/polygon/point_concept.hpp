// Boost.Polygon library point_concept.hpp header file

// Copyright (c) Intel Corporation 2008.
// Copyright (c) 2008-2012 Simonson Lucanus.
// Copyright (c) 2012-2012 Andrii Sydorchuk.

// See http://www.boost.org for updates, documentation, and revision history.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_POLYGON_POINT_CONCEPT_HPP
#define BOOST_POLYGON_POINT_CONCEPT_HPP

#include "isotropy.hpp"
#include "point_traits.hpp"

namespace boost {
namespace polygon {

struct point_concept {};

template <typename ConceptType>
struct is_point_concept {
  typedef gtl_no type;
};

template <>
struct is_point_concept<point_concept> {
  typedef gtl_yes type;
};

template <typename ConceptType>
struct is_mutable_point_concept {
  typedef gtl_no type;
};

template <>
struct is_mutable_point_concept<point_concept> {
  typedef gtl_yes type;
};

template <typename GeometryType, typename BoolType>
struct point_coordinate_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct point_coordinate_type_by_concept<GeometryType, gtl_yes> {
  typedef typename point_traits<GeometryType>::coordinate_type type;
};

template <typename GeometryType>
struct point_coordinate_type {
  typedef typename point_coordinate_type_by_concept<
    GeometryType,
    typename is_point_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

template <typename GeometryType, typename BoolType>
struct point_difference_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct point_difference_type_by_concept<GeometryType, gtl_yes> {
  typedef typename coordinate_traits<
    typename point_traits<GeometryType>::coordinate_type
  >::coordinate_difference type;
};

template <typename GeometryType>
struct point_difference_type {
  typedef typename point_difference_type_by_concept<
    GeometryType,
    typename is_point_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

template <typename GeometryType, typename BoolType>
struct point_distance_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct point_distance_type_by_concept<GeometryType, gtl_yes> {
  typedef typename coordinate_traits<
    typename point_coordinate_type<GeometryType>::type
  >::coordinate_distance type;
};

template <typename GeometryType>
struct point_distance_type {
  typedef typename point_distance_type_by_concept<
    GeometryType,
    typename is_point_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

struct y_pt_get : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_pt_get,
    typename is_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
  typename point_coordinate_type<PointType>::type
>::type get(const PointType& point, orientation_2d orient) {
  return point_traits<PointType>::get(point, orient);
}

struct y_pt_set : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_pt_set,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
  void
>::type set(PointType& point, orientation_2d orient,
    typename point_mutable_traits<PointType>::coordinate_type value) {
  point_mutable_traits<PointType>::set(point, orient, value);
}

struct y_pt_construct : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_pt_construct,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type construct(
    typename point_mutable_traits<PointType>::coordinate_type x,
    typename point_mutable_traits<PointType>::coordinate_type y) {
  return point_mutable_traits<PointType>::construct(x, y);
}

struct y_pt_assign : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_assign,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
>::type,
PointType1>::type& assign(PointType1& lvalue, const PointType2& rvalue) {
  set(lvalue, HORIZONTAL, get(rvalue, HORIZONTAL));
  set(lvalue, VERTICAL, get(rvalue, VERTICAL));
  return lvalue;
}

struct y_p_x : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_p_x,
    typename is_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
  typename point_coordinate_type<PointType>::type
>::type x(const PointType& point) {
  return get(point, HORIZONTAL);
}

struct y_p_y : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_p_y,
    typename is_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
  typename point_coordinate_type<PointType>::type
>::type y(const PointType& point) {
  return get(point, VERTICAL);
}

struct y_p_sx : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_p_sx,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
void>::type x(PointType& point,
    typename point_mutable_traits<PointType>::coordinate_type value) {
  set(point, HORIZONTAL, value);
}

struct y_p_sy : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_p_sy,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
void>::type y(PointType& point,
    typename point_mutable_traits<PointType>::coordinate_type value) {
  set(point, VERTICAL, value);
}

struct y_pt_equiv : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_equiv,
    typename is_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
bool>::type equivalence(
    const PointType1& point1, const PointType2& point2) {
  return (x(point1) == x(point2)) && (y(point1) == y(point2));
}

struct y_pt_man_dist : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_man_dist,
    typename is_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
typename point_difference_type<PointType1>::type>::type
manhattan_distance(const PointType1& point1, const PointType2& point2) {
  return euclidean_distance(point1, point2, HORIZONTAL) +
         euclidean_distance(point1, point2, VERTICAL);
}

struct y_pt_ed1 : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_ed1,
    typename is_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
typename point_difference_type<PointType1>::type>::type
euclidean_distance(
    const PointType1& point1,
    const PointType2& point2,
    orientation_2d orient) {
  typename point_difference_type<PointType1>::type dif =
      get(point1, orient) - get(point2, orient);
  return (dif < 0) ? -dif : dif;
}

struct y_pt_eds : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_eds,
    typename is_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
typename point_difference_type<PointType1>::type>::type
distance_squared(const PointType1& point1, const PointType2& point2) {
  typename point_difference_type<PointType1>::type dx =
      euclidean_distance(point1, point2, HORIZONTAL);
  typename point_difference_type<PointType1>::type dy =
      euclidean_distance(point1, point2, VERTICAL);
  dx *= dx;
  dy *= dy;
  return dx + dy;
}

struct y_pt_ed2 : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_ed2,
    typename is_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
typename point_distance_type<PointType1>::type>::type
euclidean_distance(const PointType1& point1, const PointType2& point2) {
  return (std::sqrt)(
      static_cast<double>(distance_squared(point1, point2)));
}

struct y_pt_convolve : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_convolve,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
PointType1>::type& convolve(PointType1& lvalue, const PointType2& rvalue) {
  x(lvalue, x(lvalue) + x(rvalue));
  y(lvalue, y(lvalue) + y(rvalue));
  return lvalue;
}

struct y_pt_deconvolve : gtl_yes {};

template <typename PointType1, typename PointType2>
typename enable_if<
  typename gtl_and_3<
    y_pt_deconvolve,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<PointType2>::type
    >::type
  >::type,
PointType1>::type& deconvolve(PointType1& lvalue, const PointType2& rvalue) {
  x(lvalue, x(lvalue) - x(rvalue));
  y(lvalue, y(lvalue) - y(rvalue));
  return lvalue;
}

struct y_pt_scale_up : gtl_yes {};

template <typename PointType, typename CType>
typename enable_if<
  typename gtl_and<
    y_pt_scale_up,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type& scale_up(PointType& point, CType factor) {
  typedef typename point_coordinate_type<PointType>::type Unit;
  x(point, x(point) * (Unit)factor);
  y(point, y(point) * (Unit)factor);
  return point;
}

struct y_pt_scale_down : gtl_yes {};

template <typename PointType, typename CType>
typename enable_if<
  typename gtl_and<
    y_pt_scale_down,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type& scale_down(PointType& point, CType factor) {
  typedef typename point_coordinate_type<PointType>::type Unit;
  typedef typename coordinate_traits<Unit>::coordinate_distance dt;
  x(point, scaling_policy<Unit>::round((dt)(x(point)) / (dt)factor));
  y(point, scaling_policy<Unit>::round((dt)(y(point)) / (dt)factor));
  return point;
}

struct y_pt_scale : gtl_yes {};

template <typename PointType, typename ScaleType>
typename enable_if<
  typename gtl_and<
    y_pt_scale,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type& scale(PointType& point, const ScaleType& scaling) {
  typedef typename point_coordinate_type<PointType>::type Unit;
  Unit x_coord(x(point));
  Unit y_coord(y(point));
  scaling.scale(x_coord, y_coord);
  x(point, x_coord);
  y(point, y_coord);
  return point;
}

struct y_pt_transform : gtl_yes {};

template <typename PointType, typename TransformType>
typename enable_if<
  typename gtl_and<
    y_pt_transform,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type& transform(PointType& point, const TransformType& transform) {
  typedef typename point_coordinate_type<PointType>::type Unit;
  Unit x_coord(x(point));
  Unit y_coord(y(point));
  transform.transform(x_coord, y_coord);
  x(point, x_coord);
  y(point, y_coord);
  return point;
}

struct y_pt_move : gtl_yes {};

template <typename PointType>
typename enable_if<
  typename gtl_and<
    y_pt_move,
    typename is_mutable_point_concept<
      typename geometry_concept<PointType>::type
    >::type
  >::type,
PointType>::type& move(PointType& point, orientation_2d orient,
    typename point_coordinate_type<PointType>::type displacement) {
  typedef typename point_coordinate_type<PointType>::type Unit;
  Unit coord = get(point, orient);
  set(point, orient, coord + displacement);
  return point;
}
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_POINT_CONCEPT_HPP
