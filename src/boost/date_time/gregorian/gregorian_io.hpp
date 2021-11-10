#ifndef DATE_TIME_GREGORIAN_IO_HPP__
#define DATE_TIME_GREGORIAN_IO_HPP__

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
#include <boost/date_time/date_facet.hpp>
#include <boost/date_time/period_parser.hpp>
#include <boost/date_time/period_formatter.hpp>
#include <boost/date_time/special_values_parser.hpp>
#include <boost/date_time/special_values_formatter.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/gregorian/conversion.hpp> // to_tm will be needed in the facets

namespace boost {
namespace gregorian {


  typedef boost::date_time::period_formatter<wchar_t> wperiod_formatter;
  typedef boost::date_time::period_formatter<char>    period_formatter;
  
  typedef boost::date_time::date_facet<date,wchar_t> wdate_facet;
  typedef boost::date_time::date_facet<date,char>    date_facet;

  typedef boost::date_time::period_parser<date,char>       period_parser;
  typedef boost::date_time::period_parser<date,wchar_t>    wperiod_parser;
    
  typedef boost::date_time::special_values_formatter<char> special_values_formatter; 
  typedef boost::date_time::special_values_formatter<wchar_t> wspecial_values_formatter; 
  
  typedef boost::date_time::special_values_parser<date,char> special_values_parser; 
  typedef boost::date_time::special_values_parser<date,wchar_t> wspecial_values_parser; 
  
  typedef boost::date_time::date_input_facet<date,char>    date_input_facet;
  typedef boost::date_time::date_input_facet<date,wchar_t> wdate_input_facet;

  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::date& d) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), d);
    else {
      //instantiate a custom facet for dealing with dates since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every date
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues at some later point.  With the default settings
      //for the facet the resulting format will be the same as the
      //std::time_facet settings.
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), d);
    }
    return os;
  }

  //! input operator for date
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, date& d)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;
        
        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, d);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, d);
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
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::date_duration& dd) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), dd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), dd);

    }
    return os;
  }

  //! input operator for date_duration
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, date_duration& dd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;
        
        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, dd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, dd);
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

  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::date_period& dp) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), dp);
    else {
      //instantiate a custom facet for dealing with date periods since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the local did not already exist.  Of course this will be overridden
      //if the user imbues at some later point.  With the default settings
      //for the facet the resulting format will be the same as the
      //std::time_facet settings.
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), dp);

    }
    return os;
  }

  //! input operator for date_period 
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, date_period& dp)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, dp);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, dp);
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

  /********** small gregorian types **********/
  
  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::greg_month& gm) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), gm);
    else {
      custom_date_facet* f = new custom_date_facet();//-> 10/1074199752/32 because year & day not initialized in put(...)
      //custom_date_facet* f = new custom_date_facet("%B");
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), gm);
    }
    return os;
  }

  //! input operator for greg_month
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, greg_month& m)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, m);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, m);
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


  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::greg_weekday& gw) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), gw);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), gw);
    }
    return os;
  }
 
  //! input operator for greg_weekday
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, greg_weekday& wd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, wd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, wd);
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

  //NOTE: output operator for greg_day was not necessary

  //! input operator for greg_day
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, greg_day& gd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, gd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, gd);
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

  //NOTE: output operator for greg_year was not necessary

  //! input operator for greg_year
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, greg_year& gy)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, gy);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, gy);
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

  /********** date generator types **********/
  
  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::partial_date& pd) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), pd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), pd);
    }
    return os;
  }

  //! input operator for partial_date
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, partial_date& pd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, pd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, pd);
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

  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::nth_day_of_the_week_in_month& nkd) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), nkd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), nkd);
    }
    return os;
  }

  //! input operator for nth_day_of_the_week_in_month
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, 
             nth_day_of_the_week_in_month& nday)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, nday);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, nday);
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


  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::first_day_of_the_week_in_month& fkd) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), fkd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), fkd);
    }
    return os;
  }

  //! input operator for first_day_of_the_week_in_month
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, 
             first_day_of_the_week_in_month& fkd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, fkd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, fkd);
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


  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::last_day_of_the_week_in_month& lkd) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), lkd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), lkd);
    }
    return os;
  }

  //! input operator for last_day_of_the_week_in_month
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, 
             last_day_of_the_week_in_month& lkd)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, lkd);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, lkd);
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


  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::first_day_of_the_week_after& fda) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc())) {
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), fda);
    } 
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), fda);
    }
    return os;
  }

  //! input operator for first_day_of_the_week_after
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, 
             first_day_of_the_week_after& fka)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, fka);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, fka);
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


  template <class CharT, class TraitsT>
  inline std::basic_ostream<CharT, TraitsT>&
  operator<<(std::basic_ostream<CharT, TraitsT>& os, const boost::gregorian::first_day_of_the_week_before& fdb) {
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, CharT> custom_date_facet;
    std::ostreambuf_iterator<CharT> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc())) {
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), fdb);
    }
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), fdb);
    }
    return os;
  }

  //! input operator for first_day_of_the_week_before
  template <class CharT, class Traits>
  inline
  std::basic_istream<CharT, Traits>&
  operator>>(std::basic_istream<CharT, Traits>& is, 
             first_day_of_the_week_before& fkb)
  {
    boost::io::ios_flags_saver iflags(is);
    typename std::basic_istream<CharT, Traits>::sentry strm_sentry(is, false); 
    if (strm_sentry) {
      try {
        typedef typename date_time::date_input_facet<date, CharT> date_input_facet_local;

        std::istreambuf_iterator<CharT,Traits> sit(is), str_end;
        if(std::has_facet<date_input_facet_local>(is.getloc())) {
          std::use_facet<date_input_facet_local>(is.getloc()).get(sit, str_end, is, fkb);
        }
        else {
          date_input_facet_local* f = new date_input_facet_local();
          std::locale l = std::locale(is.getloc(), f);
          is.imbue(l);
          f->get(sit, str_end, is, fkb);
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

#endif // DATE_TIME_GREGORIAN_IO_HPP__
