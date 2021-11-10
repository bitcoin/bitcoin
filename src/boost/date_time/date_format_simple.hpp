#ifndef DATE_TIME_SIMPLE_FORMAT_HPP___
#define DATE_TIME_SIMPLE_FORMAT_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include "boost/date_time/parse_format_base.hpp"

namespace boost {
namespace date_time {

//! Class to provide simple basic formatting rules
template<class charT>
class simple_format {
public:

  //! String used printed is date is invalid
  static const charT* not_a_date()
  {
    return "not-a-date-time";
  }
  //! String used to for positive infinity value
  static const charT* pos_infinity()
  {  
    return "+infinity";
  }
  //! String used to for positive infinity value
  static const charT* neg_infinity()
  {
    return "-infinity";
  }
  //! Describe month format
  static month_format_spec month_format()
  {
    return month_as_short_string;
  }
  static ymd_order_spec date_order()
  {
    return ymd_order_iso; //YYYY-MM-DD
  }
  //! This format uses '-' to separate date elements
  static bool has_date_sep_chars()
  {
    return true;
  }
  //! Char to sep?
  static charT year_sep_char()
  {
    return '-';
  }
  //! char between year-month
  static charT month_sep_char()
  {
    return '-';
  }
  //! Char to separate month-day
  static charT day_sep_char()
  {
    return '-';
  }
  //! char between date-hours
  static charT hour_sep_char()
  {
    return ' ';
  }
  //! char between hour and minute
  static charT minute_sep_char()
  {
    return ':';
  }
  //! char for second
  static charT second_sep_char()
  {
    return ':';
  }

};

#ifndef BOOST_NO_STD_WSTRING

//! Specialization of formmating rules for wchar_t
template<>
class simple_format<wchar_t> {
public:

  //! String used printed is date is invalid
  static const wchar_t* not_a_date()
  {
    return L"not-a-date-time";
  }
  //! String used to for positive infinity value
  static const wchar_t* pos_infinity()
  {   
    return L"+infinity";
  }
  //! String used to for positive infinity value
  static const wchar_t* neg_infinity()
  {
    return L"-infinity";
  }
  //! Describe month format
  static month_format_spec month_format()
  {
    return month_as_short_string;
  }
  static ymd_order_spec date_order()
  {
    return ymd_order_iso; //YYYY-MM-DD
  }
  //! This format uses '-' to separate date elements
  static bool has_date_sep_chars()
  {
    return true;
  }
  //! Char to sep?
  static wchar_t year_sep_char()
  {
    return '-';
  }
  //! char between year-month
  static wchar_t month_sep_char()
  {
    return '-';
  }
  //! Char to separate month-day
  static wchar_t day_sep_char()
  {
    return '-';
  }
  //! char between date-hours
  static wchar_t hour_sep_char()
  {
    return ' ';
  }
  //! char between hour and minute
  static wchar_t minute_sep_char()
  {
    return ':';
  }
  //! char for second
  static wchar_t second_sep_char()
  {
    return ':';
  }

};

#endif // BOOST_NO_STD_WSTRING
} } //namespace date_time




#endif
