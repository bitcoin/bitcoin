/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/

#ifndef BOOST_POLYGON_ISOTROPY_HPP
#define BOOST_POLYGON_ISOTROPY_HPP

//external
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <list>
//#include <iostream>
#include <algorithm>
#include <limits>
#include <iterator>
#include <string>

#ifndef BOOST_POLYGON_NO_DEPS

#include <boost/config.hpp>
#ifdef BOOST_MSVC
#define BOOST_POLYGON_MSVC
#endif
#ifdef BOOST_INTEL
#define BOOST_POLYGON_ICC
#endif
#ifdef BOOST_HAS_LONG_LONG
#define BOOST_POLYGON_USE_LONG_LONG
typedef boost::long_long_type polygon_long_long_type;
typedef boost::ulong_long_type polygon_ulong_long_type;
//typedef long long polygon_long_long_type;
//typedef unsigned long long polygon_ulong_long_type;
#endif
#else

#ifdef _WIN32
#define BOOST_POLYGON_MSVC
#endif
#ifdef __ICC
#define BOOST_POLYGON_ICC
#endif
#define BOOST_POLYGON_USE_LONG_LONG
typedef long long polygon_long_long_type;
typedef unsigned long long polygon_ulong_long_type;
#endif

namespace boost { namespace polygon{

  enum GEOMETRY_CONCEPT_ID {
    COORDINATE_CONCEPT,
    INTERVAL_CONCEPT,
    POINT_CONCEPT,
    POINT_3D_CONCEPT,
    RECTANGLE_CONCEPT,
    POLYGON_90_CONCEPT,
    POLYGON_90_WITH_HOLES_CONCEPT,
    POLYGON_45_CONCEPT,
    POLYGON_45_WITH_HOLES_CONCEPT,
    POLYGON_CONCEPT,
    POLYGON_WITH_HOLES_CONCEPT,
    POLYGON_90_SET_CONCEPT,
    POLYGON_45_SET_CONCEPT,
    POLYGON_SET_CONCEPT
  };

  struct undefined_concept {};

  template <typename T>
  struct geometry_concept { typedef undefined_concept type; };

  template <typename GCT, typename T>
  struct view_of {};

  template <typename T1, typename T2>
  view_of<T1, T2> view_as(const T2& obj) { return view_of<T1, T2>(obj); }

  template <typename T, bool /*enable*/ = true>
  struct coordinate_traits {};

  //used to override long double with an infinite precision datatype
  template <typename T>
  struct high_precision_type {
    typedef long double type;
  };

  template <typename T>
  T convert_high_precision_type(const typename high_precision_type<T>::type& v) {
    return T(v);
  }

  //used to override std::sort with an alternative (parallel) algorithm
  template <typename iter_type>
  void polygon_sort(iter_type _b_, iter_type _e_);

  template <typename iter_type, typename pred_type>
  void polygon_sort(iter_type _b_, iter_type _e_, const pred_type& _pred_);


  template <>
  struct coordinate_traits<int> {
    typedef int coordinate_type;
    typedef long double area_type;
#ifdef BOOST_POLYGON_USE_LONG_LONG
    typedef polygon_long_long_type manhattan_area_type;
    typedef polygon_ulong_long_type unsigned_area_type;
    typedef polygon_long_long_type coordinate_difference;
#else
    typedef long manhattan_area_type;
    typedef unsigned long unsigned_area_type;
    typedef long coordinate_difference;
#endif
    typedef long double coordinate_distance;
  };

  template<>
  struct coordinate_traits<long, sizeof(long) == sizeof(int)> {
    typedef coordinate_traits<int> cT_;
    typedef cT_::coordinate_type coordinate_type;
    typedef cT_::area_type area_type;
    typedef cT_::manhattan_area_type manhattan_area_type;
    typedef cT_::unsigned_area_type unsigned_area_type;
    typedef cT_::coordinate_difference coordinate_difference;
    typedef cT_::coordinate_distance coordinate_distance;
  };

#ifdef BOOST_POLYGON_USE_LONG_LONG
  template <>
  struct coordinate_traits<polygon_long_long_type> {
    typedef polygon_long_long_type coordinate_type;
    typedef long double area_type;
    typedef polygon_long_long_type manhattan_area_type;
    typedef polygon_ulong_long_type unsigned_area_type;
    typedef polygon_long_long_type coordinate_difference;
    typedef long double coordinate_distance;
  };

  template<>
  struct coordinate_traits<long, sizeof(long) == sizeof(polygon_long_long_type)> {
    typedef coordinate_traits<polygon_long_long_type> cT_;
    typedef cT_::coordinate_type coordinate_type;
    typedef cT_::area_type area_type;
    typedef cT_::manhattan_area_type manhattan_area_type;
    typedef cT_::unsigned_area_type unsigned_area_type;
    typedef cT_::coordinate_difference coordinate_difference;
    typedef cT_::coordinate_distance coordinate_distance;
  };
#endif

  template <>
  struct coordinate_traits<float> {
    typedef float coordinate_type;
    typedef float area_type;
    typedef float manhattan_area_type;
    typedef float unsigned_area_type;
    typedef float coordinate_difference;
    typedef float coordinate_distance;
  };

  template <>
  struct coordinate_traits<double> {
    typedef double coordinate_type;
    typedef double area_type;
    typedef double manhattan_area_type;
    typedef double unsigned_area_type;
    typedef double coordinate_difference;
    typedef double coordinate_distance;
  };

  template <>
  struct coordinate_traits<long double> {
    typedef long double coordinate_type;
    typedef long double area_type;
    typedef long double manhattan_area_type;
    typedef long double unsigned_area_type;
    typedef long double coordinate_difference;
    typedef long double coordinate_distance;
  };

  template <typename T>
  struct scaling_policy {
    template <typename T2>
    static inline T round(T2 t2) {
      return (T)std::floor(t2+0.5);
    }

    static inline T round(T t2) {
      return t2;
    }
  };

  struct coordinate_concept {};

  template <>
  struct geometry_concept<int> { typedef coordinate_concept type; };
#ifdef BOOST_POLYGON_USE_LONG_LONG
  template <>
  struct geometry_concept<polygon_long_long_type> { typedef coordinate_concept type; };
#endif
  template <>
  struct geometry_concept<float> { typedef coordinate_concept type; };
  template <>
  struct geometry_concept<double> { typedef coordinate_concept type; };
  template <>
  struct geometry_concept<long double> { typedef coordinate_concept type; };

  struct gtl_no { static const bool value = false; };
  struct gtl_yes { typedef gtl_yes type;
    static const bool value = true; };

  template <bool T, bool T2>
  struct gtl_and_c { typedef gtl_no type; };
  template <>
  struct gtl_and_c<true, true> { typedef gtl_yes type; };

  template <typename T, typename T2>
  struct gtl_and : gtl_and_c<T::value, T2::value> {};
  template <typename T, typename T2, typename T3>
  struct gtl_and_3 { typedef typename gtl_and<
                       T, typename gtl_and<T2, T3>::type>::type type; };

  template <typename T, typename T2, typename T3, typename T4>
  struct gtl_and_4 { typedef typename gtl_and_3<
                       T, T2, typename gtl_and<T3, T4>::type>::type type; };
  template <typename T, typename T2>
  struct gtl_or { typedef gtl_yes type; };
  template <typename T>
  struct gtl_or<T, T> { typedef T type; };

  template <typename T, typename T2, typename T3>
  struct gtl_or_3 { typedef typename gtl_or<
                      T, typename gtl_or<T2, T3>::type>::type type; };

  template <typename T, typename T2, typename T3, typename T4>
  struct gtl_or_4 { typedef typename gtl_or<
                      T, typename gtl_or_3<T2, T3, T4>::type>::type type; };

  template <typename T>
  struct gtl_not { typedef gtl_no type; };
  template <>
  struct gtl_not<gtl_no> { typedef gtl_yes type; };

  template <typename T>
  struct gtl_if {
#ifdef BOOST_POLYGON_MSVC
    typedef gtl_no type;
#endif
  };
  template <>
  struct gtl_if<gtl_yes> { typedef gtl_yes type; };

  template <typename T, typename T2>
  struct gtl_same_type { typedef gtl_no type; };
  template <typename T>
  struct gtl_same_type<T, T> { typedef gtl_yes type; };
  template <typename T, typename T2>
  struct gtl_different_type { typedef typename gtl_not<typename gtl_same_type<T, T2>::type>::type type; };

  struct manhattan_domain {};
  struct forty_five_domain {};
  struct general_domain {};

  template <typename T>
  struct geometry_domain { typedef general_domain type; };

  template <typename domain_type, typename coordinate_type>
  struct area_type_by_domain { typedef typename coordinate_traits<coordinate_type>::area_type type; };
  template <typename coordinate_type>
  struct area_type_by_domain<manhattan_domain, coordinate_type> {
    typedef typename coordinate_traits<coordinate_type>::manhattan_area_type type; };

  template<bool E, class R = void>
  struct enable_if_ {
      typedef R type;
  };

  template<class R>
  struct enable_if_<false, R> { };

  template<class E, class R = void>
  struct enable_if
      : enable_if_<E::value, R> { };

  struct y_c_edist : gtl_yes {};

  template <typename coordinate_type_1, typename coordinate_type_2>
    typename enable_if<
    typename gtl_and_3<y_c_edist, typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type,
                       typename gtl_same_type<typename geometry_concept<coordinate_type_1>::type, coordinate_concept>::type>::type,
    typename coordinate_traits<coordinate_type_1>::coordinate_difference>::type
  euclidean_distance(const coordinate_type_1& lvalue, const coordinate_type_2& rvalue) {
    typedef typename coordinate_traits<coordinate_type_1>::coordinate_difference Unit;
    return (lvalue < rvalue) ? (Unit)rvalue - (Unit)lvalue : (Unit)lvalue - (Unit)rvalue;
  }



  // predicated_swap swaps a and b if pred is true

  // predicated_swap is guarenteed to behave the same as
  // if(pred){
  //   T tmp = a;
  //   a = b;
  //   b = tmp;
  // }
  // but will not generate a branch instruction.
  // predicated_swap always creates a temp copy of a, but does not
  // create more than one temp copy of an input.
  // predicated_swap can be used to optimize away branch instructions in C++
  template <class T>
  inline bool predicated_swap(const bool& pred,
                              T& a,
                              T& b) {
    const T tmp = a;
    const T* input[2] = {&b, &tmp};
    a = *input[!pred];
    b = *input[pred];
    return pred;
  }

  enum direction_1d_enum { LOW = 0, HIGH = 1,
                           LEFT = 0, RIGHT = 1,
                           CLOCKWISE = 0, COUNTERCLOCKWISE = 1,
                           REVERSE = 0, FORWARD = 1,
                           NEGATIVE = 0, POSITIVE = 1 };
  enum orientation_2d_enum { HORIZONTAL = 0, VERTICAL = 1 };
  enum direction_2d_enum { WEST = 0, EAST = 1, SOUTH = 2, NORTH = 3 };
  enum orientation_3d_enum { PROXIMAL = 2 };
  enum direction_3d_enum { DOWN = 4, UP = 5 };
  enum winding_direction {
    clockwise_winding = 0,
    counterclockwise_winding = 1,
    unknown_winding = 2
  };

  class direction_2d;
  class direction_3d;
  class orientation_2d;

  class direction_1d {
  private:
    unsigned int val_;
    explicit direction_1d(int d);
  public:
    inline direction_1d() : val_(LOW) {}
    inline direction_1d(const direction_1d& that) : val_(that.val_) {}
    inline direction_1d(const direction_1d_enum val) : val_(val) {}
    explicit inline direction_1d(const direction_2d& that);
    explicit inline direction_1d(const direction_3d& that);
    inline direction_1d& operator = (const direction_1d& d) {
      val_ = d.val_; return * this; }
    inline bool operator==(direction_1d d) const { return (val_ == d.val_); }
    inline bool operator!=(direction_1d d) const { return !((*this) == d); }
    inline unsigned int to_int(void) const { return val_; }
    inline direction_1d& backward() { val_ ^= 1; return *this; }
    inline int get_sign() const { return val_ * 2 - 1; }
  };

  class direction_2d;

  class orientation_2d {
  private:
    unsigned int val_;
    explicit inline orientation_2d(int o);
  public:
    inline orientation_2d() : val_(HORIZONTAL) {}
    inline orientation_2d(const orientation_2d& ori) : val_(ori.val_) {}
    inline orientation_2d(const orientation_2d_enum val) : val_(val) {}
    explicit inline orientation_2d(const direction_2d& that);
    inline orientation_2d& operator=(const orientation_2d& ori) {
      val_ = ori.val_; return * this; }
    inline bool operator==(orientation_2d that) const { return (val_ == that.val_); }
    inline bool operator!=(orientation_2d that) const { return (val_ != that.val_); }
    inline unsigned int to_int() const { return (val_); }
    inline void turn_90() { val_ = val_^ 1; }
    inline orientation_2d get_perpendicular() const {
      orientation_2d retval = *this;
      retval.turn_90();
      return retval;
    }
    inline direction_2d get_direction(direction_1d dir) const;
  };

  class direction_2d {
  private:
    int val_;

  public:

    inline direction_2d() : val_(WEST) {}

    inline direction_2d(const direction_2d& that) : val_(that.val_) {}

    inline direction_2d(const direction_2d_enum val) : val_(val) {}

    inline direction_2d& operator=(const direction_2d& d) {
      val_ = d.val_;
      return * this;
    }

    inline ~direction_2d() { }

    inline bool operator==(direction_2d d) const { return (val_ == d.val_); }
    inline bool operator!=(direction_2d d) const { return !((*this) == d); }
    inline bool operator< (direction_2d d) const { return (val_ < d.val_); }
    inline bool operator<=(direction_2d d) const { return (val_ <= d.val_); }
    inline bool operator> (direction_2d d) const { return (val_ > d.val_); }
    inline bool operator>=(direction_2d d) const { return (val_ >= d.val_); }

    // Casting to int
    inline unsigned int to_int(void) const { return val_; }

    inline direction_2d backward() const {
      // flip the LSB, toggles 0 - 1   and 2 - 3
      return direction_2d(direction_2d_enum(val_ ^ 1));
    }

    // Returns a direction 90 degree left (LOW) or right(HIGH) to this one
    inline direction_2d turn(direction_1d t) const {
      return direction_2d(direction_2d_enum(val_ ^ 3 ^ (val_ >> 1) ^ t.to_int()));
    }

    // Returns a direction 90 degree left to this one
    inline direction_2d left() const {return turn(HIGH);}

    // Returns a direction 90 degree right to this one
    inline direction_2d right() const {return turn(LOW);}

    // N, E are positive, S, W are negative
    inline bool is_positive() const {return (val_ & 1);}
    inline bool is_negative() const {return !is_positive();}
    inline int get_sign() const {return ((is_positive()) << 1) -1;}

  };

  direction_1d::direction_1d(const direction_2d& that) : val_(that.to_int() & 1) {}

  orientation_2d::orientation_2d(const direction_2d& that) : val_(that.to_int() >> 1) {}

  direction_2d orientation_2d::get_direction(direction_1d dir) const {
    return direction_2d(direction_2d_enum((val_ << 1) + dir.to_int()));
  }

  class orientation_3d {
  private:
    unsigned int val_;
    explicit inline orientation_3d(int o);
  public:
    inline orientation_3d() : val_((int)HORIZONTAL) {}
    inline orientation_3d(const orientation_3d& ori) : val_(ori.val_) {}
    inline orientation_3d(orientation_2d ori) : val_(ori.to_int()) {}
    inline orientation_3d(const orientation_3d_enum val) : val_(val) {}
    explicit inline orientation_3d(const direction_2d& that);
    explicit inline orientation_3d(const direction_3d& that);
    inline ~orientation_3d() {  }
    inline orientation_3d& operator=(const orientation_3d& ori) {
      val_ = ori.val_; return * this; }
    inline bool operator==(orientation_3d that) const { return (val_ == that.val_); }
    inline bool operator!=(orientation_3d that) const { return (val_ != that.val_); }
    inline unsigned int to_int() const { return (val_); }
    inline direction_3d get_direction(direction_1d dir) const;
  };

  class direction_3d {
  private:
    int val_;

  public:

    inline direction_3d() : val_(WEST) {}

    inline direction_3d(direction_2d that) : val_(that.to_int()) {}
    inline direction_3d(const direction_3d& that) : val_(that.val_) {}

    inline direction_3d(const direction_2d_enum val) : val_(val) {}
    inline direction_3d(const direction_3d_enum val) : val_(val) {}

    inline direction_3d& operator=(direction_3d d) {
      val_ = d.val_;
      return * this;
    }

    inline ~direction_3d() { }

    inline bool operator==(direction_3d d) const { return (val_ == d.val_); }
    inline bool operator!=(direction_3d d) const { return !((*this) == d); }
    inline bool operator< (direction_3d d) const { return (val_ < d.val_); }
    inline bool operator<=(direction_3d d) const { return (val_ <= d.val_); }
    inline bool operator> (direction_3d d) const { return (val_ > d.val_); }
    inline bool operator>=(direction_3d d) const { return (val_ >= d.val_); }

    // Casting to int
    inline unsigned int to_int(void) const { return val_; }

    inline direction_3d backward() const {
      // flip the LSB, toggles 0 - 1   and 2 - 3 and 4 - 5
      return direction_2d(direction_2d_enum(val_ ^ 1));
    }

    // N, E, U are positive, S, W, D are negative
    inline bool is_positive() const {return (val_ & 1);}
    inline bool is_negative() const {return !is_positive();}
    inline int get_sign() const {return ((is_positive()) << 1) -1;}

  };

  direction_1d::direction_1d(const direction_3d& that) : val_(that.to_int() & 1) {}
  orientation_3d::orientation_3d(const direction_3d& that) : val_(that.to_int() >> 1) {}
  orientation_3d::orientation_3d(const direction_2d& that) : val_(that.to_int() >> 1) {}

  direction_3d orientation_3d::get_direction(direction_1d dir) const {
    return direction_3d(direction_3d_enum((val_ << 1) + dir.to_int()));
  }

}
}
#endif
