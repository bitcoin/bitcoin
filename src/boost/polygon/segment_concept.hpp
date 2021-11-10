// Boost.Polygon library segment_concept.hpp header file

// Copyright (c) Intel Corporation 2008.
// Copyright (c) 2008-2012 Simonson Lucanus.
// Copyright (c) 2012-2012 Andrii Sydorchuk.

// See http://www.boost.org for updates, documentation, and revision history.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_POLYGON_SEGMENT_CONCEPT_HPP
#define BOOST_POLYGON_SEGMENT_CONCEPT_HPP

#include "isotropy.hpp"
#include "segment_traits.hpp"
#include "rectangle_concept.hpp"

namespace boost {
namespace polygon {

struct segment_concept {};

template <typename ConceptType>
struct is_segment_concept {
  typedef gtl_no type;
};

template <>
struct is_segment_concept<segment_concept> {
  typedef gtl_yes type;
};

template <typename ConceptType>
struct is_mutable_segment_concept {
  typedef gtl_no type;
};

template <>
struct is_mutable_segment_concept<segment_concept> {
  typedef gtl_yes type;
};

template <typename GeometryType, typename BoolType>
struct segment_distance_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct segment_distance_type_by_concept<GeometryType, gtl_yes> {
  typedef typename coordinate_traits<
    typename segment_traits<GeometryType>::coordinate_type
  >::coordinate_distance type;
};

template <typename GeometryType>
struct segment_distance_type {
  typedef typename segment_distance_type_by_concept<
    GeometryType,
    typename is_segment_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

template <typename GeometryType, typename BoolType>
struct segment_point_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct segment_point_type_by_concept<GeometryType, gtl_yes> {
  typedef typename segment_traits<GeometryType>::point_type type;
};

template <typename GeometryType>
struct segment_point_type {
  typedef typename segment_point_type_by_concept<
    GeometryType,
    typename is_segment_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

template <typename GeometryType, typename BoolType>
struct segment_coordinate_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct segment_coordinate_type_by_concept<GeometryType, gtl_yes> {
  typedef typename segment_traits<GeometryType>::coordinate_type type;
};

template <typename GeometryType>
struct segment_coordinate_type {
  typedef typename segment_coordinate_type_by_concept<
    GeometryType,
    typename is_segment_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

struct y_s_get : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_get,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
typename segment_point_type<Segment>::type>::type
get(const Segment& segment, direction_1d dir) {
  return segment_traits<Segment>::get(segment, dir);
}

struct y_s_set : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_set,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
void>::type set(Segment& segment, direction_1d dir, const Point& point) {
  segment_mutable_traits<Segment>::set(segment, dir, point);
}

struct y_s_construct : gtl_yes {};

template <typename Segment, typename Point1, typename Point2>
typename enable_if<
  typename gtl_and_4<
    y_s_construct,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point1>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point2>::type
    >::type
  >::type,
Segment>::type construct(const Point1& low, const Point2& high) {
  return segment_mutable_traits<Segment>::construct(low, high);
}

struct y_s_copy_construct : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_copy_construct,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
Segment1>::type copy_construct(const Segment2& segment) {
  return construct<Segment1>(get(segment, LOW), get(segment, HIGH));
}

struct y_s_assign : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_assign,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
Segment1>::type& assign(Segment1& segment1, const Segment2& segment2) {
  return segment1 = copy_construct<Segment1>(segment2);
}

struct y_s_equivalence : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_equivalence,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
bool>::type equivalence(const Segment1& segment1, const Segment2& segment2) {
  return get(segment1, LOW) == get(segment2, LOW) &&
         get(segment1, HIGH) == get(segment2, HIGH);
}

struct y_s_low : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_low,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
typename segment_point_type<Segment>::type>::type low(const Segment& segment) {
  return get(segment, LOW);
}

struct y_s_high : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_high,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
typename segment_point_type<Segment>::type>::type high(const Segment& segment) {
  return get(segment, HIGH);
}

struct y_s_center : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_center,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
typename segment_point_type<Segment>::type>::type
center(const Segment& segment) {
  return construct<typename segment_point_type<Segment>::type>(
      (x(high(segment)) + x(low(segment)))/2,
      (y(high(segment)) + y(low(segment)))/2);
}

struct y_s_low2 : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_low2,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
void>::type low(Segment& segment, const Point& point) {
  set(segment, LOW, point);
}

struct y_s_high2 : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_high2,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
void>::type high(Segment& segment, const Point& point) {
  set(segment, HIGH, point);
}

struct y_s_orientation1 : gtl_yes {};

// -1 for CW, 0 for collinear and 1 for CCW.
template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_orientation1,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
int>::type orientation(const Segment1& segment1, const Segment2& segment2) {
  typedef typename coordinate_traits<
    typename segment_traits<Segment1>::coordinate_type
  >::manhattan_area_type int_x2;
  typedef typename coordinate_traits<
    typename segment_traits<Segment1>::coordinate_type
  >::unsigned_area_type uint_x2;
  int_x2 a1 = (int_x2)x(high(segment1)) - (int_x2)x(low(segment1));
  int_x2 b1 = (int_x2)y(high(segment1)) - (int_x2)y(low(segment1));
  int_x2 a2 = (int_x2)x(high(segment2)) - (int_x2)x(low(segment2));
  int_x2 b2 = (int_x2)y(high(segment2)) - (int_x2)y(low(segment2));

  int sign1 = 0;
  int sign2 = 0;
  if (a1 && b2)
    sign1 = ((a1 > 0) ^ (b2 > 0)) ? -1 : 1;
  if (a2 && b1)
    sign2 = ((a2 > 0) ^ (b1 > 0)) ? -1 : 1;

  if (sign1 != sign2)
    return (sign1 < sign2) ? -1 : 1;
  uint_x2 a3 = (uint_x2)(a1 < 0 ? -a1 : a1) * (uint_x2)(b2 < 0 ? -b2 : b2);
  uint_x2 b3 = (uint_x2)(b1 < 0 ? -b1 : b1) * (uint_x2)(a2 < 0 ? -a2 : a2);
  if (a3 == b3)
    return 0;
  return ((a3 < b3) ^ (sign1 == 1)) ? 1 : -1;
}

struct y_s_orientation2 : gtl_yes {};

// -1 for right, 0 for collinear and 1 for left.
template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_orientation2,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
int>::type orientation(const Segment& segment, const Point& point) {
  Segment segment2 = construct<Segment>(high(segment), point);
  return orientation(segment, segment2);
}

struct y_s_contains : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_contains,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
bool>::type contains(const Segment& segment,
    const Point& point, bool consider_touch = true ) {
  if (orientation(segment, point))
    return false;
  rectangle_data<typename segment_coordinate_type<Segment>::type> rect;
  set_points(rect, low(segment), high(segment));
  if (!contains(rect, point, true))
    return false;
  if (!consider_touch &&
      (equivalence(low(segment), point) ||
       equivalence(high(segment), point)))
    return false;
  return true;
}

struct y_s_contains2 : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_contains2,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
bool>::type contains(const Segment1& segment1,
    const Segment2& segment2, bool consider_touch = true) {
  return contains(segment1, get(segment2, LOW), consider_touch) &&
         contains(segment1, get(segment2, HIGH), consider_touch);
}

struct y_s_length : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_length,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
typename segment_distance_type<Segment>::type>::type
length(const Segment& segment) {
  return euclidean_distance(low(segment), high(segment));
}

struct y_s_scale_up : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_scale_up,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
Segment>::type& scale_up(Segment& segment,
    typename coordinate_traits<
      typename segment_coordinate_type<Segment>::type
    >::unsigned_area_type factor) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, scale_up(l, factor));
  high(segment, scale_up(h, factor));
  return segment;
}

struct y_s_scale_down : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_scale_down,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
Segment>::type& scale_down(Segment& segment,
    typename coordinate_traits<
      typename segment_coordinate_type<Segment>::type
    >::unsigned_area_type factor) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, scale_down(l, factor));
  high(segment, scale_down(h, factor));
  return segment;
}

struct y_s_scale : gtl_yes {};

template <typename Segment, typename Scale>
typename enable_if<
  typename gtl_and<
    y_s_scale,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
Segment>::type& scale(Segment& segment, const Scale& sc) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, scale(l, sc));
  high(segment, scale(h, sc));
  return segment;
}

struct y_s_transform : gtl_yes {};

template <typename Segment, typename Transform>
typename enable_if<
  typename gtl_and<
    y_s_transform,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
Segment>::type& transform(Segment& segment, const Transform& tr) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, transform(l, tr));
  high(segment, transform(h, tr));
  return segment;
}

struct y_s_move : gtl_yes {};

template <typename Segment>
typename enable_if<
  typename gtl_and<
    y_s_move,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type
  >::type,
Segment>::type& move(Segment& segment, orientation_2d orient,
    typename segment_coordinate_type<Segment>::type displacement) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, move(l, orient, displacement));
  high(segment, move(h, orient, displacement));
  return segment;
}

struct y_s_convolve : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_convolve,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
Segment>::type& convolve(Segment& segment, const Point& point) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, convolve(l, point));
  high(segment, convolve(h, point));
  return segment;
}

struct y_s_deconvolve : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_deconvolve,
    typename is_mutable_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
Segment>::type& deconvolve(Segment& segment, const Point& point) {
  typename segment_point_type<Segment>::type l = low(segment);
  typename segment_point_type<Segment>::type h = high(segment);
  low(segment, deconvolve(l, point));
  high(segment, deconvolve(h, point));
  return segment;
}

struct y_s_abuts1 : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_abuts1,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
bool>::type abuts(const Segment1& segment1,
    const Segment2& segment2, direction_1d dir) {
  return dir.to_int() ? equivalence(low(segment2) , high(segment1)) :
                        equivalence(low(segment1) , high(segment2));
}

struct y_s_abuts2 : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_abuts2,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
bool>::type abuts(const Segment1& segment1, const Segment2& segment2) {
  return abuts(segment1, segment2, HIGH) || abuts(segment1, segment2, LOW);
}

struct y_s_e_intersects : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_e_intersects,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
bool
>::type intersects(const Segment1& segment1, const Segment2& segment2,
    bool consider_touch = true) {
  rectangle_data<typename segment_coordinate_type<Segment1>::type> rect1, rect2;
  set_points(rect1, low(segment1), high(segment1));
  set_points(rect2, low(segment2), high(segment2));
  // Check if axis-parallel rectangles containing segments intersect.
  if (!intersects(rect1, rect2, true))
    return false;
  int or1_1 = orientation(segment1, low(segment2));
  int or1_2 = orientation(segment1, high(segment2));
  if (or1_1 * or1_2 > 0)
    return false;
  int or2_1 = orientation(segment2, low(segment1));
  int or2_2 = orientation(segment2, high(segment1));
  if (or2_1 * or2_2 > 0)
    return false;
  if (consider_touch || (or1_1 && or1_2) || (or2_1 && or2_2))
    return true;
  if (or1_1 || or1_2)
    return false;
  return intersects(vertical(rect1), vertical(rect2), false) ||
         intersects(horizontal(rect1), horizontal(rect2), false);
}

struct y_s_e_dist : gtl_yes {};

template <typename Segment, typename Point>
typename enable_if<
  typename gtl_and_3<
    y_s_e_dist,
    typename is_segment_concept<
      typename geometry_concept<Segment>::type
    >::type,
    typename is_point_concept<
      typename geometry_concept<Point>::type
    >::type
  >::type,
typename segment_distance_type<Segment>::type>::type
euclidean_distance(const Segment& segment, const Point& point) {
  typedef typename segment_distance_type<Segment>::type Unit;
  Unit x1 = x(low(segment));
  Unit y1 = y(low(segment));
  Unit x2 = x(high(segment));
  Unit y2 = y(high(segment));
  Unit X = x(point);
  Unit Y = y(point);
  Unit A = X - x1;
  Unit B = Y - y1;
  Unit C = x2 - x1;
  Unit D = y2 - y1;
  Unit param = (A * C + B * D);
  Unit length_sq = C * C + D * D;
  if (param > length_sq) {
    return euclidean_distance(high(segment), point);
  } else if (param < 0.0) {
    return euclidean_distance(low(segment), point);
  }
  if (length_sq == 0.0)
    return 0.0;
  Unit denom = std::sqrt(length_sq);
  Unit result = (A * D - C * B) / denom;
  return (result < 0.0) ? -result : result;
}

struct y_s_e_dist2 : gtl_yes {};

template <typename Segment1, typename Segment2>
typename enable_if<
  typename gtl_and_3<
    y_s_e_dist2,
    typename is_segment_concept<
      typename geometry_concept<Segment1>::type
    >::type,
    typename is_segment_concept<
      typename geometry_concept<Segment2>::type
    >::type
  >::type,
typename segment_distance_type<Segment1>::type>::type
euclidean_distance(const Segment1& segment1, const Segment2& segment2) {
  if (intersects(segment1, segment2))
    return 0.0;
  typename segment_distance_type<Segment1>::type
      result1 = euclidean_distance(segment1, low(segment2)),
      result2 = euclidean_distance(segment1, high(segment2)),
      result3 = euclidean_distance(segment2, low(segment1)),
      result4 = euclidean_distance(segment2, high(segment1));
  if (result2 < result1)
    result1 = result2;
  if (result4 < result3)
    result3 = result4;
  return (result1 < result3) ? result1 : result3;
}
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_SEGMENT_CONCEPT_HPP
