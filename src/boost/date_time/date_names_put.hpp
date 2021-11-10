#ifndef DATE_TIME_DATE_NAMES_PUT_HPP___
#define DATE_TIME_DATE_NAMES_PUT_HPP___

/* Copyright (c) 2002-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


#include <boost/date_time/locale_config.hpp> // set BOOST_DATE_TIME_NO_LOCALE

#ifndef BOOST_DATE_TIME_NO_LOCALE

#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/date_defs.hpp>
#include <boost/date_time/parse_format_base.hpp>
#include <boost/lexical_cast.hpp>
#include <locale>


namespace boost {
namespace date_time {

    //! Output facet base class for gregorian dates.
    /*! This class is a base class for date facets used to localize the
     *  names of months and the names of days in the week.
     *
     * Requirements of Config
     *  - define an enumeration month_enum that enumerates the months.
     *    The enumeration should be '1' based eg: Jan==1
     *  - define as_short_string and as_long_string
     *
     * (see langer & kreft p334).
     *
     */
    template<class Config,
             class charT = char,
             class OutputIterator = std::ostreambuf_iterator<charT> >
    class BOOST_SYMBOL_VISIBLE date_names_put : public std::locale::facet
    {
    public:
      date_names_put() {}
      typedef OutputIterator iter_type;
      typedef typename Config::month_type month_type;
      typedef typename Config::month_enum month_enum;
      typedef typename Config::weekday_enum weekday_enum;
      typedef typename Config::special_value_enum special_value_enum;
      //typedef typename Config::format_type format_type;
      typedef std::basic_string<charT> string_type;
      typedef charT char_type;
      static const char_type default_special_value_names[3][17];
      static const char_type separator[2];

      static std::locale::id id;

#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
      std::locale::id& __get_id (void) const { return id; }
#endif

      void put_special_value(iter_type& oitr, special_value_enum sv) const
      {
        do_put_special_value(oitr, sv);
      }
      void put_month_short(iter_type& oitr, month_enum moy) const
      {
        do_put_month_short(oitr, moy);
      }
      void put_month_long(iter_type& oitr, month_enum moy) const
      {
        do_put_month_long(oitr, moy);
      }
      void put_weekday_short(iter_type& oitr, weekday_enum wd) const
      {
        do_put_weekday_short(oitr, wd);
      }
      void put_weekday_long(iter_type& oitr, weekday_enum wd) const
      {
        do_put_weekday_long(oitr, wd);
      }
      bool has_date_sep_chars() const
      {
        return do_has_date_sep_chars();
      }
      void year_sep_char(iter_type& oitr) const
      {
        do_year_sep_char(oitr);
      }
      //! char between year-month
      void month_sep_char(iter_type& oitr) const
      {
        do_month_sep_char(oitr);
      }
      //! Char to separate month-day
      void day_sep_char(iter_type& oitr) const
      {
        do_day_sep_char(oitr);
      }
      //! Determines the order to put the date elements
      ymd_order_spec date_order() const
      {
        return do_date_order();
      }
      //! Determines if month is displayed as integer, short or long string
      month_format_spec month_format() const
      {
        return do_month_format();
      }

    protected:
      //! Default facet implementation uses month_type defaults
      virtual void do_put_month_short(iter_type& oitr, month_enum moy) const
      {
        month_type gm(moy);
        charT c = '\0';
        put_string(oitr, gm.as_short_string(c));
      }
      //! Default facet implementation uses month_type defaults
      virtual void do_put_month_long(iter_type& oitr,
                                     month_enum moy) const
      {
        month_type gm(moy);
        charT c = '\0';
        put_string(oitr, gm.as_long_string(c));
      }
      //! Default facet implementation for special value types
      virtual void do_put_special_value(iter_type& oitr, special_value_enum sv) const
      {
        if(sv <= 2) { // only output not_a_date_time, neg_infin, or pos_infin
          string_type s(default_special_value_names[sv]);
          put_string(oitr, s);
        }
      }
      virtual void do_put_weekday_short(iter_type&, weekday_enum) const
      {
      }
      virtual void do_put_weekday_long(iter_type&, weekday_enum) const
      {
      }
      virtual bool do_has_date_sep_chars() const
      {
        return true;
      }
      virtual void do_year_sep_char(iter_type& oitr) const
      {
        string_type s(separator);
        put_string(oitr, s);
      }
      //! char between year-month
      virtual void do_month_sep_char(iter_type& oitr) const
      {
        string_type s(separator);
        put_string(oitr, s);
      }
      //! Char to separate month-day
      virtual void do_day_sep_char(iter_type& oitr) const
      {
        string_type s(separator); //put in '-'
        put_string(oitr, s);
      }
      //! Default for date order
      virtual ymd_order_spec do_date_order() const
      {
        return ymd_order_iso;
      }
      //! Default month format
      virtual month_format_spec do_month_format() const
      {
        return month_as_short_string;
      }
      void put_string(iter_type& oi, const charT* const s) const
      {
        string_type s1(boost::lexical_cast<string_type>(s));
        typename string_type::iterator si,end;
        for (si=s1.begin(), end=s1.end(); si!=end; si++, oi++) {
          *oi = *si;
        }
      }
      void put_string(iter_type& oi, const string_type& s1) const
      {
        typename string_type::const_iterator si,end;
        for (si=s1.begin(), end=s1.end(); si!=end; si++, oi++) {
          *oi = *si;
        }
      }
    };

    template<class Config, class charT, class OutputIterator>
    const typename date_names_put<Config, charT, OutputIterator>::char_type
    date_names_put<Config, charT, OutputIterator>::default_special_value_names[3][17] = {
      {'n','o','t','-','a','-','d','a','t','e','-','t','i','m','e'},
      {'-','i','n','f','i','n','i','t','y'},
      {'+','i','n','f','i','n','i','t','y'} };

    template<class Config, class charT, class OutputIterator>
    const typename date_names_put<Config, charT, OutputIterator>::char_type
    date_names_put<Config, charT, OutputIterator>::separator[2] =
      {'-', '\0'} ;


    //! Generate storage location for a std::locale::id
    template<class Config, class charT, class OutputIterator>
    std::locale::id date_names_put<Config, charT, OutputIterator>::id;

    //! A date name output facet that takes an array of char* to define strings
    template<class Config,
             class charT = char,
             class OutputIterator = std::ostreambuf_iterator<charT> >
    class BOOST_SYMBOL_VISIBLE all_date_names_put : public date_names_put<Config, charT, OutputIterator>
    {
    public:
      all_date_names_put(const charT* const month_short_names[],
                         const charT* const month_long_names[],
                         const charT* const special_value_names[],
                         const charT* const weekday_short_names[],
                         const charT* const weekday_long_names[],
                         charT separator_char = '-',
                         ymd_order_spec order_spec = ymd_order_iso,
                         month_format_spec month_format = month_as_short_string) :
        month_short_names_(month_short_names),
        month_long_names_(month_long_names),
        special_value_names_(special_value_names),
        weekday_short_names_(weekday_short_names),
        weekday_long_names_(weekday_long_names),
        order_spec_(order_spec),
        month_format_spec_(month_format)
      {
        separator_char_[0] = separator_char;
        separator_char_[1] = '\0';

      }
      typedef OutputIterator iter_type;
      typedef typename Config::month_enum month_enum;
      typedef typename Config::weekday_enum weekday_enum;
      typedef typename Config::special_value_enum special_value_enum;

      const charT* const* get_short_month_names() const
      {
        return month_short_names_;
      }
      const charT* const* get_long_month_names() const
      {
        return month_long_names_;
      }
      const charT* const* get_special_value_names() const
      {
        return special_value_names_;
      }
      const charT* const* get_short_weekday_names()const
      {
        return weekday_short_names_;
      }
      const charT* const* get_long_weekday_names()const
      {
        return weekday_long_names_;
      }

    protected:
      //! Generic facet that takes array of chars
      virtual void do_put_month_short(iter_type& oitr, month_enum moy) const
      {
        this->put_string(oitr, month_short_names_[moy-1]);
      }
      //! Long month names
      virtual void do_put_month_long(iter_type& oitr, month_enum moy) const
      {
        this->put_string(oitr, month_long_names_[moy-1]);
      }
      //! Special values names
      virtual void do_put_special_value(iter_type& oitr, special_value_enum sv) const
      {
        this->put_string(oitr, special_value_names_[sv]);
      }
      virtual void do_put_weekday_short(iter_type& oitr, weekday_enum wd) const
      {
        this->put_string(oitr, weekday_short_names_[wd]);
      }
      virtual void do_put_weekday_long(iter_type& oitr, weekday_enum wd) const
      {
        this->put_string(oitr, weekday_long_names_[wd]);
      }
      //! char between year-month
      virtual void do_month_sep_char(iter_type& oitr) const
      {
        this->put_string(oitr, separator_char_);
      }
      //! Char to separate month-day
      virtual void do_day_sep_char(iter_type& oitr) const
      {
        this->put_string(oitr, separator_char_);
      }
      //! Set the date ordering
      virtual ymd_order_spec do_date_order() const
      {
        return order_spec_;
      }
      //! Set the date ordering
      virtual month_format_spec do_month_format() const
      {
        return month_format_spec_;
      }

    private:
      const charT* const* month_short_names_;
      const charT* const* month_long_names_;
      const charT* const* special_value_names_;
      const charT* const* weekday_short_names_;
      const charT* const* weekday_long_names_;
      charT separator_char_[2];
      ymd_order_spec order_spec_;
      month_format_spec month_format_spec_;
    };

} } //namespace boost::date_time

#endif //BOOST_NO_STD_LOCALE

#endif
