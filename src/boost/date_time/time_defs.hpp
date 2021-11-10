#ifndef DATE_TIME_TIME_PRECISION_LIMITS_HPP
#define DATE_TIME_TIME_PRECISION_LIMITS_HPP

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */



/*! \file time_defs.hpp 
  This file contains nice definitions for handling the resoluion of various time
  reprsentations.
*/

namespace boost {
namespace date_time {

  //!Defines some nice types for handling time level resolutions
  enum time_resolutions {
    sec,
    tenth,
    hundreth, // deprecated misspelled version of hundredth
    hundredth = hundreth,
    milli,
    ten_thousandth,
    micro,
    nano,
    NumResolutions
  };

  //! Flags for daylight savings or summer time
  enum dst_flags {not_dst, is_dst, calculate};


} } //namespace date_time



#endif
