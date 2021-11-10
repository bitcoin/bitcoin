#ifndef _DATE_TIME_WRAPPING_INT_HPP__
#define _DATE_TIME_WRAPPING_INT_HPP__

/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include "boost/config.hpp"

namespace boost {
namespace date_time {

//! A wrapping integer used to support time durations (WARNING: only instantiate with a signed type)
/*! In composite date and time types this type is used to
 *  wrap at the day boundary.
 *  Ex:
 *  A wrapping_int<short, 10> will roll over after nine, and
 *  roll under below zero. This gives a range of [0,9]
 *
 * NOTE: it is strongly recommended that wrapping_int2 be used
 * instead of wrapping_int as wrapping_int is to be depricated
 * at some point soon.
 *
 * Also Note that warnings will occur if instantiated with an
 * unsigned type. Only a signed type should be used!
 */
template<typename int_type_, int_type_ wrap_val>
class wrapping_int {
public:
  typedef int_type_ int_type;
  //typedef overflow_type_ overflow_type;
  static BOOST_CONSTEXPR int_type wrap_value() {return wrap_val;}
  //!Add, return true if wrapped
  BOOST_CXX14_CONSTEXPR wrapping_int(int_type v) : value_(v) {}
  //! Explicit converion method
  BOOST_CONSTEXPR int_type as_int()   const   {return value_;}
  BOOST_CONSTEXPR operator int_type() const   {return value_;}
  //!Add, return number of wraps performed
  /*! The sign of the returned value will indicate which direction the
   * wraps went. Ex: add a negative number and wrapping under could occur,
   * this would be indicated by a negative return value. If wrapping over
   * took place, a positive value would be returned */
  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT add(IntT v)
  {
    int_type remainder = static_cast<int_type>(v % (wrap_val));
    IntT overflow = static_cast<IntT>(v / (wrap_val));
    value_ = static_cast<int_type>(value_ + remainder);
    return calculate_wrap(overflow);
  }
  //! Subtract will return '+d' if wrapping under took place ('d' is the number of wraps)
  /*! The sign of the returned value will indicate which direction the
   * wraps went (positive indicates wrap under, negative indicates wrap over).
   * Ex: subtract a negative number and wrapping over could
   * occur, this would be indicated by a negative return value. If
   * wrapping under took place, a positive value would be returned. */
  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT subtract(IntT v)
  {
    int_type remainder = static_cast<int_type>(v % (wrap_val));
    IntT underflow = static_cast<IntT>(-(v / (wrap_val)));
    value_ = static_cast<int_type>(value_ - remainder);
    return calculate_wrap(underflow) * -1;
  }
private:
  int_type value_;

  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT calculate_wrap(IntT wrap)
  {
    if ((value_) >= wrap_val)
    {
      ++wrap;
      value_ -= (wrap_val);
    }
    else if(value_ < 0)
    {
      --wrap;
      value_ += (wrap_val);
    }
    return wrap;
  }

};


//! A wrapping integer used to wrap around at the top (WARNING: only instantiate with a signed type)
/*! Bad name, quick impl to fix a bug -- fix later!!
 *  This allows the wrap to restart at a value other than 0.
 */
template<typename int_type_, int_type_ wrap_min, int_type_ wrap_max>
class wrapping_int2 {
public:
  typedef int_type_ int_type;
  static BOOST_CONSTEXPR int_type wrap_value() {return wrap_max;}
  static BOOST_CONSTEXPR int_type min_value()  {return wrap_min;}
  /*! If initializing value is out of range of [wrap_min, wrap_max],
   * value will be initialized to closest of min or max */
  BOOST_CXX14_CONSTEXPR wrapping_int2(int_type v) : value_(v) {
    if(value_ < wrap_min)
    {
      value_ = wrap_min;
    }
    if(value_ > wrap_max)
    {
      value_ = wrap_max;
    }
  }
  //! Explicit converion method
  BOOST_CONSTEXPR int_type as_int()   const   {return value_;}
  BOOST_CONSTEXPR operator int_type() const {return value_;}
  //!Add, return number of wraps performed
  /*! The sign of the returned value will indicate which direction the
   * wraps went. Ex: add a negative number and wrapping under could occur,
   * this would be indicated by a negative return value. If wrapping over
   * took place, a positive value would be returned */
  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT add(IntT v)
  {
    int_type remainder = static_cast<int_type>(v % (wrap_max - wrap_min + 1));
    IntT overflow = static_cast<IntT>(v / (wrap_max - wrap_min + 1));
    value_ = static_cast<int_type>(value_ + remainder);
    return calculate_wrap(overflow);
  }
  //! Subtract will return '-d' if wrapping under took place ('d' is the number of wraps)
  /*! The sign of the returned value will indicate which direction the
   * wraps went. Ex: subtract a negative number and wrapping over could
   * occur, this would be indicated by a positive return value. If
   * wrapping under took place, a negative value would be returned */
  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT subtract(IntT v)
  {
    int_type remainder = static_cast<int_type>(v % (wrap_max - wrap_min + 1));
    IntT underflow = static_cast<IntT>(-(v / (wrap_max - wrap_min + 1)));
    value_ = static_cast<int_type>(value_ - remainder);
    return calculate_wrap(underflow);
  }

private:
  int_type value_;

  template< typename IntT >
  BOOST_CXX14_CONSTEXPR IntT calculate_wrap(IntT wrap)
  {
    if ((value_) > wrap_max)
    {
      ++wrap;
      value_ -= (wrap_max - wrap_min + 1);
    }
    else if((value_) < wrap_min)
    {
      --wrap;
      value_ += (wrap_max - wrap_min + 1);
    }
    return wrap;
  }
};



} } //namespace date_time



#endif

