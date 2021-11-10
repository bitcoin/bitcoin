#ifndef POSIX_TIME_PRE133_OPERATORS_HPP___
#define POSIX_TIME_PRE133_OPERATORS_HPP___

/* Copyright (c) 2002-2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 * $Date$
 */

/*! @file posix_time_pre133_operators.hpp
 * These input and output operators are for use with the 
 * pre 1.33 version of the date_time libraries io facet code. 
 * The operators used in version 1.33 and later can be found 
 * in posix_time_io.hpp */

#include <iostream>
#include <string>
#include <sstream>
#include "boost/date_time/compiler_config.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/date_time/posix_time/time_period.hpp"
#include "boost/date_time/time_parsing.hpp"

namespace boost {
namespace posix_time {


//The following code is removed for configurations with poor std::locale support (eg: MSVC6, gcc 2.9x)
#ifndef BOOST_DATE_TIME_NO_LOCALE
#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO)
  //! ostream operator for posix_time::time_duration
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const time_duration& td)
  {
    typedef boost::date_time::ostream_time_duration_formatter<time_duration, charT> duration_formatter;
    duration_formatter::duration_put(td, os);
    return os;
  }

  //! ostream operator for posix_time::ptime
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const ptime& t)
  {
    typedef boost::date_time::ostream_time_formatter<ptime, charT> time_formatter;
    time_formatter::time_put(t, os);
    return os;
  }

  //! ostream operator for posix_time::time_period
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const time_period& tp)
  {
    typedef boost::date_time::ostream_time_period_formatter<time_period, charT> period_formatter;
    period_formatter::period_put(tp, os);
    return os;
  }
#endif // USE_DATE_TIME_PRE_1_33_FACET_IO
/******** input streaming ********/
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is, time_duration& td)
  {
    // need to create a std::string and parse it
    std::basic_string<charT> inp_s;
    std::stringstream out_ss;
    is >> inp_s;
    typename std::basic_string<charT>::iterator b = inp_s.begin();
    // need to use both iterators because there is no requirement
    // for the data held by a std::basic_string<> be terminated with
    // any marker (such as '\0').
    typename std::basic_string<charT>::iterator e = inp_s.end();
    while(b != e){
      out_ss << is.narrow(*b, 0);
      ++b;
    }

    td = date_time::parse_delimited_time_duration<time_duration>(out_ss.str());
    return is;
  }

  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is, ptime& pt)
  {
    gregorian::date d(not_a_date_time);
    time_duration td(0,0,0);
    is >> d >> td;
    pt = ptime(d, td);

    return is;
  }

  /** operator>> for time_period. time_period must be in 
   * "[date time_duration/date time_duration]" format. */
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is, time_period& tp)
  {
    gregorian::date d(not_a_date_time);
    time_duration td(0,0,0);
    ptime beg(d, td);
    ptime end(beg);
    std::basic_string<charT> s;
    // get first date string and remove leading '['
    is >> s;
    {
      std::basic_stringstream<charT> ss;
      ss << s.substr(s.find('[')+1);
      ss >> d;
    }
    // get first time_duration & second date string, remove the '/'
    // and split into 2 strings
    is >> s; 
    {
      std::basic_stringstream<charT> ss;
      ss << s.substr(0, s.find('/'));
      ss >> td;
    }
    beg = ptime(d, td);
    {
      std::basic_stringstream<charT> ss;
      ss << s.substr(s.find('/')+1);
      ss >> d;
    }
    // get last time_duration and remove the trailing ']'
    is >> s;
    {
      std::basic_stringstream<charT> ss;
      ss << s.substr(0, s.find(']'));
      ss >> td;
    }
    end = ptime(d, td);

    tp = time_period(beg,end);
    return is;
  }


#endif //BOOST_DATE_TIME_NO_LOCALE

} } // namespaces

#endif // POSIX_TIME_PRE133_OPERATORS_HPP___
