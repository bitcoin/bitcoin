/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_RECTANGLE_CONCEPT_HPP
#define BOOST_POLYGON_RECTANGLE_CONCEPT_HPP

#include "isotropy.hpp"

//point
#include "point_data.hpp"
#include "point_traits.hpp"
#include "point_concept.hpp"

//interval
#include "interval_data.hpp"
#include "interval_traits.hpp"
#include "interval_concept.hpp"

#include "rectangle_data.hpp"
#include "rectangle_traits.hpp"

namespace boost { namespace polygon{
  struct rectangle_concept {};

  template <typename T>
  struct is_rectangle_concept { typedef gtl_no type; };
  template <>
  struct is_rectangle_concept<rectangle_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_mutable_rectangle_concept { typedef gtl_no type; };
  template <>
  struct is_mutable_rectangle_concept<rectangle_concept> { typedef gtl_yes type; };

  template <>
  struct geometry_domain<rectangle_concept> { typedef manhattan_domain type; };

  template <typename T, typename CT>
  struct rectangle_interval_type_by_concept { typedef void type; };
  template <typename T>
  struct rectangle_interval_type_by_concept<T, gtl_yes> { typedef typename rectangle_traits<T>::interval_type type; };

  template <typename T>
  struct rectangle_interval_type {
      typedef typename rectangle_interval_type_by_concept<T, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct rectangle_coordinate_type_by_concept { typedef void type; };
  template <typename T>
  struct rectangle_coordinate_type_by_concept<T, gtl_yes> { typedef typename rectangle_traits<T>::coordinate_type type; };

  template <typename T>
  struct rectangle_coordinate_type {
      typedef typename rectangle_coordinate_type_by_concept<T, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct rectangle_difference_type_by_concept { typedef void type; };
  template <typename T>
  struct rectangle_difference_type_by_concept<T, gtl_yes> {
     typedef typename coordinate_traits<typename rectangle_traits<T>::coordinate_type>::coordinate_difference type; };

  template <typename T>
  struct rectangle_difference_type {
    typedef typename rectangle_difference_type_by_concept<
      T, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct rectangle_distance_type_by_concept { typedef void type; };
  template <typename T>
  struct rectangle_distance_type_by_concept<T, gtl_yes> {
    typedef typename coordinate_traits<typename rectangle_coordinate_type<T>::type>::coordinate_distance type; };

  template <typename T>
  struct rectangle_distance_type {
    typedef typename rectangle_distance_type_by_concept<
      T, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  struct y_r_get_interval : gtl_yes {};

  template <typename T>
  typename enable_if< typename gtl_and<y_r_get_interval, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type,
                      typename rectangle_interval_type<T>::type>::type
  get(const T& rectangle, orientation_2d orient) {
    return rectangle_traits<T>::get(rectangle, orient);
  }

  struct y_r_h : gtl_yes {};

  template <typename T>
  typename enable_if< typename gtl_and<y_r_h, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type,
                       typename rectangle_interval_type<T>::type>::type
  horizontal(const T& rectangle) {
    return rectangle_traits<T>::get(rectangle, HORIZONTAL);
  }

  struct y_r_v : gtl_yes {};

  template <typename T>
  typename enable_if< typename gtl_and<y_r_v, typename is_rectangle_concept<typename geometry_concept<T>::type>::type>::type,
                       typename rectangle_interval_type<T>::type>::type
  vertical(const T& rectangle) {
    return rectangle_traits<T>::get(rectangle, VERTICAL);
  }

  struct y_r_set : gtl_yes {};

  template <orientation_2d_enum orient, typename T, typename T2>
  typename enable_if< typename gtl_and_3<y_r_set, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type,
                                        typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type,
                       void>::type
  set(T& rectangle, const T2& interval) {
    rectangle_mutable_traits<T>::set(rectangle, orient, interval);
  }

  struct y_r_set2 : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if< typename gtl_and_3<y_r_set2, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type,
                                        typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type,
                       void>::type
  set(T& rectangle, orientation_2d orient, const T2& interval) {
    rectangle_mutable_traits<T>::set(rectangle, orient, interval);
  }

  struct y_r_h2 : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if< typename gtl_and_3<y_r_h2, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type,
                                        typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type,
                       void>::type
  horizontal(T& rectangle, const T2& interval) {
    rectangle_mutable_traits<T>::set(rectangle, HORIZONTAL, interval);
  }

  struct y_r_v2 : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if<
    typename gtl_and_3<y_r_v2, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type,
                     typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type, void>::type
  vertical(T& rectangle, const T2& interval) {
    rectangle_mutable_traits<T>::set(rectangle, VERTICAL, interval);
  }

  struct y_r_construct : gtl_yes {};

  template <typename T, typename T2, typename T3>
  typename enable_if< typename gtl_and<y_r_construct, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type>::type,
                       T>::type
  construct(const T2& interval_horizontal,
            const T3& interval_vertical) {
    return rectangle_mutable_traits<T>::construct(interval_horizontal, interval_vertical); }

  struct y_r_construct2 : gtl_yes {};

  template <typename T, typename coord_type>
  typename enable_if< typename gtl_and<y_r_construct2, typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type>::type,
                       T>::type
  construct(coord_type xl, coord_type yl, coord_type xh, coord_type yh) {
    return rectangle_mutable_traits<T>::construct(interval_data<coord_type>(xl, xh),
                                                  interval_data<coord_type>(yl, yh));
  }

  struct y_r_cconstruct : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if<
    typename gtl_and_3<y_r_cconstruct,
      typename is_mutable_rectangle_concept<typename geometry_concept<T>::type>::type,
      typename is_rectangle_concept<typename geometry_concept<T2>::type>::type>::type,
    T>::type
  copy_construct(const T2& rectangle) {
    return construct<T> (get(rectangle, HORIZONTAL), get(rectangle, VERTICAL));
  }

  struct y_r_assign : gtl_yes {};

  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3< y_r_assign,
      typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    rectangle_type_1>::type &
  assign(rectangle_type_1& lvalue, const rectangle_type_2& rvalue) {
    set(lvalue, HORIZONTAL, get(rvalue, HORIZONTAL));
    set(lvalue, VERTICAL, get(rvalue, VERTICAL));
    return lvalue;
  }

  struct y_r_equiv : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if<
    typename gtl_and_3< y_r_equiv,
      typename is_rectangle_concept<typename geometry_concept<T>::type>::type,
      typename is_rectangle_concept<typename geometry_concept<T2>::type>::type>::type,
    bool>::type
  equivalence(const T& rect1, const T2& rect2) {
    return equivalence(get(rect1, HORIZONTAL), get(rect2, HORIZONTAL)) &&
      equivalence(get(rect1, VERTICAL), get(rect2, VERTICAL));
  }

  struct y_r_get : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_get, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_coordinate_type<rectangle_type>::type>::type
  get(const rectangle_type& rectangle, orientation_2d orient, direction_1d dir) {
    return get(rectangle_traits<rectangle_type>::get(rectangle, orient), dir);
  }

  struct y_r_set3 : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_set3, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type, void>::type
  set(rectangle_type& rectangle, orientation_2d orient, direction_1d dir,
      typename rectangle_coordinate_type<rectangle_type>::type value) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orient);
    set(ivl, dir, value);
    set(rectangle, orient, ivl);
  }

  struct y_r_xl : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_xl, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_coordinate_type<rectangle_type>::type>::type
  xl(const rectangle_type& rectangle) {
    return get(rectangle, HORIZONTAL, LOW);
  }

  struct y_r_xl2 : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_xl2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type, void>::type
      xl(rectangle_type& rectangle, typename rectangle_coordinate_type<rectangle_type>::type value) {
    return set(rectangle, HORIZONTAL, LOW, value);
  }

  struct y_r_xh : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_xh, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_coordinate_type<rectangle_type>::type>::type
  xh(const rectangle_type& rectangle) {
    return get(rectangle, HORIZONTAL, HIGH);
  }

  struct y_r_xh2 : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_xh2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type, void>::type
  xh(rectangle_type& rectangle, typename rectangle_coordinate_type<rectangle_type>::type value) {
    return set(rectangle, HORIZONTAL, HIGH, value);
  }

  struct y_r_yl : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_yl, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_coordinate_type<rectangle_type>::type>::type
  yl(const rectangle_type& rectangle) {
    return get(rectangle, VERTICAL, LOW);
  }

  struct y_r_yl2 : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_yl2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type, void>::type
      yl(rectangle_type& rectangle, typename rectangle_coordinate_type<rectangle_type>::type value) {
    return set(rectangle, VERTICAL, LOW, value);
  }

  struct y_r_yh : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_yh, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_coordinate_type<rectangle_type>::type>::type
  yh(const rectangle_type& rectangle) {
    return get(rectangle, VERTICAL, HIGH);
  }

  struct y_r_yh2 : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_yh2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type, void>::type
      yh(rectangle_type& rectangle, typename rectangle_coordinate_type<rectangle_type>::type value) {
    return set(rectangle, VERTICAL, HIGH, value);
  }

  struct y_r_ll : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_ll,  typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       point_data<typename rectangle_coordinate_type<rectangle_type>::type> >::type
  ll(const rectangle_type& rectangle) {
    return point_data<typename rectangle_coordinate_type<rectangle_type>::type> (xl(rectangle), yl(rectangle));
  }

  struct y_r_lr : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_lr,  typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       point_data<typename rectangle_coordinate_type<rectangle_type>::type> >::type
  lr(const rectangle_type& rectangle) {
    return point_data<typename rectangle_coordinate_type<rectangle_type>::type> (xh(rectangle), yl(rectangle));
  }

  struct y_r_ul : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_ul,  typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       point_data<typename rectangle_coordinate_type<rectangle_type>::type> >::type
  ul(const rectangle_type& rectangle) {
    return point_data<typename rectangle_coordinate_type<rectangle_type>::type> (xl(rectangle), yh(rectangle));
  }

  struct y_r_ur : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_ur,  typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       point_data<typename rectangle_coordinate_type<rectangle_type>::type> >::type
  ur(const rectangle_type& rectangle) {
    return point_data<typename rectangle_coordinate_type<rectangle_type>::type> (xh(rectangle), yh(rectangle));
  }

  struct y_r_contains : gtl_yes {};

  template <typename rectangle_type, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_contains, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  contains(const rectangle_type& rectangle, const rectangle_type_2 rectangle_contained,
           bool consider_touch = true) {
    return contains(horizontal(rectangle), horizontal(rectangle_contained), consider_touch) &&
      contains(vertical(rectangle), vertical(rectangle_contained), consider_touch);
  }

  struct y_r_contains2 : gtl_yes {};

  template <typename rectangle_type, typename point_type>
  typename enable_if< typename gtl_and_3<y_r_contains2, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                         typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type, bool>::type
  contains(const rectangle_type& rectangle, const point_type point_contained,
           bool consider_touch = true) {
    return contains(horizontal(rectangle), x(point_contained), consider_touch) &&
      contains(vertical(rectangle), y(point_contained), consider_touch);
  }

  struct y_r_set_points : gtl_yes {};

  // set all four coordinates based upon two points
  template <typename rectangle_type, typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_4< y_r_set_points,
    typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
    typename is_point_concept<typename geometry_concept<point_type_1>::type>::type,
    typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
                       rectangle_type>::type &
  set_points(rectangle_type& rectangle, const point_type_1& p1,
             const point_type_2& p2) {
    typedef typename rectangle_coordinate_type<rectangle_type>::type Unit;
    Unit x1(x(p1));
    Unit x2(x(p2));
    Unit y1(y(p1));
    Unit y2(y(p2));
    horizontal(rectangle, construct<typename rectangle_interval_type<rectangle_type>::type>(x1, x2));
    vertical(rectangle, construct<typename rectangle_interval_type<rectangle_type>::type>(y1, y2));
    return rectangle;
  }

  struct y_r_move : gtl_yes {};

  // move rectangle by delta in orient
  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_move, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  move(rectangle_type& rectangle, orientation_2d orient,
       typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::coordinate_difference delta) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orient);
    move(ivl, delta);
    set(rectangle, orient, ivl);
    return rectangle;
  }

  struct y_r_convolve : gtl_yes {};

  // convolve this with b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3< y_r_convolve,
      typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    rectangle_type_1>::type &
  convolve(rectangle_type_1& rectangle,
           const rectangle_type_2& convolution_rectangle) {
    typename rectangle_interval_type<rectangle_type_1>::type ivl = horizontal(rectangle);
    horizontal(rectangle, convolve(ivl, horizontal(convolution_rectangle)));
    ivl = vertical(rectangle);
    vertical(rectangle, convolve(ivl, vertical(convolution_rectangle)));
    return rectangle;
  }

  struct y_r_deconvolve : gtl_yes {};

  // deconvolve this with b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3< y_r_deconvolve,
    typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
    typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       rectangle_type_1>::type &
  deconvolve(rectangle_type_1& rectangle, const rectangle_type_2& convolution_rectangle) {
    typename rectangle_interval_type<rectangle_type_1>::type ivl = horizontal(rectangle);
    horizontal(rectangle, deconvolve(ivl, horizontal(convolution_rectangle)));
    ivl = vertical(rectangle);
    vertical(rectangle, deconvolve(ivl, vertical(convolution_rectangle)));
    return rectangle;
  }

  struct y_r_reconvolve : gtl_yes {};

  // reflectedConvolve this with b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_reconvolve, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    rectangle_type_1>::type &
  reflected_convolve(rectangle_type_1& rectangle, const rectangle_type_2& convolution_rectangle) {
    typename rectangle_interval_type<rectangle_type_1>::type ivl = horizontal(rectangle);
    horizontal(rectangle, reflected_convolve(ivl, horizontal(convolution_rectangle)));
    ivl = vertical(rectangle);
    vertical(rectangle, reflected_convolve(ivl, vertical(convolution_rectangle)));
    return rectangle;
  }

  struct y_r_redeconvolve : gtl_yes {};

  // reflectedDeconvolve this with b
  // deconvolve this with b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_redeconvolve, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    rectangle_type_1>::type &
  reflected_deconvolve(rectangle_type_1& rectangle, const rectangle_type_2& convolution_rectangle) {
    typename rectangle_interval_type<rectangle_type_1>::type ivl = horizontal(rectangle);
    horizontal(rectangle, reflected_deconvolve(ivl, horizontal(convolution_rectangle)));
    ivl = vertical(rectangle);
    vertical(rectangle, reflected_deconvolve(ivl, vertical(convolution_rectangle)));
    return rectangle;
  }

  struct y_r_convolve2 : gtl_yes {};

  // convolve with point
  template <typename rectangle_type, typename point_type>
  typename enable_if< typename gtl_and_3<y_r_convolve2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                         typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                       rectangle_type>::type &
  convolve(rectangle_type& rectangle, const point_type& convolution_point) {
    typename rectangle_interval_type<rectangle_type>::type ivl = horizontal(rectangle);
    horizontal(rectangle, convolve(ivl, x(convolution_point)));
    ivl = vertical(rectangle);
    vertical(rectangle, convolve(ivl, y(convolution_point)));
    return rectangle;
  }

  struct y_r_deconvolve2 : gtl_yes {};

  // deconvolve with point
  template <typename rectangle_type, typename point_type>
  typename enable_if<
    typename gtl_and_3<y_r_deconvolve2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                      typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type, rectangle_type>::type &
  deconvolve(rectangle_type& rectangle, const point_type& convolution_point) {
    typename rectangle_interval_type<rectangle_type>::type ivl = horizontal(rectangle);
    horizontal(rectangle, deconvolve(ivl, x(convolution_point)));
    ivl = vertical(rectangle);
    vertical(rectangle, deconvolve(ivl, y(convolution_point)));
    return rectangle;
  }

  struct y_r_delta : gtl_yes {};

  // get the magnitude of the interval range depending on orient
  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_delta, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_difference_type<rectangle_type>::type>::type
  delta(const rectangle_type& rectangle, orientation_2d orient) {
    return delta(get(rectangle, orient));
  }

  struct y_r_area : gtl_yes {};

  // get the area of the rectangle
  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_area, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::manhattan_area_type>::type
  area(const rectangle_type& rectangle) {
    typedef typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::manhattan_area_type area_type;
    return (area_type)delta(rectangle, HORIZONTAL) * (area_type)delta(rectangle, VERTICAL);
  }

  struct y_r_go : gtl_yes {};

  // returns the orientation of the longest side
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_go, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      orientation_2d>::type
  guess_orientation(const rectangle_type& rectangle) {
    return delta(rectangle, HORIZONTAL) >= delta(rectangle, VERTICAL) ?
      HORIZONTAL : VERTICAL;
  }

  struct y_r_half_p : gtl_yes {};

  // get the half perimeter of the rectangle
  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_half_p, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                       typename rectangle_difference_type<rectangle_type>::type>::type
  half_perimeter(const rectangle_type& rectangle) {
    return delta(rectangle, HORIZONTAL) + delta(rectangle, VERTICAL);
  }

  struct y_r_perimeter : gtl_yes {};

  // get the perimeter of the rectangle
  template <typename rectangle_type>
  typename enable_if< typename gtl_and<y_r_perimeter, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      typename rectangle_difference_type<rectangle_type>::type>::type
  perimeter(const rectangle_type& rectangle) {
    return 2 * half_perimeter(rectangle);
  }

  struct y_r_intersects : gtl_yes {};

  // check if Rectangle b intersects `this` Rectangle
  //  [in]     b         Rectangle that will be checked
  //  [in]     considerTouch If true, return true even if b touches the boundary
  //  [ret]    .         true if `t` intersects b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_intersects, typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    bool>::type
  intersects(const rectangle_type_1& rectangle, const rectangle_type_2& b, bool consider_touch = true) {
    return intersects(horizontal(rectangle), horizontal(b), consider_touch) &&
      intersects(vertical(rectangle), vertical(b), consider_touch);
  }

  struct y_r_b_intersect : gtl_yes {};

  // Check if boundaries of Rectangle b and `this` Rectangle intersect
  //  [in]     b         Rectangle that will be checked
  //  [in]     considerTouch If true, return true even if p is on the foundary
  //  [ret]    .         true if `t` contains p
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_b_intersect, typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    bool>::type
  boundaries_intersect(const rectangle_type_1& rectangle, const rectangle_type_2& b,
                       bool consider_touch = true) {
    return (intersects(rectangle, b, consider_touch) &&
            !(contains(rectangle, b, !consider_touch)) &&
            !(contains(b, rectangle, !consider_touch)));
  }

  struct y_r_b_abuts : gtl_yes {};

  // check if b is touching 'this' on the end specified by dir
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_b_abuts, typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  abuts(const rectangle_type_1& rectangle, const rectangle_type_2& b,
        direction_2d dir) {
    return
      abuts(get(rectangle, orientation_2d(dir)),
            get(b, orientation_2d(dir)),
            direction_1d(dir)) &&
      intersects(get(rectangle, orientation_2d(dir).get_perpendicular()),
                 get(b, orientation_2d(dir).get_perpendicular()), true);
  }

  struct y_r_b_abuts2 : gtl_yes {};

  // check if they are touching in the given orientation
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_b_abuts2, typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  abuts(const rectangle_type_1& rectangle, const rectangle_type_2& b,
        orientation_2d orient) {
    return
      abuts(get(rectangle, orient), get(b, orient)) &&
      intersects(get(rectangle, orient.get_perpendicular()),
                 get(b, orient.get_perpendicular()), true);
  }

  struct y_r_b_abuts3 : gtl_yes {};

  // check if they are touching but not overlapping
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_b_abuts3, typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  abuts(const rectangle_type_1& rectangle, const rectangle_type_2& b) {
    return abuts(rectangle, b, HORIZONTAL) || abuts(rectangle, b, VERTICAL);
  }

  struct y_r_b_intersect2 : gtl_yes {};

  // intersect rectangle with interval on orient
  template <typename rectangle_type, typename interval_type>
  typename enable_if<
    typename gtl_and_3<y_r_b_intersect2, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                      typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
    bool>::type
  intersect(rectangle_type& rectangle, const interval_type& b,
            orientation_2d orient, bool consider_touch = true) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orient);
    if(intersect(ivl, b, consider_touch)) {
      set(rectangle, orient, ivl);
      return true;
    }
    return false;
  }

  struct y_r_b_intersect3 : gtl_yes {};

  // clip rectangle to b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_b_intersect3, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  intersect(rectangle_type_1& rectangle, const rectangle_type_2& b, bool consider_touch = true) {
    if(intersects(rectangle, b)) {
      intersect(rectangle, horizontal(b), HORIZONTAL, consider_touch);
      intersect(rectangle, vertical(b), VERTICAL, consider_touch);
      return true;
    }
    return false;
  }

  struct y_r_g_intersect : gtl_yes {};

  // Sets this to the generalized intersection of this and the given rectangle
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_g_intersect,
    typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
    typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       rectangle_type_1>::type &
  generalized_intersect(rectangle_type_1& rectangle, const rectangle_type_2& b) {
    typename rectangle_interval_type<rectangle_type_1>::type ivl = get(rectangle, HORIZONTAL);
    generalized_intersect(ivl, horizontal(b));
    horizontal(rectangle, ivl);
    ivl = vertical(rectangle);
    generalized_intersect(ivl, vertical(b));
    vertical(rectangle, ivl);
    return rectangle;
  }

  struct y_r_bloat : gtl_yes {};

  // bloat the interval specified by orient by bloating
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_bloat, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  bloat(rectangle_type& rectangle, orientation_2d orient,
        typename rectangle_coordinate_type<rectangle_type>::type bloating) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orient);
    bloat(ivl, bloating);
    set(rectangle, orient, ivl);
    return rectangle;
  }

  struct y_r_bloat2 : gtl_yes {};

  // bloat the Rectangle by bloating
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_bloat2, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  bloat(rectangle_type& rectangle,
        typename rectangle_coordinate_type<rectangle_type>::type bloating) {
    bloat(rectangle, HORIZONTAL, bloating);
    return bloat(rectangle, VERTICAL, bloating);
  }

  struct y_r_bloat3 : gtl_yes {};

  // bloat the interval cooresponding to orient by bloating in dir direction
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_bloat3, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  bloat(rectangle_type& rectangle, direction_2d dir,
        typename rectangle_coordinate_type<rectangle_type>::type bloating) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orientation_2d(dir));
    bloat(ivl, direction_1d(dir), bloating);
    set(rectangle, orientation_2d(dir), ivl);
    return rectangle;
  }

  struct y_r_shrink : gtl_yes {};

  // shrink the interval specified by orient by bloating
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_shrink, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  shrink(rectangle_type& rectangle, orientation_2d orient,
         typename rectangle_coordinate_type<rectangle_type>::type shrinking) {
    return bloat(rectangle, orient, -shrinking);
  }

  struct y_r_shrink2 : gtl_yes {};

  // shrink the Rectangle by bloating
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_shrink2, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  shrink(rectangle_type& rectangle,
         typename rectangle_coordinate_type<rectangle_type>::type shrinking) {
    return bloat(rectangle, -shrinking);
  }

  struct y_r_shrink3 : gtl_yes {};

  // shrink the interval cooresponding to orient by bloating in dir direction
  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_shrink3, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                      rectangle_type>::type &
  shrink(rectangle_type& rectangle, direction_2d dir,
         typename rectangle_coordinate_type<rectangle_type>::type shrinking) {
    return bloat(rectangle, dir, -shrinking);
  }

  struct y_r_encompass : gtl_yes {};

  // encompass interval on orient
  template <typename rectangle_type, typename interval_type>
  typename enable_if<typename gtl_and_3<
        y_r_encompass,
        typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
        typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
      bool>::type
  encompass(rectangle_type& rectangle, const interval_type& b, orientation_2d orient) {
    typename rectangle_interval_type<rectangle_type>::type ivl = get(rectangle, orient);
    if(encompass(ivl, b)) {
      set(rectangle, orient, ivl);
      return true;
    }
    return false;
  }

 struct y_r_encompass2 : gtl_yes {};

  // enlarge rectangle to encompass the Rectangle b
  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<
        y_r_encompass2,
        typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
        typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type >::type,
      bool>::type
  encompass(rectangle_type_1& rectangle, const rectangle_type_2& b) {
    //note that operator | is intentional because both should be called regardless
    return encompass(rectangle, horizontal(b), HORIZONTAL) |
      encompass(rectangle, vertical(b), VERTICAL);
  }

  struct y_r_encompass3 : gtl_yes {};

  // enlarge rectangle to encompass the point b
  template <typename rectangle_type_1, typename point_type>
  typename enable_if<typename gtl_and_3<
        y_r_encompass3,
        typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
        typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
      bool>::type
  encompass(rectangle_type_1& rectangle, const point_type& b) {
    typename rectangle_interval_type<rectangle_type_1>::type hivl, vivl;
    hivl = horizontal(rectangle);
    vivl = vertical(rectangle);
    //note that operator | is intentional because both should be called regardless
    bool retval = encompass(hivl, x(b)) | encompass(vivl, y(b));
    if(retval) {
      horizontal(rectangle, hivl);
      vertical(rectangle, vivl);
    }
    return retval;
  }

  struct y_r_center : gtl_yes {};

  // returns the center of the rectangle
  template <typename point_type, typename rectangle_type>
  typename enable_if<
    typename gtl_and_3<y_r_center, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
    bool>::type
  center(point_type& center_point, const rectangle_type& rectangle) {
    center_point = construct<point_type>(center(horizontal(rectangle)),
                                         center(vertical(rectangle)));
    return true;
  }

  struct y_r_get_corner : gtl_yes {};

  template <typename point_type, typename rectangle_type>
  typename enable_if<
    typename gtl_and_3<y_r_get_corner, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type,
                      typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
    bool>::type
  get_corner(point_type& corner_point, const rectangle_type& rectangle, direction_2d direction_facing, direction_1d direction_turning) {
    typedef typename rectangle_coordinate_type<rectangle_type>::type Unit;
    Unit u1 = get(rectangle, direction_facing);
    Unit u2 = get(rectangle, direction_facing.turn(direction_turning));
    if(orientation_2d(direction_facing).to_int()) std::swap(u1, u2);
    corner_point = construct<point_type>(u1, u2);
    return true;
  }

  struct y_r_get_half : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_get_half, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                     rectangle_type>::type
  get_half(const rectangle_type& rectangle, direction_2d dir) {
    rectangle_type retval(rectangle);
    set(retval, orientation_2d(dir), get_half(get(rectangle, orientation_2d(dir)), direction_1d(dir)));
    return retval;
  }

  struct y_r_join_with : gtl_yes {};

  template <typename rectangle_type_1, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_join_with, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       bool>::type
  join_with(rectangle_type_1& rectangle, const rectangle_type_2& b) {
    typedef typename rectangle_interval_type<rectangle_type_1>::type Interval1;
    typedef typename rectangle_interval_type<rectangle_type_2>::type Interval2;
    Interval1 hi1 = get(rectangle, HORIZONTAL);
    Interval1 vi1 = get(rectangle, VERTICAL);
    Interval2 hi2 = get(b, HORIZONTAL), vi2 = get(b, VERTICAL);
    Interval1 temp;
    if (equivalence(hi1, hi2) && join_with(vi1, vi2)) {
      vertical(rectangle, vi1);
      return true;
    }
    if (equivalence(vi1, vi2) && join_with(hi1, hi2)) {
      horizontal(rectangle, hi1);
      return true;
    }
    return false;
  }

  struct y_r_eda2 : gtl_yes {};

  template <typename rectangle_type, typename point_type>
  typename enable_if< typename gtl_and_3<y_r_eda2,
    typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
    typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      typename rectangle_difference_type<rectangle_type>::type>::type
  euclidean_distance(const rectangle_type& lvalue, const point_type& rvalue, orientation_2d orient) {
    return euclidean_distance(get(lvalue, orient), get(rvalue, orient));
  }

  struct y_r_eda : gtl_yes {};

  template <typename rectangle_type, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_eda,
      typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                       typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    typename rectangle_difference_type<rectangle_type>::type>::type
  euclidean_distance(const rectangle_type& lvalue, const rectangle_type_2& rvalue, orientation_2d orient) {
    return euclidean_distance(get(lvalue, orient), get(rvalue, orient));
  }

  struct y_r_sed : gtl_yes {};

  template <typename rectangle_type, typename point_type>
  typename enable_if< typename gtl_and_3<y_r_sed,
    typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
    typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                       typename rectangle_difference_type<rectangle_type>::type>::type
  square_euclidean_distance(rectangle_type& lvalue, const point_type& rvalue) {
    typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::coordinate_difference xdist, ydist;
    xdist = euclidean_distance(lvalue, rvalue, HORIZONTAL);
    ydist = euclidean_distance(lvalue, rvalue, VERTICAL);
    return (xdist * xdist) + (ydist * ydist);
  }

  struct y_r_sed2 : gtl_yes {};

  template <typename rectangle_type, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_sed2, typename is_rectangle_concept< typename geometry_concept<rectangle_type>::type>::type,
                                       typename is_rectangle_concept< typename geometry_concept<rectangle_type_2>::type>::type>::type,
    typename rectangle_difference_type<rectangle_type>::type>::type
  square_euclidean_distance(const rectangle_type& lvalue, const rectangle_type_2& rvalue) {
    typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::coordinate_difference xdist, ydist;
    xdist = euclidean_distance(lvalue, rvalue, HORIZONTAL);
    ydist = euclidean_distance(lvalue, rvalue, VERTICAL);
    return (xdist * xdist) + (ydist * ydist);
  }

  struct y_r_edist : gtl_yes {};

  template <typename rectangle_type, typename point_type>
  typename enable_if< typename gtl_and_3<y_r_edist, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                                          typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                       typename rectangle_distance_type<rectangle_type>::type>::type
  euclidean_distance(rectangle_type& lvalue, const point_type& rvalue) {
    return std::sqrt((double)(square_euclidean_distance(lvalue, rvalue)));
  }

  struct y_r_edist2 : gtl_yes {};

  template <typename rectangle_type, typename rectangle_type_2>
  typename enable_if< typename gtl_and_3<y_r_edist2, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                                          typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
                       typename rectangle_distance_type<rectangle_type>::type>::type
  euclidean_distance(const rectangle_type& lvalue, const rectangle_type_2& rvalue) {
    double val = (int)square_euclidean_distance(lvalue, rvalue);
    return std::sqrt(val);
  }

  struct y_r_mdist : gtl_yes {};

  template <typename rectangle_type, typename point_type>
  typename enable_if<
    typename gtl_and_3<y_r_mdist, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                       typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
    typename rectangle_difference_type<rectangle_type>::type>::type
  manhattan_distance(rectangle_type& lvalue, const point_type& rvalue) {
    typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::coordinate_difference xdist, ydist;
    xdist = euclidean_distance(lvalue, rvalue, HORIZONTAL);
    ydist = euclidean_distance(lvalue, rvalue, VERTICAL);
    return xdist + ydist;
  }

  struct y_r_mdist2 : gtl_yes {};

  template <typename rectangle_type, typename rectangle_type_2>
  typename enable_if<
    typename gtl_and_3<y_r_mdist2, typename is_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type,
                                       typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
    typename rectangle_difference_type<rectangle_type>::type>::type
  manhattan_distance(const rectangle_type& lvalue, const rectangle_type_2& rvalue) {
    typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::coordinate_difference xdist, ydist;
    xdist = euclidean_distance(lvalue, rvalue, HORIZONTAL);
    ydist = euclidean_distance(lvalue, rvalue, VERTICAL);
    return xdist + ydist;
  }

  struct y_r_scale_up : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_scale_up, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                     rectangle_type>::type &
  scale_up(rectangle_type& rectangle,
           typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::unsigned_area_type factor) {
    typename rectangle_interval_type<rectangle_type>::type h = horizontal(rectangle);
    horizontal(rectangle, scale_up(h, factor));
    typename rectangle_interval_type<rectangle_type>::type v = vertical(rectangle);
    vertical(rectangle, scale_up(v, factor));
    return rectangle;
  }

  struct y_r_scale_down : gtl_yes {};

  template <typename rectangle_type>
  typename enable_if<typename gtl_and<y_r_scale_down, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                     rectangle_type>::type &
  scale_down(rectangle_type& rectangle,
             typename coordinate_traits<typename rectangle_coordinate_type<rectangle_type>::type>::unsigned_area_type factor) {
    typename rectangle_interval_type<rectangle_type>::type h = horizontal(rectangle);
    horizontal(rectangle, scale_down(h, factor));
    typename rectangle_interval_type<rectangle_type>::type v = vertical(rectangle);
    vertical(rectangle, scale_down(v, factor));
    return rectangle;
  }

  struct y_r_scale : gtl_yes {};

  template <typename rectangle_type, typename scaling_type>
  typename enable_if<typename gtl_and<y_r_scale, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                     rectangle_type>::type &
  scale(rectangle_type& rectangle, const scaling_type& scaling) {
    point_data<typename rectangle_coordinate_type<rectangle_type>::type> llp(xl(rectangle), yl(rectangle));
    point_data<typename rectangle_coordinate_type<rectangle_type>::type> urp(xl(rectangle), yl(rectangle));
    scale(llp, scaling);
    scale(urp, scaling);
    set_points(rectangle, llp, urp);
    return rectangle;
  }

  struct y_r_transform : gtl_yes {};

  template <typename rectangle_type, typename transformation_type>
  typename enable_if<typename gtl_and<y_r_transform, typename is_mutable_rectangle_concept<typename geometry_concept<rectangle_type>::type>::type>::type,
                     rectangle_type>::type &
  transform(rectangle_type& rectangle, const transformation_type& transformation) {
    point_data<typename rectangle_coordinate_type<rectangle_type>::type> llp(xl(rectangle), yl(rectangle));
    point_data<typename rectangle_coordinate_type<rectangle_type>::type> urp(xh(rectangle), yh(rectangle));
    transform(llp, transformation);
    transform(urp, transformation);
    set_points(rectangle, llp, urp);
    return rectangle;
  }

  template <typename rectangle_type_1, typename rectangle_type_2>
  class less_rectangle_concept {
  private:
    orientation_2d orient_;
  public:
    inline less_rectangle_concept(orientation_2d orient = VERTICAL) : orient_(orient) {}
    typename enable_if<
      typename gtl_and< typename is_rectangle_concept<typename geometry_concept<rectangle_type_1>::type>::type,
                        typename is_rectangle_concept<typename geometry_concept<rectangle_type_2>::type>::type>::type,
      bool>::type
    operator () (const rectangle_type_1& a,
                 const rectangle_type_2& b) const {
      typedef typename rectangle_coordinate_type<rectangle_type_1>::type Unit;
      Unit vl1 = get(get(a, orient_), LOW);
      Unit vl2 = get(get(b, orient_), LOW);
      if(vl1 > vl2) return false;
      if(vl1 == vl2) {
        orientation_2d perp = orient_.get_perpendicular();
        Unit hl1 = get(get(a, perp), LOW);
        Unit hl2 = get(get(b, perp), LOW);
        if(hl1 > hl2) return false;
        if(hl1 == hl2) {
          Unit vh1 = get(get(a, orient_), HIGH);
          Unit vh2 = get(get(b, orient_), HIGH);
          if(vh1 > vh2) return false;
          if(vh1 == vh2) {
            Unit hh1 = get(get(a, perp), HIGH);
            Unit hh2 = get(get(b, perp), HIGH);
            return hh1 < hh2;
          }
        }
      }
      return true;
    }

  };

  template <typename T>
  template <typename interval_type_1>
  inline void rectangle_data<T>::set(orientation_2d orient, const interval_type_1& interval) {
    assign(ranges_[orient.to_int()], interval);
  }

  template <class T>
  template <class T2>
  rectangle_data<T>& rectangle_data<T>::operator=(const T2& rvalue) {
    assign(*this, rvalue);
    return *this;
  }

  template <class T>
  template <class T2>
  bool rectangle_data<T>::operator==(const T2& rvalue) const {
    return equivalence(*this, rvalue);
  }

  template <typename T>
  struct geometry_concept<rectangle_data<T> > {
    typedef rectangle_concept type;
  };
}
}
#endif
