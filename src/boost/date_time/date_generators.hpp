#ifndef DATE_TIME_DATE_GENERATORS_HPP__
#define DATE_TIME_DATE_GENERATORS_HPP__

/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

/*! @file date_generators.hpp
  Definition and implementation of date algorithm templates
*/

#include <sstream>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/date_time/date.hpp>
#include <boost/date_time/compiler_config.hpp>

namespace boost {
namespace date_time {

  //! Base class for all generators that take a year and produce a date.
  /*! This class is a base class for polymorphic function objects that take
    a year and produce a concrete date.
    @tparam date_type The type representing a date.  This type must
    export a calender_type which defines a year_type.
  */
  template<class date_type>
  class year_based_generator
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::year_type        year_type;
    year_based_generator() {}
    virtual ~year_based_generator() {}
    virtual date_type get_date(year_type y) const = 0;
    //! Returns a string for use in a POSIX time_zone string
    virtual std::string to_string() const = 0;
  };

  //! Generates a date by applying the year to the given month and day.
  /*!
    Example usage:
    @code
    partial_date pd(1, Jan);
    partial_date pd2(70);
    date d = pd.get_date(2002); //2002-Jan-01
    date d2 = pd2.get_date(2002); //2002-Mar-10
    @endcode
    \ingroup date_alg
  */
  template<class date_type>
 class partial_date : public year_based_generator<date_type>
 {
 public:
   typedef typename date_type::calendar_type calendar_type;
   typedef typename calendar_type::day_type         day_type;
   typedef typename calendar_type::month_type       month_type;
   typedef typename calendar_type::year_type        year_type;
   typedef typename date_type::duration_type        duration_type;
   typedef typename duration_type::duration_rep     duration_rep;
   partial_date(day_type d, month_type m) :
     day_(d),
     month_(m)
   {}
   //! Partial date created from number of days into year. Range 1-366
   /*! Allowable values range from 1 to 366. 1=Jan1, 366=Dec31. If argument
    * exceeds range, partial_date will be created with closest in-range value.
    * 60 will always be Feb29, if get_date() is called with a non-leap year
    * an exception will be thrown */
   partial_date(duration_rep days) :
     day_(1), // default values
     month_(1)
   {
     date_type d1(2000,1,1);
     if(days > 1) {
       if(days > 366) // prevents wrapping
       {
         days = 366;
       }
       days = days - 1;
       duration_type dd(days);
       d1 = d1 + dd;
     }
     day_ = d1.day();
     month_ = d1.month();
   }
   //! Return a concrete date when provided with a year specific year.
   /*! Will throw an 'invalid_argument' exception if a partial_date object,
    * instantiated with Feb-29, has get_date called with a non-leap year.
    * Example:
    * @code
    * partial_date pd(29, Feb);
    * pd.get_date(2003); // throws invalid_argument exception
    * pg.get_date(2000); // returns 2000-2-29
    * @endcode
         */
   date_type get_date(year_type y) const BOOST_OVERRIDE
   {
     if((day_ == 29) && (month_ == 2) && !(calendar_type::is_leap_year(y))) {
       std::ostringstream ss;
       ss << "No Feb 29th in given year of " << y << ".";
       boost::throw_exception(std::invalid_argument(ss.str()));
     }
     return date_type(y, month_, day_);
   }
   date_type operator()(year_type y) const
   {
     return get_date(y);
     //return date_type(y, month_, day_);
   }
   bool operator==(const partial_date& rhs) const
   {
     return (month_ == rhs.month_) && (day_ == rhs.day_);
   }
   bool operator<(const partial_date& rhs) const
   {
     if (month_ < rhs.month_) return true;
     if (month_ > rhs.month_) return false;
     //months are equal
     return (day_ < rhs.day_);
   }

   // added for streaming purposes
   month_type month() const
   {
     return month_;
   }
   day_type day() const
   {
     return day_;
   }

   //! Returns string suitable for use in POSIX time zone string
   /*! Returns string formatted with up to 3 digits:
    * Jan-01 == "0"
    * Feb-29 == "58"
    * Dec-31 == "365" */
   std::string to_string() const BOOST_OVERRIDE
   {
     std::ostringstream ss;
     date_type d(2004, month_, day_);
     unsigned short c = d.day_of_year();
     c--; // numbered 0-365 while day_of_year is 1 based...
     ss << c;
     return ss.str();
   }
 private:
   day_type day_;
   month_type month_;
 };

  //! Returns nth arg as string. 1 -> "first", 2 -> "second", max is 5.
  inline const char* nth_as_str(int ele)
  {
    static const char* const _nth_as_str[] = {"out of range", "first", "second",
                                              "third", "fourth", "fifth"};
    if(ele >= 1 && ele <= 5) {
      return _nth_as_str[ele];
    }
    else {
      return _nth_as_str[0];
    }
  }

  //! Useful generator functor for finding holidays
  /*! Based on the idea in Cal. Calc. for finding holidays that are
   *  the 'first Monday of September'. When instantiated with
   *  'fifth' kday of month, the result will be the last kday of month
   *  which can be the fourth or fifth depending on the structure of
   *  the month.
   *
   *  The algorithm here basically guesses for the first
   *  day of the month.  Then finds the first day of the correct
   *  type.  That is, if the first of the month is a Tuesday
   *  and it needs Wednesday then we simply increment by a day
   *  and then we can add the length of a week until we get
   *  to the 'nth kday'.  There are probably more efficient
   *  algorithms based on using a mod 7, but this one works
   *  reasonably well for basic applications.
   *  \ingroup date_alg
   */
  template<class date_type>
  class nth_kday_of_month : public year_based_generator<date_type>
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    enum week_num {first=1, second, third, fourth, fifth};
    nth_kday_of_month(week_num week_no,
                      day_of_week_type dow,
                      month_type m) :
      month_(m),
      wn_(week_no),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type y) const BOOST_OVERRIDE
    {
      date_type d(y, month_, 1); //first day of month
      duration_type one_day(1);
      duration_type one_week(7);
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      int week = 1;
      while (week < wn_) {
        d = d + one_week;
        week++;
      }
      // remove wrapping to next month behavior
      if(d.month() != month_) {
        d = d - one_week;
      }
      return d;
    }
    // added for streaming
    month_type month() const
    {
      return month_;
    }
    week_num nth_week() const
    {
      return wn_;
    }
    day_of_week_type day_of_week() const
    {
      return dow_;
    }
    const char* nth_week_as_str() const
    {
      return nth_as_str(wn_);
    }
    //! Returns string suitable for use in POSIX time zone string
    /*! Returns a string formatted as "M4.3.0" ==> 3rd Sunday in April. */
    std::string to_string() const BOOST_OVERRIDE
    {
     std::ostringstream ss;
     ss << 'M'
       << static_cast<int>(month_) << '.'
       << static_cast<int>(wn_) << '.'
       << static_cast<int>(dow_);
     return ss.str();
    }
  private:
    month_type month_;
    week_num wn_;
    day_of_week_type dow_;
  };

  //! Useful generator functor for finding holidays and daylight savings
  /*! Similar to nth_kday_of_month, but requires less paramters
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_of_month : public year_based_generator<date_type>
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    //!Specify the first 'Sunday' in 'April' spec
    /*!@param dow The day of week, eg: Sunday, Monday, etc
     * @param m The month of the year, eg: Jan, Feb, Mar, etc
     */
    first_kday_of_month(day_of_week_type dow, month_type m) :
      month_(m),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type year) const BOOST_OVERRIDE
    {
      date_type d(year, month_,1);
      duration_type one_day(1);
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      return d;
    }
    // added for streaming
    month_type month() const
    {
      return month_;
    }
    day_of_week_type day_of_week() const
    {
      return dow_;
    }
    //! Returns string suitable for use in POSIX time zone string
    /*! Returns a string formatted as "M4.1.0" ==> 1st Sunday in April. */
    std::string to_string() const BOOST_OVERRIDE
    {
     std::ostringstream ss;
     ss << 'M'
       << static_cast<int>(month_) << '.'
       << 1 << '.'
       << static_cast<int>(dow_);
     return ss.str();
    }
  private:
    month_type month_;
    day_of_week_type dow_;
  };



  //! Calculate something like Last Sunday of January
  /*! Useful generator functor for finding holidays and daylight savings
   *  Get the last day of the month and then calculate the difference
   *  to the last previous day.
   *  @tparam date_type A date class that exports day_of_week, month_type, etc.
   *  \ingroup date_alg
   */
  template<class date_type>
  class last_kday_of_month : public year_based_generator<date_type>
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    //!Specify the date spec like last 'Sunday' in 'April' spec
    /*!@param dow The day of week, eg: Sunday, Monday, etc
     * @param m The month of the year, eg: Jan, Feb, Mar, etc
     */
    last_kday_of_month(day_of_week_type dow, month_type m) :
      month_(m),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type year) const BOOST_OVERRIDE
    {
      date_type d(year, month_, calendar_type::end_of_month_day(year,month_));
      duration_type one_day(1);
      while (dow_ != d.day_of_week()) {
        d = d - one_day;
      }
      return d;
    }
    // added for streaming
    month_type month() const
    {
      return month_;
    }
    day_of_week_type day_of_week() const
    {
      return dow_;
    }
    //! Returns string suitable for use in POSIX time zone string
    /*! Returns a string formatted as "M4.5.0" ==> last Sunday in April. */
    std::string to_string() const BOOST_OVERRIDE
    {
      std::ostringstream ss;
      ss << 'M'
         << static_cast<int>(month_) << '.'
         << 5 << '.'
         << static_cast<int>(dow_);
      return ss.str();
    }
  private:
    month_type month_;
    day_of_week_type dow_;
   };


  //! Calculate something like "First Sunday after Jan 1,2002
  /*! Date generator that takes a date and finds kday after
   *@code
     typedef boost::date_time::first_kday_after<date> firstkdayafter;
     firstkdayafter fkaf(Monday);
     fkaf.get_date(date(2002,Feb,1));
   @endcode
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_after
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type day_of_week_type;
    typedef typename date_type::duration_type        duration_type;
    first_kday_after(day_of_week_type dow) :
      dow_(dow)
    {}
    //! Return next kday given.
    date_type get_date(date_type start_day) const
    {
      duration_type one_day(1);
      date_type d = start_day + one_day;
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      return d;
    }
    // added for streaming
    day_of_week_type day_of_week() const
    {
      return dow_;
    }
  private:
    day_of_week_type dow_;
  };

  //! Calculate something like "First Sunday before Jan 1,2002
  /*! Date generator that takes a date and finds kday after
   *@code
     typedef boost::date_time::first_kday_before<date> firstkdaybefore;
     firstkdaybefore fkbf(Monday);
     fkbf.get_date(date(2002,Feb,1));
   @endcode
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_before
  {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type day_of_week_type;
    typedef typename date_type::duration_type        duration_type;
    first_kday_before(day_of_week_type dow) :
      dow_(dow)
    {}
    //! Return next kday given.
    date_type get_date(date_type start_day) const
    {
      duration_type one_day(1);
      date_type d = start_day - one_day;
      while (dow_ != d.day_of_week()) {
        d = d - one_day;
      }
      return d;
    }
    // added for streaming
    day_of_week_type day_of_week() const
    {
      return dow_;
    }
  private:
    day_of_week_type dow_;
  };

  //! Calculates the number of days until the next weekday
  /*! Calculates the number of days until the next weekday.
   * If the date given falls on a Sunday and the given weekday
   * is Tuesday the result will be 2 days */
  template<typename date_type, class weekday_type>
  inline
  typename date_type::duration_type days_until_weekday(const date_type& d, const weekday_type& wd)
  {
    typedef typename date_type::duration_type duration_type;
    duration_type wks(0);
    duration_type dd(wd.as_number() - d.day_of_week().as_number());
    if(dd.is_negative()){
      wks = duration_type(7);
    }
    return dd + wks;
  }

  //! Calculates the number of days since the previous weekday
  /*! Calculates the number of days since the previous weekday
   * If the date given falls on a Sunday and the given weekday
   * is Tuesday the result will be 5 days. The answer will be a positive
   * number because Tuesday is 5 days before Sunday, not -5 days before. */
  template<typename date_type, class weekday_type>
  inline
  typename date_type::duration_type days_before_weekday(const date_type& d, const weekday_type& wd)
  {
    typedef typename date_type::duration_type duration_type;
    duration_type wks(0);
    duration_type dd(wd.as_number() - d.day_of_week().as_number());
    if(dd.days() > 0){
      wks = duration_type(7);
    }
    // we want a number of days, not an offset. The value returned must
    // be zero or larger.
    return (-dd + wks);
  }

  //! Generates a date object representing the date of the following weekday from the given date
  /*! Generates a date object representing the date of the following
   * weekday from the given date. If the date given is 2004-May-9
   * (a Sunday) and the given weekday is Tuesday then the resulting date
   * will be 2004-May-11. */
  template<class date_type, class weekday_type>
  inline
  date_type next_weekday(const date_type& d, const weekday_type& wd)
  {
    return d + days_until_weekday(d, wd);
  }

  //! Generates a date object representing the date of the previous weekday from the given date
  /*! Generates a date object representing the date of the previous
   * weekday from the given date. If the date given is 2004-May-9
   * (a Sunday) and the given weekday is Tuesday then the resulting date
   * will be 2004-May-4. */
  template<class date_type, class weekday_type>
  inline
  date_type previous_weekday(const date_type& d, const weekday_type& wd)
  {
    return d - days_before_weekday(d, wd);
  }

} } //namespace date_time

#endif
