/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


namespace boost {
namespace date_time {
  //! Return the day of the week (0==Sunday, 1==Monday, etc)
  /*! Converts a year-month-day into a day of the week number
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  unsigned short
  gregorian_calendar_base<ymd_type_,date_int_type_>::day_of_week(const ymd_type& ymd) {
    unsigned short a = static_cast<unsigned short>((14-ymd.month)/12);
    unsigned short y = static_cast<unsigned short>(ymd.year - a);
    unsigned short m = static_cast<unsigned short>(ymd.month + 12*a - 2);
    unsigned short d = static_cast<unsigned short>((ymd.day + y + (y/4) - (y/100) + (y/400) + (31*m)/12) % 7);
    //std::cout << year << "-" << month << "-" << day << " is day: " << d << "\n";
    return d;
  }

  //!Return the iso week number for the date
  /*!Implements the rules associated with the iso 8601 week number.
    Basically the rule is that Week 1 of the year is the week that contains
    January 4th or the week that contains the first Thursday in January.
    Reference for this algorithm is the Calendar FAQ by Claus Tondering, April 2000.
  */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  int
  gregorian_calendar_base<ymd_type_,date_int_type_>::week_number(const ymd_type& ymd) {
    unsigned long julianbegin = julian_day_number(ymd_type(ymd.year,1,1));
    unsigned long juliantoday = julian_day_number(ymd);
    unsigned long day = (julianbegin + 3) % 7;
    unsigned long week = (juliantoday + day - julianbegin + 4)/7;

    if ((week >= 1) && (week <= 52)) {
      return static_cast<int>(week);
    }

    if (week == 53) {
      if((day==6) ||(day == 5 && is_leap_year(ymd.year))) {
        return static_cast<int>(week); //under these circumstances week == 53.
      } else {
        return 1; //monday - wednesday is in week 1 of next year
      }
    }
    //if the week is not in current year recalculate using the previous year as the beginning year
    else if (week == 0) {
      julianbegin = julian_day_number(ymd_type(static_cast<unsigned short>(ymd.year-1),1,1));
      juliantoday = julian_day_number(ymd);
      day = (julianbegin + 3) % 7;
      week = (juliantoday + day - julianbegin + 4)/7;
      return static_cast<int>(week);
    }

    return static_cast<int>(week);  //not reachable -- well except if day == 5 and is_leap_year != true

  }

  //! Convert a ymd_type into a day number
  /*! The day number is an absolute number of days since the start of count
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  date_int_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::day_number(const ymd_type& ymd)
  {
    unsigned short a = static_cast<unsigned short>((14-ymd.month)/12);
    unsigned short y = static_cast<unsigned short>(ymd.year + 4800 - a);
    unsigned short m = static_cast<unsigned short>(ymd.month + 12*a - 3);
    unsigned long  d = ymd.day + ((153*m + 2)/5) + 365*y + (y/4) - (y/100) + (y/400) - 32045;
    return static_cast<date_int_type>(d);
  }

  //! Convert a year-month-day into the julian day number
  /*! Since this implementation uses julian day internally, this is the same as the day_number.
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  date_int_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::julian_day_number(const ymd_type& ymd)
  {
    return day_number(ymd);
  }

  //! Convert year-month-day into a modified julian day number
  /*! The day number is an absolute number of days.
   *  MJD 0 thus started on 17 Nov 1858(Gregorian) at 00:00:00 UTC
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  date_int_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::modjulian_day_number(const ymd_type& ymd)
  {
    return julian_day_number(ymd)-2400001; //prerounded
  }

  //! Change a day number into a year-month-day
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  ymd_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::from_day_number(date_int_type dayNumber)
  {
    date_int_type a = dayNumber + 32044;
    date_int_type b = (4*a + 3)/146097;
    date_int_type c = a-((146097*b)/4);
    date_int_type d = (4*c + 3)/1461;
    date_int_type e = c - (1461*d)/4;
    date_int_type m = (5*e + 2)/153;
    unsigned short day = static_cast<unsigned short>(e - ((153*m + 2)/5) + 1);
    unsigned short month = static_cast<unsigned short>(m + 3 - 12 * (m/10));
    year_type year = static_cast<unsigned short>(100*b + d - 4800 + (m/10));
    //std::cout << year << "-" << month << "-" << day << "\n";

    return ymd_type(static_cast<unsigned short>(year),month,day);
  }

  //! Change a day number into a year-month-day
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  ymd_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::from_julian_day_number(date_int_type dayNumber)
  {
    date_int_type a = dayNumber + 32044;
    date_int_type b = (4*a+3)/146097;
    date_int_type c = a - ((146097*b)/4);
    date_int_type d = (4*c + 3)/1461;
    date_int_type e = c - ((1461*d)/4);
    date_int_type m = (5*e + 2)/153;
    unsigned short day = static_cast<unsigned short>(e - ((153*m + 2)/5) + 1);
    unsigned short month = static_cast<unsigned short>(m + 3 - 12 * (m/10));
    year_type year = static_cast<year_type>(100*b + d - 4800 + (m/10));
    //std::cout << year << "-" << month << "-" << day << "\n";

    return ymd_type(year,month,day);
  }

  //! Change a modified julian day number into a year-month-day
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  ymd_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::from_modjulian_day_number(date_int_type dayNumber) {
    date_int_type jd = dayNumber + 2400001; //is 2400000.5 prerounded
    return from_julian_day_number(jd);
  }

  //! Determine if the provided year is a leap year
  /*!
   *@return true if year is a leap year, false otherwise
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  bool
  gregorian_calendar_base<ymd_type_,date_int_type_>::is_leap_year(year_type year)
  {
    //divisible by 4, not if divisible by 100, but true if divisible by 400
    return (!(year % 4))  && ((year % 100) || (!(year % 400)));
  }

  //! Calculate the last day of the month
  /*! Find the day which is the end of the month given year and month
   *  No error checking is performed.
   */
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  unsigned short
  gregorian_calendar_base<ymd_type_,date_int_type_>::end_of_month_day(year_type year,
                                                                      month_type month)
  {
    switch (month) {
    case 2:
      if (is_leap_year(year)) {
        return 29;
      } else {
        return 28;
      }
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    default:
      return 31;
    }
  }

  //! Provide the ymd_type specification for the calendar start
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  ymd_type_
  gregorian_calendar_base<ymd_type_,date_int_type_>::epoch()
  {
    return ymd_type(1400,1,1);
  }

  //! Defines length of a week for week calculations
  template<typename ymd_type_, typename date_int_type_>
  BOOST_CXX14_CONSTEXPR
  inline
  unsigned short
  gregorian_calendar_base<ymd_type_,date_int_type_>::days_in_week()
  {
    return 7;
  }


} } //namespace gregorian
