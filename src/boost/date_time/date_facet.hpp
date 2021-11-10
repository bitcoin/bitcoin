#ifndef _DATE_TIME_DATE_FACET__HPP___
#define _DATE_TIME_DATE_FACET__HPP___

/* Copyright (c) 2004-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author:  Martin Andrian, Jeff Garland, Bart Garst
 * $Date$
 */

#include <iterator> // ostreambuf_iterator
#include <locale>
#include <string>
#include <vector>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/period.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/special_values_formatter.hpp>
#include <boost/date_time/period_formatter.hpp>
#include <boost/date_time/period_parser.hpp>
#include <boost/date_time/date_generator_formatter.hpp>
#include <boost/date_time/date_generator_parser.hpp>
#include <boost/date_time/format_date_parser.hpp>

namespace boost { namespace date_time {


  /*! Class that provides format based I/O facet for date types.
   *
   * This class allows the formatting of dates by using format string.
   * Format strings are:
   *
   *  - %A => long_weekday_format - Full name Ex: Tuesday
   *  - %a => short_weekday_format - Three letter abbreviation Ex: Tue
   *  - %B => long_month_format - Full name Ex: October
   *  - %b => short_month_format - Three letter abbreviation Ex: Oct
   *  - %x => standard_format_specifier - defined by the locale
   *  - %Y-%b-%d => default_date_format - YYYY-Mon-dd
   *
   * Default month format == %b
   * Default weekday format == %a
   */
  template <class date_type,
            class CharT,
            class OutItrT = std::ostreambuf_iterator<CharT, std::char_traits<CharT> > >
  class BOOST_SYMBOL_VISIBLE date_facet : public std::locale::facet {
  public:
    typedef typename date_type::duration_type duration_type;
    // greg_weekday is gregorian_calendar::day_of_week_type
    typedef typename date_type::day_of_week_type day_of_week_type;
    typedef typename date_type::day_type day_type;
    typedef typename date_type::month_type month_type;
    typedef boost::date_time::period<date_type,duration_type> period_type;
    typedef std::basic_string<CharT> string_type;
    typedef CharT                    char_type;
    typedef boost::date_time::period_formatter<CharT>  period_formatter_type;
    typedef boost::date_time::special_values_formatter<CharT>  special_values_formatter_type;
    typedef std::vector<std::basic_string<CharT> > input_collection_type;
    // used for the output of the date_generators
    typedef date_generator_formatter<date_type, CharT> date_gen_formatter_type;
    typedef partial_date<date_type>          partial_date_type;
    typedef nth_kday_of_month<date_type>     nth_kday_type;
    typedef first_kday_of_month<date_type>   first_kday_type;
    typedef last_kday_of_month<date_type>    last_kday_type;
    typedef first_kday_after<date_type>      kday_after_type;
    typedef first_kday_before<date_type>     kday_before_type;
    static const char_type long_weekday_format[3];
    static const char_type short_weekday_format[3];
    static const char_type long_month_format[3];
    static const char_type short_month_format[3];
    static const char_type default_period_separator[4];
    static const char_type standard_format_specifier[3];
    static const char_type iso_format_specifier[7];
    static const char_type iso_format_extended_specifier[9];
    static const char_type default_date_format[9]; // YYYY-Mon-DD
    static std::locale::id id;

#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
      std::locale::id& __get_id (void) const { return id; }
#endif

    explicit date_facet(::size_t a_ref = 0)
      : std::locale::facet(a_ref),
        //m_format(standard_format_specifier)
        m_format(default_date_format),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format)
    {}

    explicit date_facet(const char_type* format_str,
                        const input_collection_type& short_names,
                        ::size_t ref_count = 0)
      : std::locale::facet(ref_count),
        m_format(format_str),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format),
        m_month_short_names(short_names)
    {}


    explicit date_facet(const char_type* format_str,
                        period_formatter_type per_formatter = period_formatter_type(),
                        special_values_formatter_type sv_formatter = special_values_formatter_type(),
                        date_gen_formatter_type dg_formatter = date_gen_formatter_type(),
                        ::size_t ref_count = 0)
      : std::locale::facet(ref_count),
        m_format(format_str),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format),
        m_period_formatter(per_formatter),
        m_date_gen_formatter(dg_formatter),
        m_special_values_formatter(sv_formatter)
     {}
    void format(const char_type* const format_str) {
      m_format = format_str;
    }
    virtual void set_iso_format()
    {
      m_format = iso_format_specifier;
    }
    virtual void set_iso_extended_format()
    {
      m_format = iso_format_extended_specifier;
    }
    void month_format(const char_type* const format_str) {
      m_month_format = format_str;
    }
    void weekday_format(const char_type* const format_str) {
      m_weekday_format = format_str;
    }

    void period_formatter(period_formatter_type per_formatter) {
      m_period_formatter= per_formatter;
    }
    void special_values_formatter(const special_values_formatter_type& svf)
    {
      m_special_values_formatter = svf;
    }
    void short_weekday_names(const input_collection_type& short_names)
    {
      m_weekday_short_names = short_names;
    }
    void long_weekday_names(const input_collection_type& long_names)
    {
      m_weekday_long_names = long_names;
    }

    void short_month_names(const input_collection_type& short_names)
    {
      m_month_short_names = short_names;
    }

    void long_month_names(const input_collection_type& long_names)
    {
      m_month_long_names = long_names;
    }

    void date_gen_phrase_strings(const input_collection_type& new_strings,
                           typename date_gen_formatter_type::phrase_elements beg_pos=date_gen_formatter_type::first)
    {
      m_date_gen_formatter.elements(new_strings, beg_pos);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const date_type& d) const
    {
      if (d.is_special()) {
        return do_put_special(next, a_ios, fill_char, d.as_special());
      }
      //The following line of code required the date to support a to_tm function
      return do_put_tm(next, a_ios, fill_char, to_tm(d), m_format);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const duration_type& dd) const
    {
      if (dd.is_special()) {
        return do_put_special(next, a_ios, fill_char, dd.get_rep().as_special());
      }

      typedef std::num_put<CharT, OutItrT> num_put;
      if (std::has_facet<num_put>(a_ios.getloc())) {
        return std::use_facet<num_put>(a_ios.getloc()).put(next, a_ios, fill_char, dd.get_rep().as_number());
      }
      else {
        num_put* f = new num_put();
        std::locale l = std::locale(a_ios.getloc(), f);
        a_ios.imbue(l);
        return f->put(next, a_ios, fill_char, dd.get_rep().as_number());
      }

    }


    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const month_type& m) const
    {
      //if (d.is_special()) {
      //  return do_put_special(next, a_ios, fill_char, d.as_special());
      //}
      //The following line of code required the date to support a to_tm function
      std::tm dtm;
      std::memset(&dtm, 0, sizeof(dtm));
      dtm.tm_mon = m - 1;
      return do_put_tm(next, a_ios, fill_char, dtm, m_month_format);
    }

    //! puts the day of month
    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const day_type& day) const
    {
      std::tm dtm;
      std::memset(&dtm, 0, sizeof(dtm));
      dtm.tm_mday = day.as_number();
      char_type tmp[3] = {'%','d'};
      string_type temp_format(tmp);
      return do_put_tm(next, a_ios, fill_char, dtm, temp_format);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const day_of_week_type& dow) const
    {
      //if (d.is_special()) {
      //  return do_put_special(next, a_ios, fill_char, d.as_special());
      //}
      //The following line of code required the date to support a to_tm function
      std::tm dtm;
      std::memset(&dtm, 0, sizeof(dtm));
      dtm.tm_wday = dow;
      return do_put_tm(next, a_ios, fill_char, dtm, m_weekday_format);
    }


    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const period_type& p) const
    {
      return m_period_formatter.put_period(next, a_ios, fill_char, p, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const partial_date_type& pd) const
    {
      return m_date_gen_formatter.put_partial_date(next, a_ios, fill_char, pd, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const nth_kday_type& nkd) const
    {
      return m_date_gen_formatter.put_nth_kday(next, a_ios, fill_char, nkd, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const first_kday_type& fkd) const
    {
      return m_date_gen_formatter.put_first_kday(next, a_ios, fill_char, fkd, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const last_kday_type& lkd) const
    {
      return m_date_gen_formatter.put_last_kday(next, a_ios, fill_char, lkd, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const kday_before_type& fkb) const
    {
      return m_date_gen_formatter.put_kday_before(next, a_ios, fill_char, fkb, *this);
    }

    OutItrT put(OutItrT next,
                std::ios_base& a_ios,
                char_type fill_char,
                const kday_after_type& fka) const
    {
      return m_date_gen_formatter.put_kday_after(next, a_ios, fill_char, fka, *this);
    }

  protected:
    virtual OutItrT do_put_special(OutItrT next,
                                   std::ios_base& /*a_ios*/,
                                   char_type /*fill_char*/,
                                   const boost::date_time::special_values sv) const
    {
      m_special_values_formatter.put_special(next, sv);
      return next;
    }
    virtual OutItrT do_put_tm(OutItrT next,
                              std::ios_base& a_ios,
                              char_type fill_char,
                              const tm& tm_value,
                              string_type a_format) const
    {
      // update format string with custom names
      if (!m_weekday_long_names.empty()) {
        boost::algorithm::replace_all(a_format,
                                      long_weekday_format,
                                      m_weekday_long_names[tm_value.tm_wday]);
      }
      if (!m_weekday_short_names.empty()) {
        boost::algorithm::replace_all(a_format,
                                      short_weekday_format,
                                      m_weekday_short_names[tm_value.tm_wday]);

      }
      if (!m_month_long_names.empty()) {
        boost::algorithm::replace_all(a_format,
                                      long_month_format,
                                      m_month_long_names[tm_value.tm_mon]);
      }
      if (!m_month_short_names.empty()) {
        boost::algorithm::replace_all(a_format,
                                      short_month_format,
                                      m_month_short_names[tm_value.tm_mon]);
      }
      // use time_put facet to create final string
      const char_type* p_format = a_format.c_str();
      return std::use_facet<std::time_put<CharT> >(a_ios.getloc()).put(next, a_ios,
                                                                       fill_char,
                                                                       &tm_value,
                                                                       p_format,
                                                                       p_format + a_format.size());
    }
  protected:
    string_type                   m_format;
    string_type                   m_month_format;
    string_type                   m_weekday_format;
    period_formatter_type         m_period_formatter;
    date_gen_formatter_type       m_date_gen_formatter;
    special_values_formatter_type m_special_values_formatter;
    input_collection_type         m_month_short_names;
    input_collection_type         m_month_long_names;
    input_collection_type         m_weekday_short_names;
    input_collection_type         m_weekday_long_names;
  private:
  };

  template <class date_type, class CharT, class OutItrT>
  std::locale::id date_facet<date_type, CharT, OutItrT>::id;

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::long_weekday_format[3] = {'%','A'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::short_weekday_format[3] = {'%','a'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::long_month_format[3] = {'%','B'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::short_month_format[3] = {'%','b'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::default_period_separator[4] = { ' ', '/', ' '};

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::standard_format_specifier[3] =
    {'%', 'x' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::iso_format_specifier[7] =
    {'%', 'Y', '%', 'm', '%', 'd' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::iso_format_extended_specifier[9] =
    {'%', 'Y', '-', '%', 'm', '-', '%', 'd' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_facet<date_type, CharT, OutItrT>::char_type
  date_facet<date_type, CharT, OutItrT>::default_date_format[9] =
    {'%','Y','-','%','b','-','%','d'};



  //! Input facet
  template <class date_type,
            class CharT,
            class InItrT = std::istreambuf_iterator<CharT, std::char_traits<CharT> > >
  class BOOST_SYMBOL_VISIBLE date_input_facet : public std::locale::facet {
  public:
    typedef typename date_type::duration_type duration_type;
    // greg_weekday is gregorian_calendar::day_of_week_type
    typedef typename date_type::day_of_week_type day_of_week_type;
    typedef typename date_type::day_type day_type;
    typedef typename date_type::month_type month_type;
    typedef typename date_type::year_type year_type;
    typedef boost::date_time::period<date_type,duration_type> period_type;
    typedef std::basic_string<CharT> string_type;
    typedef CharT                    char_type;
    typedef boost::date_time::period_parser<date_type, CharT>  period_parser_type;
    typedef boost::date_time::special_values_parser<date_type,CharT> special_values_parser_type;
    typedef std::vector<std::basic_string<CharT> > input_collection_type;
    typedef format_date_parser<date_type, CharT> format_date_parser_type;
    // date_generators stuff goes here
    typedef date_generator_parser<date_type, CharT> date_gen_parser_type;
    typedef partial_date<date_type>          partial_date_type;
    typedef nth_kday_of_month<date_type>     nth_kday_type;
    typedef first_kday_of_month<date_type>   first_kday_type;
    typedef last_kday_of_month<date_type>    last_kday_type;
    typedef first_kday_after<date_type>      kday_after_type;
    typedef first_kday_before<date_type>     kday_before_type;

    static const char_type long_weekday_format[3];
    static const char_type short_weekday_format[3];
    static const char_type long_month_format[3];
    static const char_type short_month_format[3];
    static const char_type four_digit_year_format[3];
    static const char_type two_digit_year_format[3];
    static const char_type default_period_separator[4];
    static const char_type standard_format_specifier[3];
    static const char_type iso_format_specifier[7];
    static const char_type iso_format_extended_specifier[9];
    static const char_type default_date_format[9]; // YYYY-Mon-DD
    static std::locale::id id;

    explicit date_input_facet(::size_t a_ref = 0)
      : std::locale::facet(a_ref),
        m_format(default_date_format),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format),
        m_year_format(four_digit_year_format),
        m_parser(m_format, std::locale::classic())
        // default period_parser & special_values_parser used
    {}

    explicit date_input_facet(const string_type& format_str,
                              ::size_t a_ref = 0)
      : std::locale::facet(a_ref),
        m_format(format_str),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format),
        m_year_format(four_digit_year_format),
        m_parser(m_format, std::locale::classic())
        // default period_parser & special_values_parser used
    {}

    explicit date_input_facet(const string_type& format_str,
                              const format_date_parser_type& date_parser,
                              const special_values_parser_type& sv_parser,
                              const period_parser_type& per_parser,
                              const date_gen_parser_type& date_gen_parser,
                              ::size_t ref_count = 0)
      : std::locale::facet(ref_count),
        m_format(format_str),
        m_month_format(short_month_format),
        m_weekday_format(short_weekday_format),
        m_year_format(four_digit_year_format),
        m_parser(date_parser),
        m_date_gen_parser(date_gen_parser),
        m_period_parser(per_parser),
        m_sv_parser(sv_parser)
    {}


    void format(const char_type* const format_str) {
      m_format = format_str;
    }
    virtual void set_iso_format()
    {
      m_format = iso_format_specifier;
    }
    virtual void set_iso_extended_format()
    {
      m_format = iso_format_extended_specifier;
    }
    void month_format(const char_type* const format_str) {
      m_month_format = format_str;
    }
    void weekday_format(const char_type* const format_str) {
      m_weekday_format = format_str;
    }
    void year_format(const char_type* const format_str) {
      m_year_format = format_str;
    }

    void period_parser(period_parser_type per_parser) {
      m_period_parser = per_parser;
    }
    void short_weekday_names(const input_collection_type& weekday_names)
    {
      m_parser.short_weekday_names(weekday_names);
    }
    void long_weekday_names(const input_collection_type& weekday_names)
    {
      m_parser.long_weekday_names(weekday_names);
    }

    void short_month_names(const input_collection_type& month_names)
    {
      m_parser.short_month_names(month_names);
    }

    void long_month_names(const input_collection_type& month_names)
    {
      m_parser.long_month_names(month_names);
    }

    void date_gen_element_strings(const input_collection_type& col)
    {
      m_date_gen_parser.element_strings(col);
    }
    void date_gen_element_strings(const string_type& first,
                                  const string_type& second,
                                  const string_type& third,
                                  const string_type& fourth,
                                  const string_type& fifth,
                                  const string_type& last,
                                  const string_type& before,
                                  const string_type& after,
                                  const string_type& of)

    {
      m_date_gen_parser.element_strings(first,second,third,fourth,fifth,last,before,after,of);
    }

    void special_values_parser(special_values_parser_type sv_parser)
    {
      m_sv_parser = sv_parser;
    }

    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& /*a_ios*/,
               date_type& d) const
    {
      d = m_parser.parse_date(from, to, m_format, m_sv_parser);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& /*a_ios*/,
               month_type& m) const
    {
      m = m_parser.parse_month(from, to, m_month_format);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& /*a_ios*/,
               day_of_week_type& wd) const
    {
      wd = m_parser.parse_weekday(from, to, m_weekday_format);
      return from;
    }
    //! Expects 1 or 2 digit day range: 1-31
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& /*a_ios*/,
               day_type& d) const
    {
      d = m_parser.parse_var_day_of_month(from, to);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& /*a_ios*/,
               year_type& y) const
    {
      y = m_parser.parse_year(from, to, m_year_format);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               duration_type& dd) const
    {
      // skip leading whitespace
      while(std::isspace(*from) && from != to) { ++from; }

      /* num_get.get() will always consume the first character if it
       * is a sign indicator (+/-). Special value strings may begin
       * with one of these signs so we'll need a copy of it
       * in case num_get.get() fails. */
      char_type c = '\0';
      // TODO Are these characters somewhere in the locale?
      if(*from == '-' || *from == '+') {
        c = *from;
      }
      typedef std::num_get<CharT, InItrT> num_get;
      typename duration_type::duration_rep_type val = 0;
      std::ios_base::iostate err = std::ios_base::goodbit;

      if (std::has_facet<num_get>(a_ios.getloc())) {
        from = std::use_facet<num_get>(a_ios.getloc()).get(from, to, a_ios, err, val);
      }
      else {
        num_get* ng = new num_get();
        std::locale l = std::locale(a_ios.getloc(), ng);
        a_ios.imbue(l);
        from = ng->get(from, to, a_ios, err, val);
      }
      if(err & std::ios_base::failbit){
        typedef typename special_values_parser_type::match_results match_results;
        match_results mr;
        if(c == '-' || c == '+') { // was the first character consumed?
          mr.cache += c;
        }
        m_sv_parser.match(from, to, mr);
        if(mr.current_match == match_results::PARSE_ERROR) {
          boost::throw_exception(std::ios_base::failure("Parse failed. No match found for '" + mr.cache + "'"));
          BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(return from); // should never reach
        }
        dd = duration_type(static_cast<special_values>(mr.current_match));
      }
      else {
        dd = duration_type(val);
      }
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               period_type& p) const
    {
      p = m_period_parser.get_period(from, to, a_ios, p, duration_type::unit(), *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               nth_kday_type& nkd) const
    {
      nkd = m_date_gen_parser.get_nth_kday_type(from, to, a_ios, *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               partial_date_type& pd) const
    {

      pd = m_date_gen_parser.get_partial_date_type(from, to, a_ios, *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               first_kday_type& fkd) const
    {
      fkd = m_date_gen_parser.get_first_kday_type(from, to, a_ios, *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               last_kday_type& lkd) const
    {
      lkd = m_date_gen_parser.get_last_kday_type(from, to, a_ios, *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               kday_before_type& fkb) const
    {
      fkb = m_date_gen_parser.get_kday_before_type(from, to, a_ios, *this);
      return from;
    }
    InItrT get(InItrT& from,
               InItrT& to,
               std::ios_base& a_ios,
               kday_after_type& fka) const
    {
      fka = m_date_gen_parser.get_kday_after_type(from, to, a_ios, *this);
      return from;
    }

  protected:
    string_type                   m_format;
    string_type                   m_month_format;
    string_type                   m_weekday_format;
    string_type                   m_year_format;
    format_date_parser_type       m_parser;
    date_gen_parser_type          m_date_gen_parser;
    period_parser_type            m_period_parser;
    special_values_parser_type    m_sv_parser;
  private:
  };


  template <class date_type, class CharT, class OutItrT>
  std::locale::id date_input_facet<date_type, CharT, OutItrT>::id;

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::long_weekday_format[3] = {'%','A'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::short_weekday_format[3] = {'%','a'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::long_month_format[3] = {'%','B'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::short_month_format[3] = {'%','b'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::four_digit_year_format[3] = {'%','Y'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::two_digit_year_format[3] = {'%','y'};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::default_period_separator[4] = { ' ', '/', ' '};

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::standard_format_specifier[3] =
    {'%', 'x' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::iso_format_specifier[7] =
    {'%', 'Y', '%', 'm', '%', 'd' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::iso_format_extended_specifier[9] =
    {'%', 'Y', '-', '%', 'm', '-', '%', 'd' };

  template <class date_type, class CharT, class OutItrT>
  const typename date_input_facet<date_type, CharT, OutItrT>::char_type
  date_input_facet<date_type, CharT, OutItrT>::default_date_format[9] =
    {'%','Y','-','%','b','-','%','d'};

} } // namespaces

#endif
