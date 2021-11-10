
#ifndef _DATE_TIME_FACET__HPP__
#define _DATE_TIME_FACET__HPP__

/* Copyright (c) 2004-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author:  Martin Andrian, Jeff Garland, Bart Garst
 * $Date$
 */

#include <cctype>
#include <exception>
#include <iomanip>
#include <iterator> // i/ostreambuf_iterator
#include <locale>
#include <limits>
#include <sstream>
#include <string>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/date_facet.hpp>
#include <boost/date_time/string_convert.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/time_resolution_traits.hpp> // absolute_value

namespace boost {
namespace date_time {

  template <class CharT>
  struct time_formats {
    public:
      typedef CharT char_type;
      static const char_type fractional_seconds_format[3];               // f
      static const char_type fractional_seconds_or_none_format[3];       // F
      static const char_type seconds_with_fractional_seconds_format[3];  // s
      static const char_type seconds_format[3];                          // S
      static const char_type hours_format[3];                            // H
      static const char_type unrestricted_hours_format[3];               // O
      static const char_type full_24_hour_time_format[3];                // T
      static const char_type full_24_hour_time_expanded_format[9];       // HH:MM:SS
      static const char_type short_24_hour_time_format[3];               // R
      static const char_type short_24_hour_time_expanded_format[6];      // HH:MM
      static const char_type standard_format[9];                         // x X
      static const char_type zone_abbrev_format[3];                      // z
      static const char_type zone_name_format[3];                        // Z
      static const char_type zone_iso_format[3];                         // q
      static const char_type zone_iso_extended_format[3];                // Q
      static const char_type posix_zone_string_format[4];                // ZP
      static const char_type duration_sign_negative_only[3];             // -
      static const char_type duration_sign_always[3];                    // +
      static const char_type duration_seperator[2];
      static const char_type negative_sign[2];                           //-
      static const char_type positive_sign[2];                           //+
      static const char_type iso_time_format_specifier[18];
      static const char_type iso_time_format_extended_specifier[22];
      //default ptime format is YYYY-Mon-DD HH:MM:SS[.fff...][ zzz]
      static const char_type default_time_format[23];
      // default_time_input_format uses a posix_time_zone_string instead of a time zone abbrev
      static const char_type default_time_input_format[24];
      //default time_duration format is HH:MM:SS[.fff...]
      static const char_type default_time_duration_format[11];
  };

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::fractional_seconds_format[3] = {'%','f'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::fractional_seconds_or_none_format[3] = {'%','F'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::seconds_with_fractional_seconds_format[3] = {'%','s'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::seconds_format[3] =  {'%','S'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::hours_format[3] =  {'%','H'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::unrestricted_hours_format[3] =  {'%','O'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::full_24_hour_time_format[3] =  {'%','T'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::full_24_hour_time_expanded_format[9] =
  {'%','H',':','%','M',':','%','S'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::short_24_hour_time_format[3] =  {'%','R'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::short_24_hour_time_expanded_format[6] =
  {'%','H',':','%','M'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  //time_formats<CharT>::standard_format[5] =  {'%','c',' ','%','z'};
  time_formats<CharT>::standard_format[9] =  {'%','x',' ','%','X',' ','%','z'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::zone_abbrev_format[3] =  {'%','z'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::zone_name_format[3] =  {'%','Z'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::zone_iso_format[3] =  {'%','q'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::zone_iso_extended_format[3] ={'%','Q'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::posix_zone_string_format[4] ={'%','Z','P'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::duration_seperator[2] =  {':'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::negative_sign[2] =  {'-'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::positive_sign[2] =  {'+'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::duration_sign_negative_only[3] ={'%','-'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::duration_sign_always[3] ={'%','+'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::iso_time_format_specifier[18] =
    {'%', 'Y', '%', 'm', '%', 'd', 'T',
     '%', 'H', '%', 'M', '%', 'S', '%', 'F', '%','q' };

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::iso_time_format_extended_specifier[22] =
    {'%', 'Y', '-', '%', 'm', '-', '%', 'd', ' ',
     '%', 'H', ':', '%', 'M', ':', '%', 'S', '%', 'F','%','Q'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::default_time_format[23] =
    {'%','Y','-','%','b','-','%','d',' ',
      '%','H',':','%','M',':','%','S','%','F',' ','%','z'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::default_time_input_format[24] =
    {'%','Y','-','%','b','-','%','d',' ',
      '%','H',':','%','M',':','%','S','%','F',' ','%','Z','P'};

  template <class CharT>
  const typename time_formats<CharT>::char_type
  time_formats<CharT>::default_time_duration_format[11] =
    {'%','O',':','%','M',':','%','S','%','F'};



  /*! Facet used for format-based output of time types
   * This class provides for the use of format strings to output times.  In addition
   * to the flags for formatting date elements, the following are the allowed format flags:
   *  - %x %X => default format - enables addition of more flags to default (ie. "%x %X %z")
   *  - %f => fractional seconds ".123456"
   *  - %F => fractional seconds or none: like frac sec but empty if frac sec == 0
   *  - %s => seconds w/ fractional sec "02.123" (this is the same as "%S%f)
   *  - %S => seconds "02"
   *  - %z => abbreviated time zone "EDT"
   *  - %Z => full time zone name "Eastern Daylight Time"
   */
  template <class time_type,
            class CharT,
            class OutItrT = std::ostreambuf_iterator<CharT, std::char_traits<CharT> > >
  class BOOST_SYMBOL_VISIBLE time_facet :
    public boost::date_time::date_facet<typename time_type::date_type , CharT, OutItrT> {
    typedef time_formats< CharT > formats_type;
   public:
    typedef typename time_type::date_type date_type;
    typedef typename time_type::time_duration_type time_duration_type;
    typedef boost::date_time::period<time_type,time_duration_type> period_type;
    typedef boost::date_time::date_facet<typename time_type::date_type, CharT, OutItrT> base_type;
    typedef typename base_type::string_type string_type;
    typedef typename base_type::char_type   char_type;
    typedef typename base_type::period_formatter_type period_formatter_type;
    typedef typename base_type::special_values_formatter_type special_values_formatter_type;
    typedef typename base_type::date_gen_formatter_type date_gen_formatter_type;
    static const char_type* fractional_seconds_format;                // %f
    static const char_type* fractional_seconds_or_none_format;        // %F
    static const char_type* seconds_with_fractional_seconds_format;   // %s
    static const char_type* seconds_format;                           // %S
    static const char_type* hours_format;                             // %H
    static const char_type* unrestricted_hours_format;                // %O
    static const char_type* standard_format;                          // %x X
    static const char_type* zone_abbrev_format;                       // %z
    static const char_type* zone_name_format;                         // %Z
    static const char_type* zone_iso_format;                          // %q
    static const char_type* zone_iso_extended_format;                 // %Q
    static const char_type* posix_zone_string_format;                 // %ZP
    static const char_type* duration_seperator;
    static const char_type* duration_sign_always;                     // %+
    static const char_type* duration_sign_negative_only;              // %-
    static const char_type* negative_sign;                            //-
    static const char_type* positive_sign;                            //+
    static const char_type* iso_time_format_specifier;
    static const char_type* iso_time_format_extended_specifier;

    //default ptime format is YYYY-Mon-DD HH:MM:SS[.fff...][ zzz]
    static const char_type* default_time_format;
    //default time_duration format is HH:MM:SS[.fff...]
    static const char_type* default_time_duration_format;
    static std::locale::id id;

#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
      std::locale::id& __get_id (void) const { return id; }
#endif

    //! sets default formats for ptime, local_date_time, and time_duration
    explicit time_facet(::size_t ref_arg = 0)
      : base_type(default_time_format, period_formatter_type(), special_values_formatter_type(), date_gen_formatter_type(), ref_arg),
        m_time_duration_format(string_type(duration_sign_negative_only) + default_time_duration_format)
    {}

    //! Construct the facet with an explicitly specified format
    explicit time_facet(const char_type* format_arg,
                        period_formatter_type period_formatter_arg = period_formatter_type(),
                        const special_values_formatter_type& special_value_formatter = special_values_formatter_type(),
                        date_gen_formatter_type dg_formatter = date_gen_formatter_type(),
                         ::size_t ref_arg = 0)
      : base_type(format_arg,
                  period_formatter_arg,
                  special_value_formatter,
                  dg_formatter,
                  ref_arg),
        m_time_duration_format(string_type(duration_sign_negative_only) + default_time_duration_format)
    {}

    //! Changes format for time_duration
    void time_duration_format(const char_type* const format)
    {
      m_time_duration_format = format;
    }

    void set_iso_format() BOOST_OVERRIDE
    {
      this->m_format = iso_time_format_specifier;
    }
    void set_iso_extended_format() BOOST_OVERRIDE
    {
      this->m_format = iso_time_format_extended_specifier;
    }

    OutItrT put(OutItrT next_arg,
                std::ios_base& ios_arg,
                char_type fill_arg,
                const time_type& time_arg) const
    {
      if (time_arg.is_special()) {
        return this->do_put_special(next_arg, ios_arg, fill_arg,
                              time_arg.date().as_special());
      }
      string_type local_format(this->m_format);

      // %T and %R have to be replaced here since they are not standard
      boost::algorithm::replace_all(local_format,
        boost::as_literal(formats_type::full_24_hour_time_format),
        boost::as_literal(formats_type::full_24_hour_time_expanded_format));
      boost::algorithm::replace_all(local_format,
        boost::as_literal(formats_type::short_24_hour_time_format),
        boost::as_literal(formats_type::short_24_hour_time_expanded_format));

      string_type frac_str;
      if (local_format.find(seconds_with_fractional_seconds_format) != string_type::npos) {
        // replace %s with %S.nnn
        frac_str =
          fractional_seconds_as_string(time_arg.time_of_day(), false);
        char_type sep = std::use_facet<std::numpunct<char_type> >(ios_arg.getloc()).decimal_point();

        string_type replace_string(seconds_format);
        replace_string += sep;
        replace_string += frac_str;
        boost::algorithm::replace_all(local_format,
                                      seconds_with_fractional_seconds_format,
                                      replace_string);
      }
      /* NOTE: replacing posix_zone_string_format must be done BEFORE
       * zone_name_format: "%ZP" & "%Z", if Z is checked first it will
       * incorrectly replace a zone_name where a posix_string should go */
      if (local_format.find(posix_zone_string_format) != string_type::npos) {
        if(time_arg.zone_abbrev().empty()) {
          // if zone_abbrev() returns an empty string, we want to
          // erase posix_zone_string_format from format
          boost::algorithm::erase_all(local_format, posix_zone_string_format);
        }
        else{
          boost::algorithm::replace_all(local_format,
                                        posix_zone_string_format,
                                        time_arg.zone_as_posix_string());
        }
      }
      if (local_format.find(zone_name_format) != string_type::npos) {
        if(time_arg.zone_name().empty()) {
          /* TODO: this'll probably create problems if a user places
           * the zone_*_format flag in the format with a ptime. This
           * code removes the flag from the default formats */

          // if zone_name() returns an empty string, we want to
          // erase zone_name_format & one preceeding space
          std::basic_ostringstream<char_type> ss;
          ss << ' ' << zone_name_format;
          boost::algorithm::erase_all(local_format, ss.str());
        }
        else{
          boost::algorithm::replace_all(local_format,
                                        zone_name_format,
                                        time_arg.zone_name());
        }
      }
      if (local_format.find(zone_abbrev_format) != string_type::npos) {
        if(time_arg.zone_abbrev(false).empty()) {
          /* TODO: this'll probably create problems if a user places
           * the zone_*_format flag in the format with a ptime. This
           * code removes the flag from the default formats */

          // if zone_abbrev() returns an empty string, we want to
          // erase zone_abbrev_format & one preceeding space
          std::basic_ostringstream<char_type> ss;
          ss << ' ' << zone_abbrev_format;
          boost::algorithm::erase_all(local_format, ss.str());
        }
        else{
          boost::algorithm::replace_all(local_format,
                                        zone_abbrev_format,
                                        time_arg.zone_abbrev(false));
        }
      }
      if (local_format.find(zone_iso_extended_format) != string_type::npos) {
        if(time_arg.zone_name(true).empty()) {
          /* TODO: this'll probably create problems if a user places
           * the zone_*_format flag in the format with a ptime. This
           * code removes the flag from the default formats */

          // if zone_name() returns an empty string, we want to
          // erase zone_iso_extended_format from format
          boost::algorithm::erase_all(local_format, zone_iso_extended_format);
        }
        else{
          boost::algorithm::replace_all(local_format,
                                        zone_iso_extended_format,
                                        time_arg.zone_name(true));
        }
      }

      if (local_format.find(zone_iso_format) != string_type::npos) {
        if(time_arg.zone_abbrev(true).empty()) {
          /* TODO: this'll probably create problems if a user places
           * the zone_*_format flag in the format with a ptime. This
           * code removes the flag from the default formats */

          // if zone_abbrev() returns an empty string, we want to
          // erase zone_iso_format from format
          boost::algorithm::erase_all(local_format, zone_iso_format);
        }
        else{
          boost::algorithm::replace_all(local_format,
                                        zone_iso_format,
                                        time_arg.zone_abbrev(true));
        }
      }
      if (local_format.find(fractional_seconds_format) != string_type::npos) {
        // replace %f with nnnnnnn
        if (frac_str.empty()) {
          frac_str = fractional_seconds_as_string(time_arg.time_of_day(), false);
        }
        boost::algorithm::replace_all(local_format,
                                      fractional_seconds_format,
                                      frac_str);
      }

      if (local_format.find(fractional_seconds_or_none_format) != string_type::npos) {
        // replace %F with nnnnnnn or nothing if fs == 0
        frac_str =
          fractional_seconds_as_string(time_arg.time_of_day(), true);
        if (!frac_str.empty()) {
          char_type sep = std::use_facet<std::numpunct<char_type> >(ios_arg.getloc()).decimal_point();
          string_type replace_string;
          replace_string += sep;
          replace_string += frac_str;
          boost::algorithm::replace_all(local_format,
                                        fractional_seconds_or_none_format,
                                        replace_string);
        }
        else {
          boost::algorithm::erase_all(local_format,
                                      fractional_seconds_or_none_format);
        }
      }

      return this->do_put_tm(next_arg, ios_arg, fill_arg,
                       to_tm(time_arg), local_format);
    }

    //! put function for time_duration
    OutItrT put(OutItrT next_arg,
                std::ios_base& ios_arg,
                char_type fill_arg,
                const time_duration_type& time_dur_arg) const
    {
      if (time_dur_arg.is_special()) {
        return this->do_put_special(next_arg, ios_arg, fill_arg,
                              time_dur_arg.get_rep().as_special());
      }

      string_type format(m_time_duration_format);
      if (time_dur_arg.is_negative()) {
        // replace %- with minus sign.  Should we use the numpunct facet?
        boost::algorithm::replace_all(format,
                                      duration_sign_negative_only,
                                      negative_sign);
          // remove all the %+ in the string with '-'
        boost::algorithm::replace_all(format,
                                      duration_sign_always,
                                      negative_sign);
      }
      else { //duration is positive
        // remove all the %- combos from the string
        boost::algorithm::erase_all(format, duration_sign_negative_only);
        // remove all the %+ in the string with '+'
        boost::algorithm::replace_all(format,
                                      duration_sign_always,
                                      positive_sign);
      }

      // %T and %R have to be replaced here since they are not standard
      boost::algorithm::replace_all(format,
        boost::as_literal(formats_type::full_24_hour_time_format),
        boost::as_literal(formats_type::full_24_hour_time_expanded_format));
      boost::algorithm::replace_all(format,
        boost::as_literal(formats_type::short_24_hour_time_format),
        boost::as_literal(formats_type::short_24_hour_time_expanded_format));

      /*
       * It is possible for a time duration to span more then 24 hours.
       * Standard time_put::put is obliged to behave the same as strftime
       * (See ISO 14882-2003 22.2.5.3.1 par. 1) and strftime's behavior is
       * unspecified for the case when tm_hour field is outside 0-23 range
       * (See ISO 9899-1999 7.23.3.5 par. 3). So we must output %H and %O
       * here ourself.
       */
      string_type hours_str;
      if (format.find(unrestricted_hours_format) != string_type::npos) {
        hours_str = hours_as_string(time_dur_arg);
        boost::algorithm::replace_all(format, unrestricted_hours_format, hours_str);
      }
      // We still have to process restricted hours format specifier. In order to
      // support parseability of durations in ISO format (%H%M%S), we'll have to
      // restrict the stringified hours length to 2 characters.
      if (format.find(hours_format) != string_type::npos) {
        if (hours_str.empty())
          hours_str = hours_as_string(time_dur_arg);
        BOOST_ASSERT(hours_str.length() <= 2);
        boost::algorithm::replace_all(format, hours_format, hours_str);
      }

      string_type frac_str;
      if (format.find(seconds_with_fractional_seconds_format) != string_type::npos) {
        // replace %s with %S.nnn
        frac_str =
          fractional_seconds_as_string(time_dur_arg, false);
        char_type sep = std::use_facet<std::numpunct<char_type> >(ios_arg.getloc()).decimal_point();

        string_type replace_string(seconds_format);
        replace_string += sep;
        replace_string += frac_str;
        boost::algorithm::replace_all(format,
                                      seconds_with_fractional_seconds_format,
                                      replace_string);
      }
      if (format.find(fractional_seconds_format) != string_type::npos) {
        // replace %f with nnnnnnn
        if (!frac_str.size()) {
          frac_str = fractional_seconds_as_string(time_dur_arg, false);
        }
        boost::algorithm::replace_all(format,
                                      fractional_seconds_format,
                                      frac_str);
      }

      if (format.find(fractional_seconds_or_none_format) != string_type::npos) {
        // replace %F with nnnnnnn or nothing if fs == 0
        frac_str =
          fractional_seconds_as_string(time_dur_arg, true);
        if (frac_str.size()) {
          char_type sep = std::use_facet<std::numpunct<char_type> >(ios_arg.getloc()).decimal_point();
          string_type replace_string;
          replace_string += sep;
          replace_string += frac_str;
          boost::algorithm::replace_all(format,
                                        fractional_seconds_or_none_format,
                                        replace_string);
        }
        else {
          boost::algorithm::erase_all(format,
                                      fractional_seconds_or_none_format);
        }
      }

      return this->do_put_tm(next_arg, ios_arg, fill_arg,
                       to_tm(time_dur_arg), format);
    }

    OutItrT put(OutItrT next, std::ios_base& ios_arg,
                char_type fill, const period_type& p) const
    {
      return this->m_period_formatter.put_period(next, ios_arg, fill,p,*this);
    }


  protected:

    static
    string_type
    fractional_seconds_as_string(const time_duration_type& time_arg,
                                 bool null_when_zero)
    {
      typename time_duration_type::fractional_seconds_type frac_sec =
        time_arg.fractional_seconds();

      if (null_when_zero && (frac_sec == 0)) {
        return string_type();
      }

      //make sure there is no sign
      return integral_as_string(
        date_time::absolute_value(frac_sec),
        time_duration_type::num_fractional_digits());
    }

    static
    string_type
    hours_as_string(const time_duration_type& time_arg, int width = 2)
    {
      return integral_as_string(date_time::absolute_value(time_arg.hours()), width);
    }

    template< typename IntT >
    static
    string_type
    integral_as_string(IntT val, int width = 2)
    {
      std::basic_ostringstream<char_type> ss;
      ss.imbue(std::locale::classic()); // don't want any formatting
      ss << std::setw(width)
        << std::setfill(static_cast<char_type>('0'));
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
      // JDG [7/6/02 VC++ compatibility]
      char_type buff[34];
      ss << _i64toa(static_cast<boost::int64_t>(val), buff, 10);
#else
      ss << val;
#endif
      return ss.str();
    }

  private:
    string_type m_time_duration_format;

  };

  template <class time_type, class CharT, class OutItrT>
  std::locale::id time_facet<time_type, CharT, OutItrT>::id;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::fractional_seconds_format = time_formats<CharT>::fractional_seconds_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::fractional_seconds_or_none_format = time_formats<CharT>::fractional_seconds_or_none_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::seconds_with_fractional_seconds_format =
    time_formats<CharT>::seconds_with_fractional_seconds_format;


  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::zone_name_format =  time_formats<CharT>::zone_name_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::zone_abbrev_format =  time_formats<CharT>::zone_abbrev_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::zone_iso_extended_format =time_formats<CharT>::zone_iso_extended_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::posix_zone_string_format =time_formats<CharT>::posix_zone_string_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::zone_iso_format =  time_formats<CharT>::zone_iso_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::seconds_format =  time_formats<CharT>::seconds_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::hours_format =  time_formats<CharT>::hours_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::unrestricted_hours_format =  time_formats<CharT>::unrestricted_hours_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::standard_format =  time_formats<CharT>::standard_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::duration_seperator =  time_formats<CharT>::duration_seperator;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::negative_sign =  time_formats<CharT>::negative_sign;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::positive_sign =  time_formats<CharT>::positive_sign;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::duration_sign_negative_only =  time_formats<CharT>::duration_sign_negative_only;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::duration_sign_always =  time_formats<CharT>::duration_sign_always;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type,CharT, OutItrT>::char_type*
  time_facet<time_type,CharT, OutItrT>::iso_time_format_specifier = time_formats<CharT>::iso_time_format_specifier;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::iso_time_format_extended_specifier = time_formats<CharT>::iso_time_format_extended_specifier;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::default_time_format =
    time_formats<CharT>::default_time_format;

  template <class time_type, class CharT, class OutItrT>
  const typename time_facet<time_type, CharT, OutItrT>::char_type*
  time_facet<time_type, CharT, OutItrT>::default_time_duration_format =
    time_formats<CharT>::default_time_duration_format;


  //! Facet for format-based input.
  /*!
   */
  template <class time_type,
            class CharT,
            class InItrT = std::istreambuf_iterator<CharT, std::char_traits<CharT> > >
  class BOOST_SYMBOL_VISIBLE time_input_facet :
    public boost::date_time::date_input_facet<typename time_type::date_type , CharT, InItrT> {
    public:
      typedef typename time_type::date_type date_type;
      typedef typename time_type::time_duration_type time_duration_type;
      typedef typename time_duration_type::fractional_seconds_type fracional_seconds_type;
      typedef boost::date_time::period<time_type,time_duration_type> period_type;
      typedef boost::date_time::date_input_facet<typename time_type::date_type, CharT, InItrT> base_type;
      typedef typename base_type::duration_type date_duration_type;
      typedef typename base_type::year_type year_type;
      typedef typename base_type::month_type month_type;
      typedef typename base_type::day_type day_type;
      typedef typename base_type::string_type string_type;
      typedef typename string_type::const_iterator const_itr;
      typedef typename base_type::char_type   char_type;
      typedef typename base_type::format_date_parser_type format_date_parser_type;
      typedef typename base_type::period_parser_type period_parser_type;
      typedef typename base_type::special_values_parser_type special_values_parser_type;
      typedef typename base_type::date_gen_parser_type date_gen_parser_type;
      typedef typename base_type::special_values_parser_type::match_results match_results;

      static const char_type* fractional_seconds_format;                // f
      static const char_type* fractional_seconds_or_none_format;        // F
      static const char_type* seconds_with_fractional_seconds_format;   // s
      static const char_type* seconds_format;                           // S
      static const char_type* standard_format;                          // x X
      static const char_type* zone_abbrev_format;                       // z
      static const char_type* zone_name_format;                         // Z
      static const char_type* zone_iso_format;                          // q
      static const char_type* zone_iso_extended_format;                 // Q
      static const char_type* duration_seperator;
      static const char_type* iso_time_format_specifier;
      static const char_type* iso_time_format_extended_specifier;
      static const char_type* default_time_input_format;
      static const char_type* default_time_duration_format;
      static std::locale::id id;

      //! Constructor that takes a format string for a ptime
      explicit time_input_facet(const string_type& format, ::size_t ref_arg = 0)
        : base_type(format, ref_arg),
          m_time_duration_format(default_time_duration_format)
      { }

      explicit time_input_facet(const string_type& format,
                                const format_date_parser_type& date_parser,
                                const special_values_parser_type& sv_parser,
                                const period_parser_type& per_parser,
                                const date_gen_parser_type& date_gen_parser,
                                ::size_t ref_arg = 0)
        : base_type(format,
                    date_parser,
                    sv_parser,
                    per_parser,
                    date_gen_parser,
                    ref_arg),
          m_time_duration_format(default_time_duration_format)
      {}

      //! sets default formats for ptime, local_date_time, and time_duration
      explicit time_input_facet(::size_t ref_arg = 0)
        : base_type(default_time_input_format, ref_arg),
          m_time_duration_format(default_time_duration_format)
      { }

      //! Set the format for time_duration
      void time_duration_format(const char_type* const format) {
        m_time_duration_format = format;
      }
      virtual void set_iso_format()
      {
        this->m_format = iso_time_format_specifier;
      }
      virtual void set_iso_extended_format()
      {
        this->m_format = iso_time_format_extended_specifier;
      }

      InItrT get(InItrT& sitr,
                 InItrT& stream_end,
                 std::ios_base& ios_arg,
                 period_type& p) const
      {
        p = this->m_period_parser.get_period(sitr,
                                             stream_end,
                                             ios_arg,
                                             p,
                                             time_duration_type::unit(),
                                             *this);
        return sitr;
      }

      //default ptime format is YYYY-Mon-DD HH:MM:SS[.fff...][ zzz]
      //default time_duration format is %H:%M:%S%F HH:MM:SS[.fff...]

      InItrT get(InItrT& sitr,
                 InItrT& stream_end,
                 std::ios_base& ios_arg,
                 time_duration_type& td) const
      {
        // skip leading whitespace
        while((sitr != stream_end) && std::isspace(*sitr)) { ++sitr; }

        bool use_current_char = false;

        // num_get will consume the +/-, we may need a copy if special_value
        char_type c = '\0';
        if((sitr != stream_end) && (*sitr == '-' || *sitr == '+')) {
          c = *sitr;
        }

        typedef typename time_duration_type::hour_type hour_type;
        typedef typename time_duration_type::min_type min_type;
        typedef typename time_duration_type::sec_type sec_type;

        hour_type hour = 0;
        min_type min = 0;
        sec_type sec = 0;
        typename time_duration_type::fractional_seconds_type frac(0);

        typedef std::num_get<CharT, InItrT> num_get;
        if(!std::has_facet<num_get>(ios_arg.getloc())) {
          num_get* ng = new num_get();
          std::locale loc = std::locale(ios_arg.getloc(), ng);
          ios_arg.imbue(loc);
        }

        const_itr itr(m_time_duration_format.begin());
        while (itr != m_time_duration_format.end() && (sitr != stream_end)) {
          if (*itr == '%') {
            if (++itr == m_time_duration_format.end()) break;
            if (*itr != '%') {
              switch(*itr) {
              case 'O':
                {
                  // A period may span more than 24 hours. In that case the format
                  // string should be composed with the unrestricted hours specifier.
                  hour = var_string_to_int<hour_type, CharT>(sitr, stream_end,
                                      std::numeric_limits<hour_type>::digits10 + 1);
                  if(hour == -1){
                     return check_special_value(sitr, stream_end, td, c);
                  }
                  break;
                }
              case 'H':
                {
                  match_results mr;
                  hour = fixed_string_to_int<hour_type, CharT>(sitr, stream_end, mr, 2);
                  if(hour == -1){
                     return check_special_value(sitr, stream_end, td, c);
                  }
                  break;
                }
              case 'M':
                {
                  match_results mr;
                  min = fixed_string_to_int<min_type, CharT>(sitr, stream_end, mr, 2);
                  if(min == -1){
                     return check_special_value(sitr, stream_end, td, c);
                  }
                  break;
                }
              case 's':
              case 'S':
                {
                  match_results mr;
                  sec = fixed_string_to_int<sec_type, CharT>(sitr, stream_end, mr, 2);
                  if(sec == -1){
                     return check_special_value(sitr, stream_end, td, c);
                  }
                  if (*itr == 'S')
                    break;
                  // %s is the same as %S%f so we drop through into %f
                }
                /* Falls through. */
              case 'f':
                {
                  // check for decimal, check special_values if missing
                  if(*sitr == '.') {
                    ++sitr;
                    parse_frac_type(sitr, stream_end, frac);
                    // sitr will point to next expected char after this parsing
                    // is complete so no need to advance it
                    use_current_char = true;
                  }
                  else {
                    return check_special_value(sitr, stream_end, td, c);
                  }
                  break;
                }
              case 'F':
                {
                  // check for decimal, skip if missing
                  if(*sitr == '.') {
                    ++sitr;
                    parse_frac_type(sitr, stream_end, frac);
                    // sitr will point to next expected char after this parsing
                    // is complete so no need to advance it
                    use_current_char = true;
                  }
                  else {
                    // nothing was parsed so we don't want to advance sitr
                    use_current_char = true;
                  }
                  break;
                }
              default:
                {} // ignore what we don't understand?
              }// switch
            }
            else { // itr == '%', second consecutive
              ++sitr;
            }

            ++itr; //advance past format specifier
          }
          else {  //skip past chars in format and in buffer
            ++itr;
            // set use_current_char when sitr is already
            // pointing at the next character to process
            if (use_current_char) {
              use_current_char = false;
            }
            else {
              ++sitr;
            }
          }
        }

        td = time_duration_type(hour, min, sec, frac);
        return sitr;
      }


      //! Parses a time object from the input stream
      InItrT get(InItrT& sitr,
                 InItrT& stream_end,
                 std::ios_base& ios_arg,
                 time_type& t) const
      {
        string_type tz_str;
        return get(sitr, stream_end, ios_arg, t, tz_str, false);
      }
      //! Expects a time_zone in the input stream
      InItrT get_local_time(InItrT& sitr,
                            InItrT& stream_end,
                            std::ios_base& ios_arg,
                            time_type& t,
                            string_type& tz_str) const
      {
        return get(sitr, stream_end, ios_arg, t, tz_str, true);
      }

    protected:

      InItrT get(InItrT& sitr,
                 InItrT& stream_end,
                 std::ios_base& ios_arg,
                 time_type& t,
                 string_type& tz_str,
                 bool time_is_local) const
      {
        // skip leading whitespace
        while((sitr != stream_end) && std::isspace(*sitr)) { ++sitr; }

        bool use_current_char = false;
        bool use_current_format_char = false; // used with two character flags

        // num_get will consume the +/-, we may need a copy if special_value
        char_type c = '\0';
        if((sitr != stream_end) && (*sitr == '-' || *sitr == '+')) {
          c = *sitr;
        }

        typedef typename time_duration_type::hour_type hour_type;
        typedef typename time_duration_type::min_type min_type;
        typedef typename time_duration_type::sec_type sec_type;

        // time elements
        hour_type hour = 0;
        min_type min = 0;
        sec_type sec = 0;
        typename time_duration_type::fractional_seconds_type frac(0);
        // date elements
        short day_of_year(0);
        /* Initialized the following to their minimum values. These intermediate
         * objects are used so we get specific exceptions when part of the input
         * is unparsable.
         * Ex: "205-Jan-15" will throw a bad_year, "2005-Jsn-15"- bad_month, etc.*/
        year_type t_year(1400);
        month_type t_month(1);
        day_type t_day(1);

        typedef std::num_get<CharT, InItrT> num_get;
        if(!std::has_facet<num_get>(ios_arg.getloc())) {
          num_get* ng = new num_get();
          std::locale loc = std::locale(ios_arg.getloc(), ng);
          ios_arg.imbue(loc);
        }

        const_itr itr(this->m_format.begin());
        while (itr != this->m_format.end() && (sitr != stream_end)) {
          if (*itr == '%') {
            if (++itr == this->m_format.end()) break;
            if (*itr != '%') {
              // the cases are grouped by date & time flags - not alphabetical order
              switch(*itr) {
                // date flags
                case 'Y':
                case 'y':
                  {
                    char_type cs[3] = { '%', *itr };
                    string_type s(cs);
                    match_results mr;
                    try {
                      t_year = this->m_parser.parse_year(sitr, stream_end, s, mr);
                    }
                    catch(std::out_of_range&) { // base class for bad_year exception
                      if(this->m_sv_parser.match(sitr, stream_end, mr)) {
                        t = time_type(static_cast<special_values>(mr.current_match));
                        return sitr;
                      }
                      else {
                        throw; // rethrow bad_year
                      }
                    }
                    break;
                  }
                case 'B':
                case 'b':
                case 'm':
                  {
                    char_type cs[3] = { '%', *itr };
                    string_type s(cs);
                    match_results mr;
                    try {
                      t_month = this->m_parser.parse_month(sitr, stream_end, s, mr);
                    }
                    catch(std::out_of_range&) { // base class for bad_month exception
                      if(this->m_sv_parser.match(sitr, stream_end, mr)) {
                        t = time_type(static_cast<special_values>(mr.current_match));
                        return sitr;
                      }
                      else {
                        throw; // rethrow bad_month
                      }
                    }
                    // did m_parser already advance sitr to next char?
                    if(mr.has_remaining()) {
                      use_current_char = true;
                    }
                    break;
                  }
                case 'a':
                case 'A':
                case 'w':
                  {
                    // weekday is not used in construction but we need to get it out of the stream
                    char_type cs[3] = { '%', *itr };
                    string_type s(cs);
                    match_results mr;
                    typename date_type::day_of_week_type wd(0);
                    try {
                      wd = this->m_parser.parse_weekday(sitr, stream_end, s, mr);
                    }
                    catch(std::out_of_range&) { // base class for bad_weekday exception
                      if(this->m_sv_parser.match(sitr, stream_end, mr)) {
                        t = time_type(static_cast<special_values>(mr.current_match));
                        return sitr;
                      }
                      else {
                        throw; // rethrow bad_weekday
                      }
                    }
                    // did m_parser already advance sitr to next char?
                    if(mr.has_remaining()) {
                      use_current_char = true;
                    }
                    break;
                  }
                case 'j':
                  {
                    // code that gets julian day (from format_date_parser)
                    match_results mr;
                    day_of_year = fixed_string_to_int<unsigned short, CharT>(sitr, stream_end, mr, 3);
                    if(day_of_year == -1) {
                      if(this->m_sv_parser.match(sitr, stream_end, mr)) {
                        t = time_type(static_cast<special_values>(mr.current_match));
                        return sitr;
                      }
                    }
                    // these next two lines are so we get an exception with bad input
                    typedef typename time_type::date_type::day_of_year_type day_of_year_type;
                    day_of_year_type t_day_of_year(day_of_year);
                    break;
                  }
                case 'd':
                case 'e':
                  {
                    try {
                      t_day = (*itr == 'd') ?
                          this->m_parser.parse_day_of_month(sitr, stream_end) :
                          this->m_parser.parse_var_day_of_month(sitr, stream_end);
                    }
                    catch(std::out_of_range&) { // base class for exception bad_day_of_month
                      match_results mr;
                      if(this->m_sv_parser.match(sitr, stream_end, mr)) {
                        t = time_type(static_cast<special_values>(mr.current_match));
                        return sitr;
                      }
                      else {
                        throw; // rethrow bad_day_of_month
                      }
                    }
                    break;
                  }
                // time flags
                case 'H':
                  {
                    match_results mr;
                    hour = fixed_string_to_int<hour_type, CharT>(sitr, stream_end, mr, 2);
                    if(hour == -1){
                       return check_special_value(sitr, stream_end, t, c);
                    }
                    break;
                  }
                case 'M':
                  {
                    match_results mr;
                    min = fixed_string_to_int<min_type, CharT>(sitr, stream_end, mr, 2);
                    if(min == -1){
                       return check_special_value(sitr, stream_end, t, c);
                    }
                    break;
                  }
                case 's':
                case 'S':
                  {
                    match_results mr;
                    sec = fixed_string_to_int<sec_type, CharT>(sitr, stream_end, mr, 2);
                    if(sec == -1){
                       return check_special_value(sitr, stream_end, t, c);
                    }
                    if (*itr == 'S' || sitr == stream_end)
                      break;
                    // %s is the same as %S%f so we drop through into %f if we are
                    // not at the end of the stream
                  }
                  /* Falls through. */
                case 'f':
                  {
                    // check for decimal, check SV if missing
                    if(*sitr == '.') {
                      ++sitr;
                      parse_frac_type(sitr, stream_end, frac);
                      // sitr will point to next expected char after this parsing
                      // is complete so no need to advance it
                      use_current_char = true;
                    }
                    else {
                      return check_special_value(sitr, stream_end, t, c);
                    }
                    break;
                  }
                case 'F':
                  {
                    // check for decimal, skip if missing
                    if(*sitr == '.') {
                      ++sitr;
                      parse_frac_type(sitr, stream_end, frac);
                      // sitr will point to next expected char after this parsing
                      // is complete so no need to advance it
                      use_current_char = true;
                    }
                    else {
                      // nothing was parsed so we don't want to advance sitr
                      use_current_char = true;
                    }
                    break;
                  }
                  // time_zone flags
                //case 'q':
                //case 'Q':
                //case 'z':
                case 'Z':
                  {
                    if(time_is_local) { // skip if 't' is a ptime
                      ++itr;
                      if(*itr == 'P') {
                        // skip leading whitespace
                        while((sitr != stream_end) && std::isspace(*sitr)) { ++sitr; }
                        // parse zone
                        while((sitr != stream_end) && (!std::isspace(*sitr))) {
                          tz_str += *sitr;
                          ++sitr;
                        }
                      }
                      else {
                        use_current_format_char = true;
                      }

                    }
                    else {
                      // nothing was parsed so we don't want to advance sitr
                      use_current_char = true;
                    }

                    break;
                  }
                default:
                {} // ignore what we don't understand?
              }// switch
            }
            else { // itr == '%', second consecutive
              ++sitr;
            }

            if(use_current_format_char) {
              use_current_format_char = false;
            }
            else {
              ++itr; //advance past format specifier
            }

          }
          else {  //skip past chars in format and in buffer
            ++itr;
            // set use_current_char when sitr is already
            // pointing at the next character to process
            if (use_current_char) {
              use_current_char = false;
            }
            else {
              ++sitr;
            }
          }
        }

        date_type d(not_a_date_time);
        if (day_of_year > 0) {
          d = date_type(static_cast<unsigned short>(t_year),1,1) + date_duration_type(day_of_year-1);
        }
        else {
          d = date_type(t_year, t_month, t_day);
        }

        time_duration_type td(hour, min, sec, frac);
        t = time_type(d, td);
        return sitr;
      }

      //! Helper function to check for special_value
      /*! First character may have been consumed during original parse
       * attempt. Parameter 'c' should be a copy of that character.
       * Throws ios_base::failure if parse fails. */
      template<class temporal_type>
      inline
      InItrT check_special_value(InItrT& sitr,InItrT& stream_end, temporal_type& tt, char_type c='\0') const
      {
        match_results mr;
        if((c == '-' || c == '+') && (*sitr != c)) { // was the first character consumed?
          mr.cache += c;
        }
        (void)this->m_sv_parser.match(sitr, stream_end, mr);
        if(mr.current_match == match_results::PARSE_ERROR) {
          std::string tmp = convert_string_type<char_type, char>(mr.cache);
          boost::throw_exception(std::ios_base::failure("Parse failed. No match found for '" + tmp + "'"));
          BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(return sitr); // should never reach
        }
        tt = temporal_type(static_cast<special_values>(mr.current_match));
        return sitr;
      }

      //! Helper function for parsing a fractional second type from the stream
      void parse_frac_type(InItrT& sitr,
                           InItrT& stream_end,
                           fracional_seconds_type& frac) const
      {
        string_type cache;
        while((sitr != stream_end) && std::isdigit(*sitr)) {
          cache += *sitr;
          ++sitr;
        }
        if(cache.size() > 0) {
          unsigned short precision = time_duration_type::num_fractional_digits();
          // input may be only the first few decimal places
          if(cache.size() < precision) {
            frac = lexical_cast<fracional_seconds_type>(cache);
            frac = decimal_adjust(frac, static_cast<unsigned short>(precision - cache.size()));
          }
          else {
            // if input has too many decimal places, drop excess digits
            frac = lexical_cast<fracional_seconds_type>(cache.substr(0, precision));
          }
        }
      }

    private:
      string_type m_time_duration_format;

      //! Helper function to adjust trailing zeros when parsing fractional digits
      template<class int_type>
      inline
      int_type decimal_adjust(int_type val, const unsigned short places) const
      {
        unsigned long factor = 1;
        for(int i = 0; i < places; ++i){
          factor *= 10; // shift decimal to the right
        }
        return val * factor;
      }

  };

template <class time_type, class CharT, class InItrT>
  std::locale::id time_input_facet<time_type, CharT, InItrT>::id;

template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::fractional_seconds_format = time_formats<CharT>::fractional_seconds_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::fractional_seconds_or_none_format = time_formats<CharT>::fractional_seconds_or_none_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::seconds_with_fractional_seconds_format = time_formats<CharT>::seconds_with_fractional_seconds_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::seconds_format = time_formats<CharT>::seconds_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::standard_format = time_formats<CharT>::standard_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::zone_abbrev_format = time_formats<CharT>::zone_abbrev_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::zone_name_format = time_formats<CharT>::zone_name_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::zone_iso_format = time_formats<CharT>::zone_iso_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::zone_iso_extended_format = time_formats<CharT>::zone_iso_extended_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::duration_seperator = time_formats<CharT>::duration_seperator;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::iso_time_format_specifier = time_formats<CharT>::iso_time_format_specifier;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::iso_time_format_extended_specifier = time_formats<CharT>::iso_time_format_extended_specifier;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::default_time_input_format = time_formats<CharT>::default_time_input_format;

  template <class time_type, class CharT, class InItrT>
  const typename time_input_facet<time_type, CharT, InItrT>::char_type*
  time_input_facet<time_type, CharT, InItrT>::default_time_duration_format = time_formats<CharT>::default_time_duration_format;

} } // namespaces

#endif
