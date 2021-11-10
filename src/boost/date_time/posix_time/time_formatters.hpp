#ifndef POSIXTIME_FORMATTERS_HPP___
#define POSIXTIME_FORMATTERS_HPP___

/* Copyright (c) 2002-2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/iso_format.hpp>
#include <boost/date_time/date_format_simple.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/time_formatting_streams.hpp>
#include <boost/date_time/time_resolution_traits.hpp> // absolute_value
#include <boost/date_time/time_parsing.hpp>

/* NOTE: The "to_*_string" code for older compilers, ones that define
 * BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS, is located in
 * formatters_limited.hpp
 */

namespace boost {

namespace posix_time {

  // template function called by wrapper functions:
  // to_*_string(time_duration) & to_*_wstring(time_duration)
  template<class charT>
  inline std::basic_string<charT> to_simple_string_type(time_duration td) {
    std::basic_ostringstream<charT> ss;
    if(td.is_special()) {
      /* simply using 'ss << td.get_rep()' won't work on compilers
       * that don't support locales. This way does. */
      // switch copied from date_names_put.hpp
      switch(td.get_rep().as_special())
      {
      case not_a_date_time:
        //ss << "not-a-number";
        ss << "not-a-date-time";
        break;
      case pos_infin:
        ss << "+infinity";
        break;
      case neg_infin:
        ss << "-infinity";
        break;
      default:
        ss << "";
      }
    }
    else {
      charT fill_char = '0';
      if(td.is_negative()) {
        ss << '-';
      }
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.hours()) << ":";
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.minutes()) << ":";
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.seconds());
      //TODO the following is totally non-generic, yelling FIXME
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
      boost::int64_t frac_sec =
        date_time::absolute_value(td.fractional_seconds());
      // JDG [7/6/02 VC++ compatibility]
      charT buff[32];
      _i64toa(frac_sec, buff, 10);
#else
      time_duration::fractional_seconds_type frac_sec =
        date_time::absolute_value(td.fractional_seconds());
#endif
      if (frac_sec != 0) {
        ss  << "." << std::setw(time_duration::num_fractional_digits())
            << std::setfill(fill_char)

          // JDG [7/6/02 VC++ compatibility]
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
            << buff;
#else
        << frac_sec;
#endif
      }
    }// else
    return ss.str();
  }
  //! Time duration to string -hh::mm::ss.fffffff. Example: 10:09:03.0123456
  /*!\ingroup time_format
   */
  inline std::string to_simple_string(time_duration td) {
    return to_simple_string_type<char>(td);
  }


  // template function called by wrapper functions:
  // to_*_string(time_duration) & to_*_wstring(time_duration)
  template<class charT>
  inline std::basic_string<charT> to_iso_string_type(time_duration td)
  {
    std::basic_ostringstream<charT> ss;
    if(td.is_special()) {
      /* simply using 'ss << td.get_rep()' won't work on compilers
       * that don't support locales. This way does. */
      // switch copied from date_names_put.hpp
      switch(td.get_rep().as_special()) {
      case not_a_date_time:
        //ss << "not-a-number";
        ss << "not-a-date-time";
        break;
      case pos_infin:
        ss << "+infinity";
        break;
      case neg_infin:
        ss << "-infinity";
        break;
      default:
        ss << "";
      }
    }
    else {
      charT fill_char = '0';
      if(td.is_negative()) {
        ss << '-';
      }
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.hours());
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.minutes());
      ss  << std::setw(2) << std::setfill(fill_char)
          << date_time::absolute_value(td.seconds());
      //TODO the following is totally non-generic, yelling FIXME
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
      boost::int64_t frac_sec =
        date_time::absolute_value(td.fractional_seconds());
      // JDG [7/6/02 VC++ compatibility]
      charT buff[32];
      _i64toa(frac_sec, buff, 10);
#else
      time_duration::fractional_seconds_type frac_sec =
        date_time::absolute_value(td.fractional_seconds());
#endif
      if (frac_sec != 0) {
        ss  << "." << std::setw(time_duration::num_fractional_digits())
            << std::setfill(fill_char)

          // JDG [7/6/02 VC++ compatibility]
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
            << buff;
#else
        << frac_sec;
#endif
      }
    }// else
    return ss.str();
  }
  //! Time duration in iso format -hhmmss,fffffff Example: 10:09:03,0123456
  /*!\ingroup time_format
   */
  inline std::string to_iso_string(time_duration td){
    return to_iso_string_type<char>(td);
  }

  //! Time to simple format CCYY-mmm-dd hh:mm:ss.fffffff
  /*!\ingroup time_format
   */
  template<class charT>
  inline std::basic_string<charT> to_simple_string_type(ptime t)
  {
    // can't use this w/gcc295, no to_simple_string_type<>(td) available
    std::basic_string<charT> ts = gregorian::to_simple_string_type<charT>(t.date());// + " ";
    if(!t.time_of_day().is_special()) {
      charT space = ' ';
      return ts + space + to_simple_string_type<charT>(t.time_of_day());
    }
    else {
      return ts;
    }
  }
  inline std::string to_simple_string(ptime t){
    return to_simple_string_type<char>(t);
  }

  // function called by wrapper functions to_*_string(time_period)
  // & to_*_wstring(time_period)
  template<class charT>
  inline std::basic_string<charT> to_simple_string_type(time_period tp)
  {
    charT beg = '[', mid = '/', end = ']';
    std::basic_string<charT> d1(to_simple_string_type<charT>(tp.begin()));
    std::basic_string<charT> d2(to_simple_string_type<charT>(tp.last()));
    return std::basic_string<charT>(beg + d1 + mid + d2 + end);
  }
  //! Convert to string of form [YYYY-mmm-DD HH:MM::SS.ffffff/YYYY-mmm-DD HH:MM::SS.fffffff]
  /*!\ingroup time_format
   */
  inline std::string to_simple_string(time_period tp){
    return to_simple_string_type<char>(tp);
  }

  // function called by wrapper functions to_*_string(time_period)
  // & to_*_wstring(time_period)
  template<class charT>
  inline std::basic_string<charT> to_iso_string_type(ptime t)
  {
    std::basic_string<charT> ts = gregorian::to_iso_string_type<charT>(t.date());// + "T";
    if(!t.time_of_day().is_special()) {
      charT sep = 'T';
      return ts + sep + to_iso_string_type<charT>(t.time_of_day());
    }
    else {
      return ts;
    }
  }
  //! Convert iso short form YYYYMMDDTHHMMSS where T is the date-time separator
  /*!\ingroup time_format
   */
  inline std::string to_iso_string(ptime t){
    return to_iso_string_type<char>(t);
  }


  // function called by wrapper functions to_*_string(time_period)
  // & to_*_wstring(time_period)
  template<class charT>
  inline std::basic_string<charT> to_iso_extended_string_type(ptime t)
  {
    std::basic_string<charT> ts = gregorian::to_iso_extended_string_type<charT>(t.date());// + "T";
    if(!t.time_of_day().is_special()) {
      charT sep = 'T';
      return ts + sep + to_simple_string_type<charT>(t.time_of_day());
    }
    else {
      return ts;
    }
  }
  //! Convert to form YYYY-MM-DDTHH:MM:SS where T is the date-time separator
  /*!\ingroup time_format
   */
  inline std::string to_iso_extended_string(ptime t){
    return to_iso_extended_string_type<char>(t);
  }

#if !defined(BOOST_NO_STD_WSTRING)
  //! Time duration to wstring -hh::mm::ss.fffffff. Example: 10:09:03.0123456
  /*!\ingroup time_format
   */
  inline std::wstring to_simple_wstring(time_duration td) {
    return to_simple_string_type<wchar_t>(td);
  }
  //! Time duration in iso format -hhmmss,fffffff Example: 10:09:03,0123456
  /*!\ingroup time_format
   */
  inline std::wstring to_iso_wstring(time_duration td){
    return to_iso_string_type<wchar_t>(td);
  }
    inline std::wstring to_simple_wstring(ptime t){
    return to_simple_string_type<wchar_t>(t);
  }
  //! Convert to wstring of form [YYYY-mmm-DD HH:MM::SS.ffffff/YYYY-mmm-DD HH:MM::SS.fffffff]
  /*!\ingroup time_format
   */
  inline std::wstring to_simple_wstring(time_period tp){
    return to_simple_string_type<wchar_t>(tp);
  }
  //! Convert iso short form YYYYMMDDTHHMMSS where T is the date-time separator
  /*!\ingroup time_format
   */
  inline std::wstring to_iso_wstring(ptime t){
    return to_iso_string_type<wchar_t>(t);
  }
  //! Convert to form YYYY-MM-DDTHH:MM:SS where T is the date-time separator
  /*!\ingroup time_format
   */
  inline std::wstring to_iso_extended_wstring(ptime t){
    return to_iso_extended_string_type<wchar_t>(t);
  }

#endif // BOOST_NO_STD_WSTRING


} } //namespace posix_time


#endif

