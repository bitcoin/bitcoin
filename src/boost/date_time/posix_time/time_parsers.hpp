#ifndef POSIXTIME_PARSERS_HPP___
#define POSIXTIME_PARSERS_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/time_parsing.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"

 
namespace boost {

namespace posix_time {

  //! Creates a time_duration object from a delimited string
  /*! Expected format for string is "[-]h[h][:mm][:ss][.fff]".
   * A negative duration will be created if the first character in
   * string is a '-', all other '-' will be treated as delimiters.
   * Accepted delimiters are "-:,.". */
  inline time_duration duration_from_string(const std::string& s) {
    return date_time::parse_delimited_time_duration<time_duration>(s);
  }

  inline ptime time_from_string(const std::string& s) {
    return date_time::parse_delimited_time<ptime>(s, ' ');
  }

  inline ptime from_iso_string(const std::string& s) {
    return date_time::parse_iso_time<ptime>(s, 'T');
  }

  inline ptime from_iso_extended_string(const std::string& s) {
    if (s.size() == 10) { //assume we just have a date which is 10 chars
      gregorian::date d = gregorian::from_simple_string(s);
      return ptime( d );
    }
    return date_time::parse_delimited_time<ptime>(s, 'T');
  }



} } //namespace posix_time


#endif

