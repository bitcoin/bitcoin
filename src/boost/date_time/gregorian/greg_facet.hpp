#ifndef GREGORIAN_FACET_HPP___
#define GREGORIAN_FACET_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/date_formatting_locales.hpp> // sets BOOST_DATE_TIME_NO_LOCALE
#include <boost/date_time/gregorian/parsers.hpp>
#include <boost/io/ios_state.hpp>

//This file is basically commented out if locales are not supported
#ifndef BOOST_DATE_TIME_NO_LOCALE

#include <string>
#include <memory>
#include <locale>
#include <iostream>
#include <exception>

namespace boost {
namespace gregorian {
  
  //! Configuration of the output facet template
  struct BOOST_SYMBOL_VISIBLE greg_facet_config
  {
    typedef boost::gregorian::greg_month month_type;
    typedef boost::date_time::special_values special_value_enum;
    typedef boost::gregorian::months_of_year month_enum;
    typedef boost::date_time::weekdays weekday_enum;
  };

#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO)
  //! Create the base facet type for gregorian::date
  typedef boost::date_time::date_names_put<greg_facet_config> greg_base_facet;

  //! ostream operator for gregorian::date
  /*! Uses the date facet to determine various output parameters including:
   *  - string values for the month (eg: Jan, Feb, Mar) (default: English)
   *  - string values for special values (eg: not-a-date-time) (default: English)
   *  - selection of long, short strings, or numerical month representation (default: short string)
   *  - month day year order (default yyyy-mmm-dd)
   */
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const date& d)
  {
    typedef boost::date_time::date_names_put<greg_facet_config, charT> facet_def;
    typedef boost::date_time::ostream_date_formatter<date, facet_def, charT> greg_ostream_formatter;
    greg_ostream_formatter::date_put(d, os);
    return os;
  }

  //! operator<< for gregorian::greg_month typically streaming: Jan, Feb, Mar...
  /*! Uses the date facet to determine output string as well as selection of long or short strings.
   *  Default if no facet is installed is to output a 2 wide numeric value for the month
   *  eg: 01 == Jan, 02 == Feb, ... 12 == Dec.
   */
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const greg_month& m)
  {
    typedef boost::date_time::date_names_put<greg_facet_config, charT> facet_def;
    typedef boost::date_time::ostream_month_formatter<facet_def, charT> greg_month_formatter;
    std::locale locale = os.getloc();
    if (std::has_facet<facet_def>(locale)) {
      const facet_def& f = std::use_facet<facet_def>(locale);
      greg_month_formatter::format_month(m, os, f);

    }
    else { // default to numeric
      boost::io::basic_ios_fill_saver<charT> ifs(os);
      os  << std::setw(2) << std::setfill(os.widen('0')) << m.as_number();
    }

    return os;
  }

  //! operator<< for gregorian::greg_weekday typically streaming: Sun, Mon, Tue, ...
  /*! Uses the date facet to determine output string as well as selection of long or short string.
   *  Default if no facet is installed is to output a 3 char english string for the
   *  day of the week.
   */
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const greg_weekday& wd)
  {
    typedef boost::date_time::date_names_put<greg_facet_config, charT> facet_def;
    typedef boost::date_time::ostream_weekday_formatter<greg_weekday, facet_def, charT> greg_weekday_formatter;
    std::locale locale = os.getloc();
    if (std::has_facet<facet_def>(locale)) {
      const facet_def& f = std::use_facet<facet_def>(locale);
      greg_weekday_formatter::format_weekday(wd, os, f, true);
    }
    else { //default to short English string eg: Sun, Mon, Tue, Wed...
      os  << wd.as_short_string();
    }

    return os;
  }

  //! operator<< for gregorian::date_period typical output: [2002-Jan-01/2002-Jan-31]
  /*! Uses the date facet to determine output string as well as selection of long 
   *  or short string fr dates.
   *  Default if no facet is installed is to output a 3 char english string for the
   *  day of the week.
   */
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const date_period& dp)
  {
    os << '['; //TODO: facet or manipulator for periods?
    os << dp.begin();
    os << '/'; //TODO: facet or manipulator for periods?
    os << dp.last();
    os << ']'; 
    return os;
  }

  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const date_duration& dd)
  {
    //os << dd.days();
    os << dd.get_rep();
    return os;
  }

  //! operator<< for gregorian::partial_date. Output: "Jan 1"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, const partial_date& pd)
  {
    boost::io::basic_ios_fill_saver<charT> ifs(os);
    os << std::setw(2) << std::setfill(os.widen('0')) << pd.day() << ' ' 
       << pd.month().as_short_string() ; 
    return os;
  }

  //! operator<< for gregorian::nth_kday_of_month. Output: "first Mon of Jun"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, 
             const nth_kday_of_month& nkd)
  {
    os << nkd.nth_week_as_str() << ' ' 
       << nkd.day_of_week() << " of "
       << nkd.month().as_short_string() ; 
    return os;
  }

  //! operator<< for gregorian::first_kday_of_month. Output: "first Mon of Jun"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, 
             const first_kday_of_month& fkd)
  {
    os << "first " << fkd.day_of_week() << " of " 
       << fkd.month().as_short_string() ; 
    return os;
  }

  //! operator<< for gregorian::last_kday_of_month. Output: "last Mon of Jun"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, 
             const last_kday_of_month& lkd)
  {
    os << "last " << lkd.day_of_week() << " of " 
       << lkd.month().as_short_string() ; 
    return os;
  }

  //! operator<< for gregorian::first_kday_after. Output: "first Mon after"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, 
             const first_kday_after& fka)
  {
    os << fka.day_of_week() << " after"; 
    return os;
  }

  //! operator<< for gregorian::first_kday_before. Output: "first Mon before"
  template <class charT, class traits>
  inline
  std::basic_ostream<charT, traits>&
  operator<<(std::basic_ostream<charT, traits>& os, 
             const first_kday_before& fkb)
  {
    os << fkb.day_of_week() << " before"; 
    return os;
  }
#endif // USE_DATE_TIME_PRE_1_33_FACET_IO
  /**************** Input Streaming ******************/
  
#if !defined(BOOST_NO_STD_ITERATOR_TRAITS)
  //! operator>> for gregorian::date
  template<class charT>
  inline 
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is, date& d)
  {
    std::istream_iterator<std::basic_string<charT>, charT> beg(is), eos;
    d = from_stream(beg, eos);
    return is;
  }
#endif // BOOST_NO_STD_ITERATOR_TRAITS

  //! operator>> for gregorian::date_duration
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is, 
                                        date_duration& dd)
  {
    long v;
    is >> v;
    dd = date_duration(v);
    return is;
  }

  //! operator>> for gregorian::date_period
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is,
                                        date_period& dp)
  {
    std::basic_string<charT> s;
    is >> s;
    dp = date_time::from_simple_string_type<date>(s);
    return is;
  }

  //! generates a locale with the set of gregorian name-strings of type char*
  BOOST_DATE_TIME_DECL std::locale generate_locale(std::locale& loc, char type);

  //! Returns a pointer to a facet with a default set of names (English)
  /* Necessary in the event an exception is thrown from op>> for 
   * weekday or month. See comments in those functions for more info */
  BOOST_DATE_TIME_DECL boost::date_time::all_date_names_put<greg_facet_config, char>* create_facet_def(char type);

#ifndef BOOST_NO_STD_WSTRING
  //! generates a locale with the set of gregorian name-strings of type wchar_t*
  BOOST_DATE_TIME_DECL std::locale generate_locale(std::locale& loc, wchar_t type);
  //! Returns a pointer to a facet with a default set of names (English)
  /* Necessary in the event an exception is thrown from op>> for 
   * weekday or month. See comments in those functions for more info */
  BOOST_DATE_TIME_DECL boost::date_time::all_date_names_put<greg_facet_config, wchar_t>* create_facet_def(wchar_t type);
#endif // BOOST_NO_STD_WSTRING

  //! operator>> for gregorian::greg_month - throws exception if invalid month given
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is,greg_month& m) 
  {
    typedef boost::date_time::all_date_names_put<greg_facet_config, charT> facet_def;

    std::basic_string<charT> s;
    is >> s;
    
    if(!std::has_facet<facet_def>(is.getloc())) {
      std::locale loc = is.getloc();
      charT a = '\0';
      is.imbue(generate_locale(loc, a));
    }

    short num = 0;

    try{
      const facet_def& f = std::use_facet<facet_def>(is.getloc());
      num = date_time::find_match(f.get_short_month_names(), 
                                  f.get_long_month_names(), 
                                  (greg_month::max)(), s); // greg_month spans 1..12, so max returns the array size,
                                                           // which is needed by find_match
    }
    /* bad_cast will be thrown if the desired facet is not accessible
     * so we can generate the facet. This has the drawback of using english
     * names as a default. */
    catch(std::bad_cast&){
      charT a = '\0';
      
#if defined(BOOST_NO_CXX11_SMART_PTR)
      
      std::auto_ptr< const facet_def > f(create_facet_def(a));
      
#else

      std::unique_ptr< const facet_def > f(create_facet_def(a));
      
#endif
      
      num = date_time::find_match(f->get_short_month_names(), 
                                  f->get_long_month_names(), 
                                  (greg_month::max)(), s); // greg_month spans 1..12, so max returns the array size,
                                                           // which is needed by find_match
    }
    
    ++num; // months numbered 1-12
    m = greg_month(num); 

    return is;
  }

  //! operator>> for gregorian::greg_weekday  - throws exception if invalid weekday given
  template<class charT>
  inline
  std::basic_istream<charT>& operator>>(std::basic_istream<charT>& is,greg_weekday& wd) 
  {
    typedef boost::date_time::all_date_names_put<greg_facet_config, charT> facet_def;

    std::basic_string<charT> s;
    is >> s;

    if(!std::has_facet<facet_def>(is.getloc())) {
      std::locale loc = is.getloc();
      charT a = '\0';
      is.imbue(generate_locale(loc, a));
    }

    short num = 0;
    try{
      const facet_def& f = std::use_facet<facet_def>(is.getloc());
      num = date_time::find_match(f.get_short_weekday_names(), 
                                  f.get_long_weekday_names(), 
                                  (greg_weekday::max)() + 1, s); // greg_weekday spans 0..6, so increment is needed
                                                                 // to form the array size which is needed by find_match
    }
    /* bad_cast will be thrown if the desired facet is not accessible
     * so we can generate the facet. This has the drawback of using english
     * names as a default. */
    catch(std::bad_cast&){
      charT a = '\0';
      
#if defined(BOOST_NO_CXX11_SMART_PTR)

      std::auto_ptr< const facet_def > f(create_facet_def(a));
      
#else 

      std::unique_ptr< const facet_def > f(create_facet_def(a));
      
#endif
      
      num = date_time::find_match(f->get_short_weekday_names(), 
                                  f->get_long_weekday_names(), 
                                  (greg_weekday::max)() + 1, s); // greg_weekday spans 0..6, so increment is needed
                                                                 // to form the array size which is needed by find_match
    }
   
    wd = greg_weekday(num); // weekdays numbered 0-6
    return is;
  }

} } //namespace gregorian

#endif

#endif

