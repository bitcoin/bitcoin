// Boost.Polygon library interval_concept.hpp header file

// Copyright (c) Intel Corporation 2008.
// Copyright (c) 2008-2012 Simonson Lucanus.
// Copyright (c) 2012-2012 Andrii Sydorchuk.

// See http://www.boost.org for updates, documentation, and revision history.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_POLYGON_INTERVAL_CONCEPT_HPP
#define BOOST_POLYGON_INTERVAL_CONCEPT_HPP

#include "isotropy.hpp"
#include "interval_traits.hpp"

namespace boost {
namespace polygon {

struct interval_concept {};

template <typename ConceptType>
struct is_interval_concept {
  typedef gtl_no type;
};

template <>
struct is_interval_concept<interval_concept> {
  typedef gtl_yes type;
};

template <typename ConceptType>
struct is_mutable_interval_concept {
  typedef gtl_no type;
};

template <>
struct is_mutable_interval_concept<interval_concept> {
  typedef gtl_yes type;
};

template <typename GeometryType, typename BoolType>
struct interval_coordinate_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct interval_coordinate_type_by_concept<GeometryType, gtl_yes> {
  typedef typename interval_traits<GeometryType>::coordinate_type type;
};

template <typename GeometryType>
struct interval_coordinate_type {
  typedef typename interval_coordinate_type_by_concept<
    GeometryType,
    typename is_interval_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

template <typename GeometryType, typename BoolType>
struct interval_difference_type_by_concept {
  typedef void type;
};

template <typename GeometryType>
struct interval_difference_type_by_concept<GeometryType, gtl_yes> {
  typedef typename coordinate_traits<
    typename interval_traits<GeometryType>::coordinate_type
  >::coordinate_difference type;
};

template <typename GeometryType>
struct interval_difference_type {
  typedef typename interval_difference_type_by_concept<
    GeometryType,
    typename is_interval_concept<
      typename geometry_concept<GeometryType>::type
    >::type
  >::type type;
};

struct y_i_get : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_get,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_coordinate_type<IntervalType>::type
>::type get(const IntervalType& interval, direction_1d dir) {
  return interval_traits<IntervalType>::get(interval, dir);
}

struct y_i_set : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_set,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  void
>::type set(IntervalType& interval, direction_1d dir,
    typename interval_mutable_traits<IntervalType>::coordinate_type value) {
  interval_mutable_traits<IntervalType>::set(interval, dir, value);
}

struct y_i_construct : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_construct,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type construct(
    typename interval_mutable_traits<IntervalType>::coordinate_type low,
    typename interval_mutable_traits<IntervalType>::coordinate_type high) {
  if (low > high) {
    (std::swap)(low, high);
  }
  return interval_mutable_traits<IntervalType>::construct(low, high);
}

struct y_i_copy_construct : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_copy_construct,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type copy_construct(const IntervalType2& interval) {
  return construct<IntervalType1>(get(interval, LOW), get(interval, HIGH));
}

struct y_i_assign : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_assign,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& assign(IntervalType1& lvalue, const IntervalType2& rvalue) {
  set(lvalue, LOW, get(rvalue, LOW));
  set(lvalue, HIGH, get(rvalue, HIGH));
  return lvalue;
}

struct y_i_low : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_low,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_coordinate_type<IntervalType>::type
>::type low(const IntervalType& interval) {
  return get(interval, LOW);
}

struct y_i_high : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_high,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_coordinate_type<IntervalType>::type
>::type high(const IntervalType& interval) {
  return get(interval, HIGH);
}

struct y_i_low2 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_low2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  void
>::type low(IntervalType& interval,
    typename interval_mutable_traits<IntervalType>::coordinate_type value) {
  set(interval, LOW, value);
}

struct y_i_high2 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_high2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  void
>::type high(IntervalType& interval,
    typename interval_mutable_traits<IntervalType>::coordinate_type value) {
  set(interval, HIGH, value);
}

struct y_i_equivalence : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_equivalence,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type equivalence(
    const IntervalType1& interval1,
    const IntervalType2& interval2) {
  return (get(interval1, LOW) == get(interval2, LOW)) &&
         (get(interval1, HIGH) == get(interval2, HIGH));
}

struct y_i_contains : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_contains,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  bool
>::type contains(
    const IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type value,
    bool consider_touch = true ) {
  if (consider_touch) {
    return value <= high(interval) && value >= low(interval);
  } else {
    return value < high(interval) && value > low(interval);
  }
}

struct y_i_contains2 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_contains2,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type contains(
    const IntervalType1& interval1,
    const IntervalType2& interval2,
    bool consider_touch = true) {
  return contains(interval1, get(interval2, LOW), consider_touch) &&
         contains(interval1, get(interval2, HIGH), consider_touch);
}

struct y_i_center : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_center,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_coordinate_type<IntervalType>::type
>::type center(const IntervalType& interval) {
  return (high(interval) + low(interval)) / 2;
}

struct y_i_delta : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_delta,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_difference_type<IntervalType>::type
>::type delta(const IntervalType& interval) {
  typedef typename interval_difference_type<IntervalType>::type diff_type;
  return static_cast<diff_type>(high(interval)) -
         static_cast<diff_type>(low(interval));
}

struct y_i_flip : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_flip,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
IntervalType>::type& flip(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type axis = 0) {
  typename interval_coordinate_type<IntervalType>::type newLow, newHigh;
  newLow  = 2 * axis - high(interval);
  newHigh = 2 * axis - low(interval);
  low(interval, newLow);
  high(interval, newHigh);
  return interval;
}

struct y_i_scale_up : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_scale_up,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& scale_up(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type factor) {
  typename interval_coordinate_type<IntervalType>::type newHigh =
      high(interval) * factor;
  low(interval, low(interval) * factor);
  high(interval, (newHigh));
  return interval;
}

struct y_i_scale_down : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_scale_down,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& scale_down(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type factor) {
  typename interval_coordinate_type<IntervalType>::type newHigh =
      high(interval) / factor;
  low(interval, low(interval) / factor);
  high(interval, (newHigh));
  return interval;
}

// TODO(asydorchuk): Deprecated.
struct y_i_scale : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_scale,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& scale(IntervalType& interval, double factor) {
  typedef typename interval_coordinate_type<IntervalType>::type Unit;
  Unit newHigh = scaling_policy<Unit>::round(
      static_cast<double>(high(interval)) * factor);
  low(interval, scaling_policy<Unit>::round(
      static_cast<double>(low(interval)) * factor));
  high(interval, (newHigh));
  return interval;
}

struct y_i_move : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_move,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& move(
    IntervalType& interval,
    typename interval_difference_type<IntervalType>::type displacement) {
  typedef typename interval_coordinate_type<IntervalType>::type ctype;
  typedef typename coordinate_traits<ctype>::coordinate_difference Unit;
  low(interval, static_cast<ctype>(
      static_cast<Unit>(low(interval)) + displacement));
  high(interval, static_cast<ctype>(
      static_cast<Unit>(high(interval)) + displacement));
  return interval;
}

struct y_i_convolve : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_convolve,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& convolve(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type value) {
    typedef typename interval_coordinate_type<IntervalType>::type Unit;
  Unit newLow  = low(interval) + value;
  Unit newHigh = high(interval) + value;
  low(interval, newLow);
  high(interval, newHigh);
  return interval;
}

struct y_i_deconvolve : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_deconvolve,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& deconvolve(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type value) {
    typedef typename interval_coordinate_type<IntervalType>::type Unit;
  Unit newLow  = low(interval) - value;
  Unit newHigh = high(interval) - value;
  low(interval, newLow);
  high(interval, newHigh);
  return interval;
}

struct y_i_convolve2 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_convolve2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& convolve(IntervalType1& lvalue, const IntervalType2& rvalue) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit newLow  = low(lvalue) + low(rvalue);
  Unit newHigh = high(lvalue) + high(rvalue);
  low(lvalue, newLow);
  high(lvalue, newHigh);
  return lvalue;
}

struct y_i_deconvolve2 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_deconvolve2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& deconvolve(IntervalType1& lvalue, const IntervalType2& rvalue) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit newLow  = low(lvalue) - low(rvalue);
  Unit newHigh = high(lvalue) - high(rvalue);
  low(lvalue, newLow);
  high(lvalue, newHigh);
  return lvalue;
}

struct y_i_reconvolve : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_reconvolve,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& reflected_convolve(
    IntervalType1& lvalue,
    const IntervalType2& rvalue) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit newLow  = low(lvalue) - high(rvalue);
  Unit newHigh = high(lvalue) - low(rvalue);
  low(lvalue, newLow);
  high(lvalue, newHigh);
  return lvalue;
}

struct y_i_redeconvolve : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_redeconvolve,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& reflected_deconvolve(
    IntervalType1& lvalue,
    const IntervalType2& rvalue) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit newLow  = low(lvalue)  + high(rvalue);
  Unit newHigh = high(lvalue) + low(rvalue);
  low(lvalue, newLow);
  high(lvalue, newHigh);
  return lvalue;
}

struct y_i_e_dist1 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<y_i_e_dist1,
    typename is_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  typename interval_difference_type<IntervalType>::type
>::type euclidean_distance(
    const IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type position) {
  typedef typename interval_difference_type<IntervalType>::type Unit;
  Unit dist[3] = {
      0,
      (Unit)low(interval) - (Unit)position,
      (Unit)position - (Unit)high(interval)
  };
  return dist[(dist[1] > 0) + ((dist[2] > 0) << 1)];
}

struct y_i_e_dist2 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_e_dist2,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  typename interval_difference_type<IntervalType1>::type
>::type euclidean_distance(
    const IntervalType1& interval1,
    const IntervalType2& interval2) {
  typedef typename interval_difference_type<IntervalType1>::type Unit;
  Unit dist[3] = {
      0,
      (Unit)low(interval1) - (Unit)high(interval2),
      (Unit)low(interval2) - (Unit)high(interval1)
  };
  return dist[(dist[1] > 0) + ((dist[2] > 0) << 1)];
}

struct y_i_e_intersects : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_e_intersects,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type intersects(
    const IntervalType1& interval1,
    const IntervalType2& interval2,
    bool consider_touch = true) {
  return consider_touch ?
      (low(interval1) <= high(interval2)) &&
      (high(interval1) >= low(interval2)) :
      (low(interval1) < high(interval2)) &&
      (high(interval1) > low(interval2));
}

struct y_i_e_bintersect : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_e_bintersect,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type boundaries_intersect(
    const IntervalType1& interval1,
    const IntervalType2& interval2,
    bool consider_touch = true) {
  return (contains(interval1, low(interval2), consider_touch) ||
          contains(interval1, high(interval2), consider_touch)) &&
         (contains(interval2, low(interval1), consider_touch) ||
          contains(interval2, high(interval1), consider_touch));
}

struct y_i_intersect : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_intersect,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type intersect(
    IntervalType1& lvalue,
    const IntervalType2& rvalue,
    bool consider_touch = true) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit lowVal = (std::max)(low(lvalue), low(rvalue));
  Unit highVal = (std::min)(high(lvalue), high(rvalue));
  bool valid = consider_touch ? lowVal <= highVal : lowVal < highVal;
  if (valid) {
    low(lvalue, lowVal);
    high(lvalue, highVal);
  }
  return valid;
}

struct y_i_g_intersect : gtl_yes {};

// TODO(asydorchuk): Deprecated.
template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_g_intersect,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  IntervalType1
>::type& generalized_intersect(
    IntervalType1& lvalue,
    const IntervalType2& rvalue) {
  typedef typename interval_coordinate_type<IntervalType1>::type Unit;
  Unit coords[4] = {low(lvalue), high(lvalue), low(rvalue), high(rvalue)};
  // TODO(asydorchuk): consider implementing faster sorting of small
  // fixed length range.
  polygon_sort(coords, coords+4);
  low(lvalue, coords[1]);
  high(lvalue, coords[2]);
  return lvalue;
}

struct y_i_abuts1 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_abuts1,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type abuts(
    const IntervalType1& interval1,
    const IntervalType2& interval2,
    direction_1d dir) {
  return dir.to_int() ? low(interval2) == high(interval1) :
                        low(interval1) == high(interval2);
}

struct y_i_abuts2 : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_abuts2,
    typename is_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type abuts(
    const IntervalType1& interval1,
    const IntervalType2& interval2) {
  return abuts(interval1, interval2, HIGH) ||
         abuts(interval1, interval2, LOW);
}

struct y_i_bloat : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_bloat,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& bloat(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type bloating) {
  low(interval, low(interval) - bloating);
  high(interval, high(interval) + bloating);
  return interval;
}

struct y_i_bloat2 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_bloat2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& bloat(
    IntervalType& interval,
    direction_1d dir,
    typename interval_coordinate_type<IntervalType>::type bloating) {
  set(interval, dir, get(interval, dir) + dir.get_sign() * bloating);
  return interval;
}

struct y_i_shrink : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_shrink,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& shrink(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type shrinking) {
  return bloat(interval, -shrinking);
}

struct y_i_shrink2 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_shrink2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type& shrink(
    IntervalType& interval,
    direction_1d dir,
    typename interval_coordinate_type<IntervalType>::type shrinking) {
  return bloat(interval, dir, -shrinking);
}

struct y_i_encompass : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_encompass,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type
  >::type,
  bool
>::type encompass(IntervalType1& interval1, const IntervalType2& interval2) {
  bool retval = !contains(interval1, interval2, true);
  low(interval1, (std::min)(low(interval1), low(interval2)));
  high(interval1, (std::max)(high(interval1), high(interval2)));
  return retval;
}

struct y_i_encompass2 : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_encompass2,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  bool
>::type encompass(
    IntervalType& interval,
    typename interval_coordinate_type<IntervalType>::type value) {
  bool retval = !contains(interval, value, true);
  low(interval, (std::min)(low(interval), value));
  high(interval, (std::max)(high(interval), value));
  return retval;
}

struct y_i_get_half : gtl_yes {};

template <typename IntervalType>
typename enable_if<
  typename gtl_and<
    y_i_get_half,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType>::type
    >::type
  >::type,
  IntervalType
>::type get_half(const IntervalType& interval, direction_1d dir) {
  typedef typename interval_coordinate_type<IntervalType>::type Unit;
  Unit c = (get(interval, LOW) + get(interval, HIGH)) / 2;
  return construct<IntervalType>(
      (dir == LOW) ? get(interval, LOW) : c,
      (dir == LOW) ? c : get(interval, HIGH));
}

struct y_i_join_with : gtl_yes {};

template <typename IntervalType1, typename IntervalType2>
typename enable_if<
  typename gtl_and_3<
    y_i_join_with,
    typename is_mutable_interval_concept<
      typename geometry_concept<IntervalType1>::type
    >::type,
    typename is_interval_concept<
      typename geometry_concept<IntervalType2>::type
    >::type>::type,
  bool
>::type join_with(IntervalType1& interval1, const IntervalType2& interval2) {
  if (abuts(interval1, interval2)) {
    encompass(interval1, interval2);
    return true;
  }
  return false;
}
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_INTERVAL_CONCEPT_HPP
