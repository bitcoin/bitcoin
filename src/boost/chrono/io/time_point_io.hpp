//  (C) Copyright Howard Hinnant
//  (C) Copyright 2010-2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

//===-------------------------- locale ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This code was adapted by Vicente from Howard Hinnant's experimental work
// on chrono i/o to Boost and some functions from libc++/locale to emulate the missing time_get::get()

#ifndef BOOST_CHRONO_IO_TIME_POINT_IO_HPP
#define BOOST_CHRONO_IO_TIME_POINT_IO_HPP

#include <boost/chrono/io/time_point_put.hpp>
#include <boost/chrono/io/time_point_get.hpp>
#include <boost/chrono/io/duration_io.hpp>
#include <boost/chrono/io/ios_base_state.hpp>
#include <boost/chrono/io/utility/manip_base.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/clock_string.hpp>
#include <boost/chrono/round.hpp>
#include <boost/chrono/detail/scan_keyword.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <cstring>
#include <locale>
#include <ctime>

#if  ( defined BOOST_WINDOWS && ! defined(__CYGWIN__) )  \
  || (defined(sun) || defined(__sun)) \
  || (defined __IBMCPP__) \
  || defined __ANDROID__ \
  || defined __QNXNTO__ \
  || (defined(_AIX) && defined __GNUC__)
#define  BOOST_CHRONO_INTERNAL_TIMEGM
#endif

#if (defined BOOST_WINDOWS && ! defined(__CYGWIN__)) \
  || ( (defined(sun) || defined(__sun)) && defined __GNUC__) \
  || (defined __IBMCPP__) \
  || defined __ANDROID__ \
  || (defined(_AIX) && defined __GNUC__)
#define  BOOST_CHRONO_INTERNAL_GMTIME
#endif

#define  BOOST_CHRONO_USES_INTERNAL_TIME_GET

namespace boost
{
  namespace chrono
  {
    typedef double fractional_seconds;
    namespace detail
    {


      template <class CharT, class InputIterator = std::istreambuf_iterator<CharT> >
      struct time_get
      {
        std::time_get<CharT> const &that_;
        time_get(std::time_get<CharT> const& that) : that_(that) {}

        typedef std::time_get<CharT> facet;
        typedef typename facet::iter_type iter_type;
        typedef typename facet::char_type char_type;
        typedef std::basic_string<char_type> string_type;

        static int
        get_up_to_n_digits(
            InputIterator& b, InputIterator e,
            std::ios_base::iostate& err,
            const std::ctype<CharT>& ct,
            int n)
        {
            // Precondition:  n >= 1
            if (b == e)
            {
                err |= std::ios_base::eofbit | std::ios_base::failbit;
                return 0;
            }
            // get first digit
            CharT c = *b;
            if (!ct.is(std::ctype_base::digit, c))
            {
                err |= std::ios_base::failbit;
                return 0;
            }
            int r = ct.narrow(c, 0) - '0';
            for (++b, --n; b != e && n > 0; ++b, --n)
            {
                // get next digit
                c = *b;
                if (!ct.is(std::ctype_base::digit, c))
                    return r;
                r = r * 10 + ct.narrow(c, 0) - '0';
            }
            if (b == e)
                err |= std::ios_base::eofbit;
            return r;
        }


        void get_day(
            int& d,
            iter_type& b, iter_type e,
            std::ios_base::iostate& err,
            const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && 1 <= t && t <= 31)
                d = t;
            else
                err |= std::ios_base::failbit;
        }

        void get_month(
            int& m,
            iter_type& b, iter_type e,
            std::ios_base::iostate& err,
            const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && 1 <= t && t <= 12)
                m = --t;
            else
                err |= std::ios_base::failbit;
        }


        void get_year4(int& y,
                                                      iter_type& b, iter_type e,
                                                      std::ios_base::iostate& err,
                                                      const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 4);
            if (!(err & std::ios_base::failbit))
                y = t - 1900;
        }

        void
        get_hour(int& h,
                                                     iter_type& b, iter_type e,
                                                     std::ios_base::iostate& err,
                                                     const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 23)
                h = t;
            else
                err |= std::ios_base::failbit;
        }

        void
        get_minute(int& m,
                                                       iter_type& b, iter_type e,
                                                       std::ios_base::iostate& err,
                                                       const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 59)
                m = t;
            else
                err |= std::ios_base::failbit;
        }

        void  get_second(int& s,
                                                       iter_type& b, iter_type e,
                                                       std::ios_base::iostate& err,
                                                       const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && t <= 60)
                s = t;
            else
                err |= std::ios_base::failbit;
        }

        void get_white_space(iter_type& b, iter_type e,
                                                            std::ios_base::iostate& err,
                                                            const std::ctype<char_type>& ct) const
        {
            for (; b != e && ct.is(std::ctype_base::space, *b); ++b)
                ;
            if (b == e)
                err |= std::ios_base::eofbit;
        }

        void get_12_hour(int& h,
                                                        iter_type& b, iter_type e,
                                                        std::ios_base::iostate& err,
                                                        const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 2);
            if (!(err & std::ios_base::failbit) && 1 <= t && t <= 12)
                h = t;
            else
                err |= std::ios_base::failbit;
        }

        void get_percent(iter_type& b, iter_type e,
                                                        std::ios_base::iostate& err,
                                                        const std::ctype<char_type>& ct) const
        {
            if (b == e)
            {
                err |= std::ios_base::eofbit | std::ios_base::failbit;
                return;
            }
            if (ct.narrow(*b, 0) != '%')
                err |= std::ios_base::failbit;
            else if(++b == e)
                err |= std::ios_base::eofbit;
        }

        void get_day_year_num(int& d,
                                                             iter_type& b, iter_type e,
                                                             std::ios_base::iostate& err,
                                                             const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 3);
            if (!(err & std::ios_base::failbit) && 1 <= t && t <= 366)
                d = --t;
            else
                err |= std::ios_base::failbit;
        }

        void
        get_weekday(int& w,
                                                        iter_type& b, iter_type e,
                                                        std::ios_base::iostate& err,
                                                        const std::ctype<char_type>& ct) const
        {
            int t = get_up_to_n_digits(b, e, err, ct, 1);
            if (!(err & std::ios_base::failbit) && t <= 6)
                w = t;
            else
                err |= std::ios_base::failbit;
        }
#if 0

        void
        get_am_pm(int& h,
                                                      iter_type& b, iter_type e,
                                                      std::ios_base::iostate& err,
                                                      const std::ctype<char_type>& ct) const
        {
            const string_type* ap = am_pm();
            if (ap[0].size() + ap[1].size() == 0)
            {
                err |= ios_base::failbit;
                return;
            }
            ptrdiff_t i = detail::scan_keyword(b, e, ap, ap+2, ct, err, false) - ap;
            if (i == 0 && h == 12)
                h = 0;
            else if (i == 1 && h < 12)
                h += 12;
        }

#endif

        InputIterator get(
            iter_type b, iter_type e,
            std::ios_base& iob,
            std::ios_base::iostate& err,
            std::tm* tm,
            char fmt, char) const
        {
            err = std::ios_base::goodbit;
            const std::ctype<char_type>& ct = std::use_facet<std::ctype<char_type> >(iob.getloc());

            switch (fmt)
            {
            case 'a':
            case 'A':
              {
                std::tm tm2;
                std::memset(&tm2, 0, sizeof(std::tm));
                that_.get_weekday(b, e, iob, err, &tm2);
                //tm->tm_wday = tm2.tm_wday;
              }
              break;
            case 'b':
            case 'B':
            case 'h':
              {
                std::tm tm2;
                std::memset(&tm2, 0, sizeof(std::tm));
                that_.get_monthname(b, e, iob, err, &tm2);
                //tm->tm_mon = tm2.tm_mon;
              }
              break;
//            case 'c':
//              {
//                const string_type& fm = c();
//                b = get(b, e, iob, err, tm, fm.data(), fm.data() + fm.size());
//              }
//              break;
            case 'd':
            case 'e':
              get_day(tm->tm_mday, b, e, err, ct);
              break;
            case 'D':
              {
                const char_type fm[] = {'%', 'm', '/', '%', 'd', '/', '%', 'y'};
                b = get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
              }
              break;
            case 'F':
              {
                const char_type fm[] = {'%', 'Y', '-', '%', 'm', '-', '%', 'd'};
                b = get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
              }
              break;
            case 'H':
              get_hour(tm->tm_hour, b, e, err, ct);
              break;
            case 'I':
              get_12_hour(tm->tm_hour, b, e, err, ct);
              break;
            case 'j':
              get_day_year_num(tm->tm_yday, b, e, err, ct);
              break;
            case 'm':
              get_month(tm->tm_mon, b, e, err, ct);
              break;
            case 'M':
              get_minute(tm->tm_min, b, e, err, ct);
              break;
            case 'n':
            case 't':
              get_white_space(b, e, err, ct);
              break;
//            case 'p':
//              get_am_pm(tm->tm_hour, b, e, err, ct);
//              break;
            case 'r':
              {
                const char_type fm[] = {'%', 'I', ':', '%', 'M', ':', '%', 'S', ' ', '%', 'p'};
                b = get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
              }
              break;
            case 'R':
              {
                const char_type fm[] = {'%', 'H', ':', '%', 'M'};
                b = get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
              }
              break;
            case 'S':
              get_second(tm->tm_sec, b, e, err, ct);
              break;
            case 'T':
              {
                const char_type fm[] = {'%', 'H', ':', '%', 'M', ':', '%', 'S'};
                b = get(b, e, iob, err, tm, fm, fm + sizeof(fm)/sizeof(fm[0]));
              }
              break;
            case 'w':
              {
                get_weekday(tm->tm_wday, b, e, err, ct);
              }
              break;
            case 'x':
              return that_.get_date(b, e, iob, err, tm);
//            case 'X':
//              return that_.get_time(b, e, iob, err, tm);
//              {
//                const string_type& fm = X();
//                b = that_.get(b, e, iob, err, tm, fm.data(), fm.data() + fm.size());
//              }
//              break;
//            case 'y':
//              get_year(tm->tm_year, b, e, err, ct);
                break;
            case 'Y':
              get_year4(tm->tm_year, b, e, err, ct);
              break;
            case '%':
              get_percent(b, e, err, ct);
              break;
            default:
                err |= std::ios_base::failbit;
            }
            return b;
        }


        InputIterator get(
          iter_type b, iter_type e,
          std::ios_base& iob,
          std::ios_base::iostate& err, std::tm* tm,
          const char_type* fmtb, const char_type* fmte) const
        {
          const std::ctype<char_type>& ct = std::use_facet<std::ctype<char_type> >(iob.getloc());
          err = std::ios_base::goodbit;
          while (fmtb != fmte && err == std::ios_base::goodbit)
          {
              if (b == e)
              {
                  err = std::ios_base::failbit;
                  break;
              }
              if (ct.narrow(*fmtb, 0) == '%')
              {
                  if (++fmtb == fmte)
                  {
                      err = std::ios_base::failbit;
                      break;
                  }
                  char cmd = ct.narrow(*fmtb, 0);
                  char opt = '\0';
                  if (cmd == 'E' || cmd == '0')
                  {
                      if (++fmtb == fmte)
                      {
                          err = std::ios_base::failbit;
                          break;
                      }
                      opt = cmd;
                      cmd = ct.narrow(*fmtb, 0);
                  }
                  b = get(b, e, iob, err, tm, cmd, opt);
                  ++fmtb;
              }
              else if (ct.is(std::ctype_base::space, *fmtb))
              {
                  for (++fmtb; fmtb != fmte && ct.is(std::ctype_base::space, *fmtb); ++fmtb)
                      ;
                  for (        ;    b != e    && ct.is(std::ctype_base::space, *b);    ++b)
                      ;
              }
              else if (ct.toupper(*b) == ct.toupper(*fmtb))
              {
                  ++b;
                  ++fmtb;
              }
              else
                  err = std::ios_base::failbit;
          }
          if (b == e)
              err |= std::ios_base::eofbit;
          return b;
        }

      };


      template <class CharT>
      class time_manip: public manip<time_manip<CharT> >
      {
        std::basic_string<CharT> fmt_;
        timezone tz_;
      public:

        time_manip(timezone tz, std::basic_string<CharT> fmt)
        // todo move semantics
        :
          fmt_(fmt), tz_(tz)
        {
        }

        /**
         * Change the timezone and time format ios state;
         */
        void operator()(std::ios_base &ios) const
        {
          set_time_fmt<CharT> (ios, fmt_);
          set_timezone(ios, tz_);
        }
      };

      class time_man: public manip<time_man>
      {
        timezone tz_;
      public:

        time_man(timezone tz)
        // todo move semantics
        :
          tz_(tz)
        {
        }

        /**
         * Change the timezone and time format ios state;
         */
        void operator()(std::ios_base &ios) const
        {
          //set_time_fmt<typename out_stream::char_type>(ios, "");
          set_timezone(ios, tz_);
        }
      };

    }

    template <class CharT>
    inline detail::time_manip<CharT> time_fmt(timezone tz, const CharT* fmt)
    {
      return detail::time_manip<CharT>(tz, fmt);
    }

    template <class CharT>
    inline detail::time_manip<CharT> time_fmt(timezone tz, std::basic_string<CharT> fmt)
    {
      // todo move semantics
      return detail::time_manip<CharT>(tz, fmt);
    }

    inline detail::time_man time_fmt(timezone f)
    {
      return detail::time_man(f);
    }

    /**
     * time_fmt_io_saver i/o saver.
     *
     * See Boost.IO i/o state savers for a motivating compression.
     */
    template <typename CharT = char, typename Traits = std::char_traits<CharT> >
    struct time_fmt_io_saver
    {

      //! the type of the state to restore
      //typedef std::basic_ostream<CharT, Traits> state_type;
      typedef std::ios_base state_type;

      //! the type of aspect to save
      typedef std::basic_string<CharT, Traits> aspect_type;

      /**
       * Explicit construction from an i/o stream.
       *
       * Store a reference to the i/o stream and the value of the associated @c time format .
       */
      explicit time_fmt_io_saver(state_type &s) :
        s_save_(s), a_save_(get_time_fmt<CharT>(s_save_))
      {
      }

      /**
       * Construction from an i/o stream and a @c time format  to restore.
       *
       * Stores a reference to the i/o stream and the value @c new_value to restore given as parameter.
       */
      time_fmt_io_saver(state_type &s, aspect_type new_value) :
        s_save_(s), a_save_(get_time_fmt<CharT>(s_save_))
      {
        set_time_fmt(s_save_, new_value);
      }

      /**
       * Destructor.
       *
       * Restores the i/o stream with the format to be restored.
       */
      ~time_fmt_io_saver()
      {
        this->restore();
      }

      /**
       * Restores the i/o stream with the time format to be restored.
       */
      void restore()
      {
        set_time_fmt(s_save_, a_save_);
      }
    private:
      state_type& s_save_;
      aspect_type a_save_;
    };

    /**
     * timezone_io_saver i/o saver.
     *
     * See Boost.IO i/o state savers for a motivating compression.
     */
    struct timezone_io_saver
    {

      //! the type of the state to restore
      typedef std::ios_base state_type;
      //! the type of aspect to save
      typedef timezone aspect_type;

      /**
       * Explicit construction from an i/o stream.
       *
       * Store a reference to the i/o stream and the value of the associated @c timezone.
       */
      explicit timezone_io_saver(state_type &s) :
        s_save_(s), a_save_(get_timezone(s_save_))
      {
      }

      /**
       * Construction from an i/o stream and a @c timezone to restore.
       *
       * Stores a reference to the i/o stream and the value @c new_value to restore given as parameter.
       */
      timezone_io_saver(state_type &s, aspect_type new_value) :
        s_save_(s), a_save_(get_timezone(s_save_))
      {
        set_timezone(s_save_, new_value);
      }

      /**
       * Destructor.
       *
       * Restores the i/o stream with the format to be restored.
       */
      ~timezone_io_saver()
      {
        this->restore();
      }

      /**
       * Restores the i/o stream with the timezone to be restored.
       */
      void restore()
      {
        set_timezone(s_save_, a_save_);
      }
    private:
      timezone_io_saver& operator=(timezone_io_saver const& rhs) ;

      state_type& s_save_;
      aspect_type a_save_;
    };

    /**
     *
     * @param os
     * @param tp
     * @Effects Behaves as a formatted output function. After constructing a @c sentry object, if the @ sentry
     * converts to true, calls to @c facet.put(os,os,os.fill(),tp) where @c facet is the @c time_point_put<CharT>
     * facet associated to @c os or a new created instance of the default @c time_point_put<CharT> facet.
     * @return @c os.
     */
    template <class CharT, class Traits, class Clock, class Duration>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const time_point<Clock, Duration>& tp)
    {

      bool failed = false;
      BOOST_TRY
      {
        std::ios_base::iostate err = std::ios_base::goodbit;
        BOOST_TRY
        {
          typename std::basic_ostream<CharT, Traits>::sentry opfx(os);
          if (bool(opfx))
          {
            if (!std::has_facet<time_point_put<CharT> >(os.getloc()))
            {
              if (time_point_put<CharT> ().put(os, os, os.fill(), tp) .failed())
              {
                err = std::ios_base::badbit;
              }
            }
            else
            {
              if (std::use_facet<time_point_put<CharT> >(os.getloc()) .put(os, os, os.fill(), tp).failed())
              {
                err = std::ios_base::badbit;
              }
            }
            os.width(0);
          }
        }
        BOOST_CATCH (...)
        {
          bool flag = false;
          BOOST_TRY
          {
            os.setstate(std::ios_base::failbit);
          }
          BOOST_CATCH (std::ios_base::failure )
          {
            flag = true;
          }
          BOOST_CATCH_END
          if (flag) throw;
        }
        BOOST_CATCH_END
        if (err) os.setstate(err);
        return os;
      }
      BOOST_CATCH (...)
      {
        failed = true;
      }
      BOOST_CATCH_END
      if (failed) os.setstate(std::ios_base::failbit | std::ios_base::badbit);
      return os;
    }

    template <class CharT, class Traits, class Clock, class Duration>
    std::basic_istream<CharT, Traits>&
    operator>>(std::basic_istream<CharT, Traits>& is, time_point<Clock, Duration>& tp)
    {
      std::ios_base::iostate err = std::ios_base::goodbit;

      BOOST_TRY
      {
        typename std::basic_istream<CharT, Traits>::sentry ipfx(is);
        if (bool(ipfx))
        {
          if (!std::has_facet<time_point_get<CharT> >(is.getloc()))
          {
            time_point_get<CharT> ().get(is, std::istreambuf_iterator<CharT, Traits>(), is, err, tp);
          }
          else
          {
            std::use_facet<time_point_get<CharT> >(is.getloc()).get(is, std::istreambuf_iterator<CharT, Traits>(), is,
                err, tp);
          }
        }
      }
      BOOST_CATCH (...)
      {
        bool flag = false;
        BOOST_TRY
        {
          is.setstate(std::ios_base::failbit);
        }
        BOOST_CATCH (std::ios_base::failure )
        {
          flag = true;
        }
        BOOST_CATCH_END
        if (flag) throw;
      }
      BOOST_CATCH_END
      if (err) is.setstate(err);
      return is;
    }


    namespace detail
    {

//#if defined BOOST_CHRONO_INTERNAL_TIMEGM

    inline int32_t is_leap(int32_t year)
    {
      if(year % 400 == 0)
      return 1;
      if(year % 100 == 0)
      return 0;
      if(year % 4 == 0)
      return 1;
      return 0;
    }
    inline int32_t days_from_0(int32_t year)
    {
      year--;
      return 365 * year + (year / 400) - (year/100) + (year / 4);
    }
    inline int32_t days_from_1970(int32_t year)
    {
      static const int32_t days_from_0_to_1970 = days_from_0(1970);
      return days_from_0(year) - days_from_0_to_1970;
    }
    inline int32_t days_from_1jan(int32_t year,int32_t month,int32_t day)
    {
      static const int32_t days[2][12] =
      {
        { 0,31,59,90,120,151,181,212,243,273,304,334},
        { 0,31,60,91,121,152,182,213,244,274,305,335}
      };

      return days[is_leap(year)][month-1] + day - 1;
    }

    inline time_t internal_timegm(std::tm const *t)
    {
      int year = t->tm_year + 1900;
      int month = t->tm_mon;
      if(month > 11)
      {
        year += month/12;
        month %= 12;
      }
      else if(month < 0)
      {
        int years_diff = (-month + 11)/12;
        year -= years_diff;
        month+=12 * years_diff;
      }
      month++;
      int day = t->tm_mday;
      int day_of_year = days_from_1jan(year,month,day);
      int days_since_epoch = days_from_1970(year) + day_of_year ;

      time_t seconds_in_day = 3600 * 24;
      time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

      return result;
    }
//#endif

    /**
    * from_ymd could be made more efficient by using a table
    * day_count_table indexed by the y%400.
    * This table could contain the day_count
    * by*365 + by/4 - by/100 + by/400
    *
    * from_ymd = (by/400)*days_by_400_years+day_count_table[by%400] +
    * days_in_year_before[is_leap_table[by%400]][m-1] + d;
    */
    inline unsigned days_before_years(int32_t y)
   {
     return y * 365 + y / 4 - y / 100 + y / 400;
   }

    // Returns year/month/day triple in civil calendar
    // Preconditions:  z is number of days since 1970-01-01 and is in the range:
    //                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-719468].
    template <class Int>
    //constexpr
    void
    inline civil_from_days(Int z, Int& y, unsigned& m, unsigned& d) BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT_MSG(std::numeric_limits<unsigned>::digits >= 18,
                 "This algorithm has not been ported to a 16 bit unsigned integer");
        BOOST_STATIC_ASSERT_MSG(std::numeric_limits<Int>::digits >= 20,
                 "This algorithm has not been ported to a 16 bit signed integer");
        z += 719468;
        const Int era = (z >= 0 ? z : z - 146096) / 146097;
        const unsigned doe = static_cast<unsigned>(z - era * 146097);          // [0, 146096]
        const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;  // [0, 399]
        y = static_cast<Int>(yoe) + era * 400;
        const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);                // [0, 365]
        const unsigned mp = (5*doy + 2)/153;                                   // [0, 11]
        d = doy - (153*mp+2)/5 + 1;                             // [1, 31]
        m = mp + (mp < 10 ? 3 : -9);                            // [1, 12]
        y += (m <= 2);
        --m;
    }
   inline std::tm * internal_gmtime(std::time_t const* t, std::tm *tm)
   {
      if (t==0) return 0;
      if (tm==0) return 0;

#if 0
      static  const unsigned char
        day_of_year_month[2][366] =
           {
           { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 },

           { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12

           } };

      static const int32_t days_in_year_before[2][13] =
     {
       { -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 },
       { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }
     };
#endif

     const time_t seconds_in_day = 3600 * 24;
     int32_t days_since_epoch = static_cast<int32_t>(*t / seconds_in_day);
     int32_t hms = static_cast<int32_t>(*t - seconds_in_day*days_since_epoch);
     if (hms < 0) {
       days_since_epoch-=1;
       hms = seconds_in_day+hms;
     }

#if 0
     int32_t x = days_since_epoch;
     int32_t y = static_cast<int32_t> (static_cast<long long> (x + 2) * 400
           / 146097);
       const int32_t ym1 = y - 1;
       int32_t doy = x - days_before_years(y);
       const int32_t doy1 = x - days_before_years(ym1);
       const int32_t N = std::numeric_limits<int>::digits - 1;
       const int32_t mask1 = doy >> N; // arithmetic rshift - not portable - but nearly universal
       const int32_t mask0 = ~mask1;
       doy = (doy & mask0) | (doy1 & mask1);
       y = (y & mask0) | (ym1 & mask1);
       //y -= 32767 + 2;
       y += 70;
       tm->tm_year=y;
       const int32_t leap = is_leap(y);
       tm->tm_mon = day_of_year_month[leap][doy]-1;
       tm->tm_mday = doy - days_in_year_before[leap][tm->tm_mon] ;
#else
       int32_t y;
       unsigned m, d;
       civil_from_days(days_since_epoch, y, m, d);
       tm->tm_year=y-1900; tm->tm_mon=m; tm->tm_mday=d;
#endif

     tm->tm_hour = hms / 3600;
     const int ms = hms % 3600;
     tm->tm_min = ms / 60;
     tm->tm_sec = ms % 60;

     tm->tm_isdst = -1;
     (void)mktime(tm);
     return tm;
   }

    } // detail
#ifndef BOOST_CHRONO_NO_UTC_TIMEPOINT

#if defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT

    template <class CharT, class Traits, class Duration>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os, const time_point<system_clock, Duration>& tp)
    {
      typename std::basic_ostream<CharT, Traits>::sentry ok(os);
      if (bool(ok))
      {
        bool failed = false;
        BOOST_TRY
        {
          const CharT* pb = 0; //nullptr;
          const CharT* pe = pb;
          std::basic_string<CharT> fmt = get_time_fmt<CharT> (os);
          pb = fmt.data();
          pe = pb + fmt.size();

          timezone tz = get_timezone(os);
          std::locale loc = os.getloc();
          time_t t = system_clock::to_time_t(time_point_cast<system_clock::duration>(tp));
          std::tm tm;
          std::memset(&tm, 0, sizeof(std::tm));
          if (tz == timezone::local)
          {
#if defined BOOST_WINDOWS && ! defined(__CYGWIN__)
#if BOOST_MSVC < 1400  // localtime_s doesn't exist in vc7.1
            std::tm *tmp = 0;
            if ((tmp=localtime(&t)) == 0)
              failed = true;
            else
              tm =*tmp;
# else
            if (localtime_s(&tm, &t) != 0) failed = true;
# endif
#else
            if (localtime_r(&t, &tm) == 0) failed = true;
#endif
          }
          else
          {
#if defined BOOST_CHRONO_INTERNAL_GMTIME
            if (detail::internal_gmtime(&t, &tm) == 0) failed = true;

#elif defined BOOST_WINDOWS && ! defined(__CYGWIN__)
            std::tm *tmp = 0;
            if((tmp = gmtime(&t)) == 0)
              failed = true;
            else
              tm = *tmp;
#else
            if (gmtime_r(&t, &tm) == 0) failed = true;
            tm.tm_isdst = -1;
            (void)mktime(&tm);

#endif

          }
          if (!failed)
          {
            const std::time_put<CharT>& tpf = std::use_facet<std::time_put<CharT> >(loc);
            if (pb == pe)
            {
              CharT pattern[] =
              { '%', 'Y', '-', '%', 'm', '-', '%', 'd', ' ', '%', 'H', ':', '%', 'M', ':' };
              pb = pattern;
              pe = pb + sizeof (pattern) / sizeof(CharT);
              failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
              if (!failed)
              {
                duration<fractional_seconds> d = tp - system_clock::from_time_t(t) + seconds(tm.tm_sec);
                if (d.count() < 10) os << CharT('0');
                //if (! os.good()) {
                //  throw "exception";
                //}
                std::ios::fmtflags flgs = os.flags();
                os.setf(std::ios::fixed, std::ios::floatfield);
                //if (! os.good()) {
                //throw "exception";
                //}
                os.precision(9);
                os << d.count();
                //if (! os.good()) {
                //throw "exception";
                //}
                os.flags(flgs);
                if (tz == timezone::local)
                {
                  CharT sub_pattern[] =
                  { ' ', '%', 'z' };
                  pb = sub_pattern;
                  pe = pb + +sizeof (sub_pattern) / sizeof(CharT);
                  failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
                }
                else
                {
                  CharT sub_pattern[] =
                  { ' ', '+', '0', '0', '0', '0', 0 };
                  os << sub_pattern;
                }
              }
            }
            else
            {
              failed = tpf.put(os, os, os.fill(), &tm, pb, pe).failed();
            }
          }
        }
        BOOST_CATCH (...)
        {
          failed = true;
        }
        BOOST_CATCH_END
        if (failed)
        {
          os.setstate(std::ios_base::failbit | std::ios_base::badbit);
        }
      }
      return os;
    }
#endif

    namespace detail
    {

      template <class CharT, class InputIterator>
      minutes extract_z(InputIterator& b, InputIterator e, std::ios_base::iostate& err, const std::ctype<CharT>& ct)
      {
        int min = 0;
        if (b != e)
        {
          char cn = ct.narrow(*b, 0);
          if (cn != '+' && cn != '-')
          {
            err |= std::ios_base::failbit;
            return minutes(0);
          }
          int sn = cn == '-' ? -1 : 1;
          int hr = 0;
          for (int i = 0; i < 2; ++i)
          {
            if (++b == e)
            {
              err |= std::ios_base::eofbit | std::ios_base::failbit;
              return minutes(0);
            }
            cn = ct.narrow(*b, 0);
            if (! ('0' <= cn && cn <= '9'))
            {
              err |= std::ios_base::failbit;
              return minutes(0);
            }
            hr = hr * 10 + cn - '0';
          }
          for (int i = 0; i < 2; ++i)
          {
            if (++b == e)
            {
              err |= std::ios_base::eofbit | std::ios_base::failbit;
              return minutes(0);
            }
            cn = ct.narrow(*b, 0);
            if (! ('0' <= cn && cn <= '9'))
            {
              err |= std::ios_base::failbit;
              return minutes(0);
            }
            min = min * 10 + cn - '0';
          }
          if (++b == e) {
            err |= std::ios_base::eofbit;
          }
          min += hr * 60;
          min *= sn;
        }
        else
        {
          err |= std::ios_base::eofbit | std::ios_base::failbit;
        }
        return minutes(min);
      }

    } // detail

#if defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT

    template <class CharT, class Traits, class Duration>
    std::basic_istream<CharT, Traits>&
    operator>>(std::basic_istream<CharT, Traits>& is, time_point<system_clock, Duration>& tp)
    {
      typename std::basic_istream<CharT, Traits>::sentry ok(is);
      if (bool(ok))
      {
        std::ios_base::iostate err = std::ios_base::goodbit;
        BOOST_TRY
        {
          const CharT* pb = 0; //nullptr;
          const CharT* pe = pb;
          std::basic_string<CharT> fmt = get_time_fmt<CharT> (is);
          pb = fmt.data();
          pe = pb + fmt.size();

          timezone tz = get_timezone(is);
          std::locale loc = is.getloc();
          const std::time_get<CharT>& tg = std::use_facet<std::time_get<CharT> >(loc);
          const std::ctype<CharT>& ct = std::use_facet<std::ctype<CharT> >(loc);
          tm tm; // {0}
          std::memset(&tm, 0, sizeof(std::tm));

          typedef std::istreambuf_iterator<CharT, Traits> It;
          if (pb == pe)
          {
            CharT pattern[] =
            { '%', 'Y', '-', '%', 'm', '-', '%', 'd', ' ', '%', 'H', ':', '%', 'M', ':' };
            pb = pattern;
            pe = pb + sizeof (pattern) / sizeof(CharT);

#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
            const detail::time_get<CharT>& dtg(tg);
            dtg.get(is, 0, is, err, &tm, pb, pe);
#else
            tg.get(is, 0, is, err, &tm, pb, pe);
#endif
            if (err & std::ios_base::failbit) goto exit;
            fractional_seconds sec;
            CharT c = CharT();
            std::ios::fmtflags flgs = is.flags();
            is.setf(std::ios::fixed, std::ios::floatfield);
            is.precision(9);
            is >> sec;
            is.flags(flgs);
            if (is.fail())
            {
              err |= std::ios_base::failbit;
              goto exit;
            }
            It i(is);
            It eof;
            c = *i;
            if (++i == eof || c != ' ')
            {
              err |= std::ios_base::failbit;
              goto exit;
            }
            minutes min = detail::extract_z(i, eof, err, ct);

            if (err & std::ios_base::failbit) goto exit;
            time_t t;

#if defined BOOST_CHRONO_INTERNAL_TIMEGM
            t = detail::internal_timegm(&tm);
#else
            t = timegm(&tm);
#endif
            tp = time_point_cast<Duration>(
                system_clock::from_time_t(t) - min + round<system_clock::duration> (duration<fractional_seconds> (sec))
                );
          }
          else
          {
            const CharT z[2] =
            { '%', 'z' };
            const CharT* fz = std::search(pb, pe, z, z + 2);
#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
            const detail::time_get<CharT>& dtg(tg);
            dtg.get(is, 0, is, err, &tm, pb, fz);
#else
            tg.get(is, 0, is, err, &tm, pb, fz);
#endif
            minutes minu(0);
            if (fz != pe)
            {
              if (err != std::ios_base::goodbit)
              {
                err |= std::ios_base::failbit;
                goto exit;
              }
              It i(is);
              It eof;
              minu = detail::extract_z(i, eof, err, ct);
              if (err & std::ios_base::failbit) goto exit;
              if (fz + 2 != pe)
              {
                if (err != std::ios_base::goodbit)
                {
                  err |= std::ios_base::failbit;
                  goto exit;
                }
#if defined BOOST_CHRONO_USES_INTERNAL_TIME_GET
                const detail::time_get<CharT>& dtg(tg);
                dtg.get(is, 0, is, err, &tm, fz + 2, pe);
#else
                tg.get(is, 0, is, err, &tm, fz + 2, pe);
#endif
                if (err & std::ios_base::failbit) goto exit;
              }
            }
            tm.tm_isdst = -1;
            time_t t;
            if (tz == timezone::utc || fz != pe)
            {
#if defined BOOST_CHRONO_INTERNAL_TIMEGM
              t = detail::internal_timegm(&tm);
#else
              t = timegm(&tm);
#endif
            }
            else
            {
              t = mktime(&tm);
            }
            tp = time_point_cast<Duration>(
                system_clock::from_time_t(t) - minu
                );
          }
        }
        BOOST_CATCH (...)
        {
          err |= std::ios_base::badbit | std::ios_base::failbit;
        }
        BOOST_CATCH_END
        exit: is.setstate(err);
      }
      return is;
    }

#endif
#endif //UTC
  } // chrono

}

#endif  // header
