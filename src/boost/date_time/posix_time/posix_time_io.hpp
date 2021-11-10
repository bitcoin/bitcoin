#ifndef DATE_TIME_POSIX_TIME_IO_HPP__
#define DATE_TIME_POSIX_TIME_IO_HPP__

/* Copyright (c) 2004-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <locale>
#include <iostream>
#include <iterator> // i/ostreambuf_iterator
#include <boost/io/ios_state.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/date_time/period_formatter.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_period.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/conversion.hpp> // to_tm will be needed in the facets

namespace boost {
namespace posix_time {


  //! wptime_facet is depricated and will be phased out. use wtime_facet instead
  //typedef boost::date_time::time_facet<ptime, wchar_t> wptime_facet;
  //! ptime_facet is depricated and will be phased out. use time_facet instead
  //typedef boost::date_time::time_facet<ptime, char>     ptime_facet;

  //! wptime_input_facet is depricated and will be phased out. use wtime_input_facet instead
  //typedef boost::date_time::time_input_facet<ptime,wchar_t> wptime_input_facet;
  //! ptime_input_facet is depricated and will be phased out. use time_input_facet instead
  //typedef boost::date_time::time_input_facet<ptime,char>     ptime_input_facet;

  typedef boost::date_time::time_facet<ptime, wchar_t>     wtime_facet;
  typedef boost::date_time::time_facet<ptime, char>         time_facet;

  typedef boost::date_time::time_input_facet<ptime, wchar_t>     wtime_input_facet;
  typedef boost::date_time::time_input_facet<ptime, char>         time_input_facet;

  template <class CharT, class TraitsT>
  inline
  std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os,
             const ptime& p) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::time_facet<ptime, CharT> custom_ptime_facet;
    std::ostreambuf_iterator<CharT> oitr(os);
    if (std::has_facet<custom_ptime_facet>(os.getloc()))
      std::use_facet<custom_ptime_facet>(os.getloc()).put(oitr, os, os.fill(), p);
    else {
      //instantiate a custom facet for dealing with times since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues as some later point.
      custom_ptime_facet* f = new custom_ptime_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(oitr, os, os.fill(), p);
    }
    return os;
  }

  //! input operator for ptime
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, ptime& pt)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false);
    if (strm_sentry) {
      try {
        typedef typename date_time::time_input_facet<ptime, CharT> time_input_facet_local;
        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<time_input_facet_local>(is.getloc())) {
          std::use_facet<time_input_facet_local>(is.getloc()).get(sit, str_end, is, pt);
        }
        else {
          time_input_facet_local* f = new time_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, pt);
        }
      }
      catch(...) {
        // mask tells us what exceptions are turned on
        std::ios_base::iostate exception_mask = is.exceptions();
        // if the user wants exceptions on failbit, we'll rethrow our 
        // date_time exception & set the failbit
        if(std::ios_base::failbit & exception_mask) {
          try { is.setstate(std::ios_base::failbit); }
          catch(std::ios_base::failure&) {} // ignore this one
          throw; // rethrow original exception
        }
        else {
          // if the user want's to fail quietly, we simply set the failbit
          is.setstate(std::ios_base::failbit);
        }
      }
    }
    return is;
  }


  template <class CharT, class TraitsT>
  inline 
  std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os,
             const boost::posix_time::time_period& p) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::time_facet<ptime, CharT> custom_ptime_facet;
    std::ostreambuf_iterator<CharT> oitr(os);
    if (std::has_facet<custom_ptime_facet>(os.getloc())) {
      std::use_facet<custom_ptime_facet>(os.getloc()).put(oitr, os, os.fill(), p);
    }
    else {
      //instantiate a custom facet for dealing with periods since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the local did not already exist.  Of course this will be overridden
      //if the user imbues as some later point.
      custom_ptime_facet* f = new custom_ptime_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(oitr, os, os.fill(), p);
    }
    return os;
  }

  //! input operator for time_period
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, time_period& tp)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false);
    if (strm_sentry) {
      try {
        typedef typename date_time::time_input_facet<ptime, CharT> time_input_facet_local;
        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<time_input_facet_local>(is.getloc())) {
          std::use_facet<time_input_facet_local>(is.getloc()).get(sit, str_end, is, tp);
        }
        else {
          time_input_facet_local* f = new time_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, tp);
        }
      }
      catch(...) {
        std::ios_base::iostate exception_mask = is.exceptions();
        if(std::ios_base::failbit & exception_mask) {
          try { is.setstate(std::ios_base::failbit); }
          catch(std::ios_base::failure&) {}
          throw; // rethrow original exception
        }
        else {
          is.setstate(std::ios_base::failbit);
        }
      }
    }
    return is;
  }


  //! ostream operator for posix_time::time_duration 
  //  todo fix to use facet --  place holder for now...
  template <class CharT, class Traits>
  inline
  std::basic_ostream<CharT, Traits>&
  operator<<(std::basic_ostream<CharT, Traits>& os, const time_duration& td)
  {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::time_facet<ptime, CharT> custom_ptime_facet;
    std::ostreambuf_iterator<CharT> oitr(os);
    if (std::has_facet<custom_ptime_facet>(os.getloc()))
      std::use_facet<custom_ptime_facet>(os.getloc()).put(oitr, os, os.fill(), td);
    else {
      //instantiate a custom facet for dealing with times since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues as some later point.
      custom_ptime_facet* f = new custom_ptime_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(oitr, os, os.fill(), td);
    }
    return os;
  }

  //! input operator for time_duration
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, time_duration& td)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false);
    if (strm_sentry) {
      try {
        typedef typename date_time::time_input_facet<ptime, CharT> time_input_facet_local;
        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<time_input_facet_local>(is.getloc())) {
          std::use_facet<time_input_facet_local>(is.getloc()).get(sit, str_end, is, td);
        }
        else {
          time_input_facet_local* f = new time_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, td);
        }
      }
      catch(...) {
        std::ios_base::iostate exception_mask = is.exceptions();
        if(std::ios_base::failbit & exception_mask) {
          try { is.setstate(std::ios_base::failbit); }
          catch(std::ios_base::failure&) {}
          throw; // rethrow original exception
        }
        else {
          is.setstate(std::ios_base::failbit);
        }
      }
    }
    return is;
  }

} } // namespaces
#endif // DATE_TIME_POSIX_TIME_IO_HPP__
