#ifndef DATE_TIME_DST_RULES_HPP__
#define DATE_TIME_DST_RULES_HPP__

/* Copyright (c) 2002,2003, 2007 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

/*! @file dst_rules.hpp
  Contains template class to provide static dst rule calculations
*/

#include "boost/date_time/date_generators.hpp"
#include "boost/date_time/period.hpp"
#include "boost/date_time/date_defs.hpp"
#include <stdexcept>

namespace boost {
  namespace date_time {

    enum time_is_dst_result {is_not_in_dst, is_in_dst,
                             ambiguous, invalid_time_label};


    //! Dynamic class used to caluclate dst transition information
    template<class date_type_,
             class time_duration_type_>
    class dst_calculator
    {
    public:
      typedef time_duration_type_ time_duration_type;
      typedef date_type_ date_type;

      //! Check the local time offset when on dst start day
      /*! On this dst transition, the time label between
       *  the transition boundary and the boudary + the offset
       *  are invalid times.  If before the boundary then still
       *  not in dst.
       *@param time_of_day Time offset in the day for the local time
       *@param dst_start_offset_minutes Local day offset for start of dst
       *@param dst_length_minutes Number of minutes to adjust clock forward
       *@retval status of time label w.r.t. dst
       */
      static time_is_dst_result
      process_local_dst_start_day(const time_duration_type& time_of_day,
                                  unsigned int dst_start_offset_minutes,
                                  long dst_length_minutes)
      {
        //std::cout << "here" << std::endl;
        if (time_of_day < time_duration_type(0,dst_start_offset_minutes,0)) {
          return is_not_in_dst;
        }
        long offset = dst_start_offset_minutes + dst_length_minutes;
        if (time_of_day >= time_duration_type(0,offset,0)) {
          return is_in_dst;
        }
        return invalid_time_label;
      }

      //! Check the local time offset when on the last day of dst
      /*! This is the calculation for the DST end day.  On that day times
       *  prior to the conversion time - dst_length (1 am in US) are still
       *  in dst.  Times between the above and the switch time are
       *  ambiguous.  Times after the start_offset are not in dst.
       *@param time_of_day Time offset in the day for the local time
       *@param dst_end_offset_minutes Local time of day for end of dst
       *@retval status of time label w.r.t. dst
       */
      static time_is_dst_result
      process_local_dst_end_day(const time_duration_type& time_of_day,
                                unsigned int dst_end_offset_minutes,
                                long dst_length_minutes)
      {
        //in US this will be 60 so offset in day is 1,0,0
        int offset = dst_end_offset_minutes-dst_length_minutes;
        if (time_of_day < time_duration_type(0,offset,0)) {
          return is_in_dst;
        }
        if (time_of_day >= time_duration_type(0,dst_end_offset_minutes,0)) {
          return is_not_in_dst;
        }
        return ambiguous;
      }

      //! Calculates if the given local time is dst or not
      /*! Determines if the time is really in DST or not.  Also checks for
       *  invalid and ambiguous.
       *  @param current_day The day to check for dst
       *  @param time_of_day Time offset within the day to check
       *  @param dst_start_day  Starting day of dst for the given locality
       *  @param dst_start_offset Time offset within day for dst boundary
       *  @param dst_end_day    Ending day of dst for the given locality
       *  @param dst_end_offset Time offset within day given in dst for dst boundary
       *  @param dst_length_minutes length of dst adjusment
       *  @retval The time is either ambiguous, invalid, in dst, or not in dst
       */
      static time_is_dst_result
      local_is_dst(const date_type& current_day,
                   const time_duration_type& time_of_day,
                   const date_type& dst_start_day,
                   const time_duration_type& dst_start_offset,
                   const date_type& dst_end_day,
                   const time_duration_type& dst_end_offset,
                   const time_duration_type& dst_length)
      {
        unsigned int start_minutes = static_cast<unsigned>(
          dst_start_offset.hours() * 60 + dst_start_offset.minutes());
        unsigned int end_minutes = static_cast<unsigned>(
          dst_end_offset.hours() * 60 + dst_end_offset.minutes());
        long length_minutes = static_cast<long>(
          dst_length.hours() * 60 + dst_length.minutes());

        return local_is_dst(current_day, time_of_day,
                            dst_start_day, start_minutes,
                            dst_end_day, end_minutes,
                            length_minutes);
      }

      //! Calculates if the given local time is dst or not
      /*! Determines if the time is really in DST or not.  Also checks for
       *  invalid and ambiguous.
       *  @param current_day The day to check for dst
       *  @param time_of_day Time offset within the day to check
       *  @param dst_start_day  Starting day of dst for the given locality
       *  @param dst_start_offset_minutes Offset within day for dst
       *         boundary (eg 120 for US which is 02:00:00)
       *  @param dst_end_day    Ending day of dst for the given locality
       *  @param dst_end_offset_minutes Offset within day given in dst for dst
       *         boundary (eg 120 for US which is 02:00:00)
       *  @param dst_length_minutes Length of dst adjusment (eg: 60 for US)
       *  @retval The time is either ambiguous, invalid, in dst, or not in dst
       */
      static time_is_dst_result
      local_is_dst(const date_type& current_day,
                   const time_duration_type& time_of_day,
                   const date_type& dst_start_day,
                   unsigned int dst_start_offset_minutes,
                   const date_type& dst_end_day,
                   unsigned int dst_end_offset_minutes,
                   long dst_length_minutes)
      {
        //in northern hemisphere dst is in the middle of the year
        if (dst_start_day < dst_end_day) {
          if ((current_day > dst_start_day) && (current_day < dst_end_day)) {
            return is_in_dst;
          }
          if ((current_day < dst_start_day) || (current_day > dst_end_day)) {
            return is_not_in_dst;
          }
        }
        else {//southern hemisphere dst is at begining /end of year
          if ((current_day < dst_start_day) && (current_day > dst_end_day)) {
            return is_not_in_dst;
          }
          if ((current_day > dst_start_day) || (current_day < dst_end_day)) {
            return is_in_dst;
          }
        }

        if (current_day == dst_start_day) {
          return process_local_dst_start_day(time_of_day,
                                             dst_start_offset_minutes,
                                             dst_length_minutes);
        }

        if (current_day == dst_end_day) {
          return process_local_dst_end_day(time_of_day,
                                           dst_end_offset_minutes,
                                           dst_length_minutes);
        }
        //you should never reach this statement
        return invalid_time_label;
      }

    };


    //! Compile-time configurable daylight savings time calculation engine
    /* This template provides the ability to configure a daylight savings
     * calculation at compile time covering all the cases.  Unfortunately
     * because of the number of dimensions related to daylight savings
     * calculation the number of parameters is high.  In addition, the
     * start and end transition rules are complex types that specify
     * an algorithm for calculation of the starting day and ending
     * day of daylight savings time including the month and day
     * specifications (eg: last sunday in October).
     *
     * @param date_type A type that represents dates, typically gregorian::date
     * @param time_duration_type Used for the offset in the day calculations
     * @param dst_traits A set of traits that define the rules of dst
     *        calculation.  The dst_trait must include the following:
     * start_rule_functor - Rule to calculate the starting date of a
     *                      dst transition (eg: last_kday_of_month).
     * start_day - static function that returns month of dst start for
     *             start_rule_functor
     * start_month -static function that returns day or day of week for
     *              dst start of dst
     * end_rule_functor - Rule to calculate the end of dst day.
     * end_day - static fucntion that returns end day for end_rule_functor
     * end_month - static function that returns end month for end_rule_functor
     * dst_start_offset_minutes - number of minutes from start of day to transition to dst -- 120 (or 2:00 am) is typical for the U.S. and E.U.
     * dst_start_offset_minutes - number of minutes from start of day to transition off of dst -- 180 (or 3:00 am) is typical for E.U.
     * dst_length_minutes - number of minutes that dst shifts clock
     */
    template<class date_type,
             class time_duration_type,
             class dst_traits>
    class dst_calc_engine
    {
    public:
      typedef typename date_type::year_type year_type;
      typedef typename date_type::calendar_type calendar_type;
      typedef dst_calculator<date_type, time_duration_type> dstcalc;

      //! Calculates if the given local time is dst or not
      /*! Determines if the time is really in DST or not.  Also checks for
       *  invalid and ambiguous.
       *  @retval The time is either ambiguous, invalid, in dst, or not in dst
       */
      static time_is_dst_result local_is_dst(const date_type& d,
                                             const time_duration_type& td)
      {

        year_type y = d.year();
        date_type dst_start = local_dst_start_day(y);
        date_type dst_end   = local_dst_end_day(y);
        return dstcalc::local_is_dst(d,td,
                                     dst_start,
                                     dst_traits::dst_start_offset_minutes(),
                                     dst_end,
                                     dst_traits::dst_end_offset_minutes(),
                                     dst_traits::dst_shift_length_minutes());

      }

      static bool is_dst_boundary_day(date_type d)
      {
        year_type y = d.year();
        return ((d == local_dst_start_day(y)) ||
                (d == local_dst_end_day(y)));
      }

      //! The time of day for the dst transition (eg: typically 01:00:00 or 02:00:00)
      static time_duration_type dst_offset()
      {
        return time_duration_type(0,dst_traits::dst_shift_length_minutes(),0);
      }

      static date_type local_dst_start_day(year_type year)
      {
        return dst_traits::local_dst_start_day(year);
      }

      static date_type local_dst_end_day(year_type year)
      {
        return dst_traits::local_dst_end_day(year);
      }


    };

    //! Depricated: Class to calculate dst boundaries for US time zones
    /* Use dst_calc_engine instead.
     * In 2007 US/Canada DST rules changed
     * (http://en.wikipedia.org/wiki/Energy_Policy_Act_of_2005#Change_to_daylight_saving_time).
     */
    template<class date_type_,
             class time_duration_type_,
             unsigned int dst_start_offset_minutes=120, //from start of day
             short dst_length_minutes=60>  //1 hour == 60 min in US
    class us_dst_rules
    {
    public:
      typedef time_duration_type_ time_duration_type;
      typedef date_type_ date_type;
      typedef typename date_type::year_type year_type;
      typedef typename date_type::calendar_type calendar_type;
      typedef date_time::last_kday_of_month<date_type> lkday;
      typedef date_time::first_kday_of_month<date_type> fkday;
      typedef date_time::nth_kday_of_month<date_type> nkday;
      typedef dst_calculator<date_type, time_duration_type> dstcalc;

      //! Calculates if the given local time is dst or not
      /*! Determines if the time is really in DST or not.  Also checks for
       *  invalid and ambiguous.
       *  @retval The time is either ambiguous, invalid, in dst, or not in dst
       */
      static time_is_dst_result local_is_dst(const date_type& d,
                                             const time_duration_type& td)
      {

        year_type y = d.year();
        date_type dst_start = local_dst_start_day(y);
        date_type dst_end   = local_dst_end_day(y);
        return dstcalc::local_is_dst(d,td,
                                     dst_start,dst_start_offset_minutes,
                                     dst_end, dst_start_offset_minutes,
                                     dst_length_minutes);

      }


      static bool is_dst_boundary_day(date_type d)
      {
        year_type y = d.year();
        return ((d == local_dst_start_day(y)) ||
                (d == local_dst_end_day(y)));
      }

      static date_type local_dst_start_day(year_type year)
      {
        if (year >= year_type(2007)) {
          //second sunday in march
          nkday ssim(nkday::second, Sunday, date_time::Mar);
          return ssim.get_date(year);
        } else {
          //first sunday in april
          fkday fsia(Sunday, date_time::Apr);
          return fsia.get_date(year);
        }
      }

      static date_type local_dst_end_day(year_type year)
      {
        if (year >= year_type(2007)) {
          //first sunday in november
          fkday fsin(Sunday, date_time::Nov);
          return fsin.get_date(year);
        } else {
          //last sunday in october
          lkday lsio(Sunday, date_time::Oct);
          return lsio.get_date(year);
        }
      }

      static time_duration_type dst_offset()
      {
        return time_duration_type(0,dst_length_minutes,0);
      }

     private:


    };

    //! Used for local time adjustments in places that don't use dst
    template<class date_type_, class time_duration_type_>
    class null_dst_rules
    {
    public:
      typedef time_duration_type_ time_duration_type;
      typedef date_type_ date_type;


      //! Calculates if the given local time is dst or not
      /*! @retval Always is_not_in_dst since this is for zones without dst
       */
      static time_is_dst_result local_is_dst(const date_type&,
                                             const time_duration_type&)
      {
        return is_not_in_dst;
      }

      //! Calculates if the given utc time is in dst
      static time_is_dst_result utc_is_dst(const date_type&,
                                           const time_duration_type&)
      {
        return is_not_in_dst;
      }

      static bool is_dst_boundary_day(date_type /*d*/)
      {
        return false;
      }

      static time_duration_type dst_offset()
      {
        return time_duration_type(0,0,0);
      }

    };


  } } //namespace date_time



#endif
