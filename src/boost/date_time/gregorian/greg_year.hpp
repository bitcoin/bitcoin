#ifndef GREG_YEAR_HPP___
#define GREG_YEAR_HPP___

/* Copyright (c) 2002,2003, 2020 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */

#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/constrained_value.hpp>
#include <stdexcept>
#include <string>

namespace boost {
namespace gregorian {

  //! Exception type for gregorian year
  struct BOOST_SYMBOL_VISIBLE bad_year : public std::out_of_range
  {
    bad_year() : 
      std::out_of_range(std::string("Year is out of valid range: 1400..9999")) 
    {}
  };
  //! Policy class that declares error handling gregorian year type
  typedef CV::simple_exception_policy<unsigned short, 1400, 9999, bad_year> greg_year_policies;

  //! Generated representation for gregorian year
  typedef CV::constrained_value<greg_year_policies> greg_year_rep;

  //! Represent a year (range 1400 - 9999) 
  /*! This small class allows for simple conversion an integer value into
      a year for the gregorian calendar.  This currently only allows a
      range of 1400 to 9999.  Both ends of the range are a bit arbitrary
      at the moment, but they are the limits of current testing of the 
      library.  As such they may be increased in the future.
  */
  class BOOST_SYMBOL_VISIBLE greg_year : public greg_year_rep {
  public:
    BOOST_CXX14_CONSTEXPR greg_year(value_type year) : greg_year_rep(year) {}
    BOOST_CXX14_CONSTEXPR operator value_type()  const {return value_;}
  };



} } //namespace gregorian



#endif
