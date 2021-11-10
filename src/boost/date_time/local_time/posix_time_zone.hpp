#ifndef _DATE_TIME_POSIX_TIME_ZONE__
#define _DATE_TIME_POSIX_TIME_ZONE__

/* Copyright (c) 2003-2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <string>
#include <sstream>
#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <boost/throw_exception.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/time_zone_names.hpp>
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/local_time/dst_transition_day_rules.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/string_convert.hpp>
#include <boost/date_time/time_parsing.hpp>

namespace boost{
namespace local_time{

  //! simple exception for UTC and Daylight savings start/end offsets
  struct BOOST_SYMBOL_VISIBLE bad_offset : public std::out_of_range
  {
    bad_offset(std::string const& msg = std::string()) :
      std::out_of_range(std::string("Offset out of range: " + msg)) {}
  };
  //! simple exception for UTC daylight savings adjustment
  struct BOOST_SYMBOL_VISIBLE bad_adjustment : public std::out_of_range
  {
    bad_adjustment(std::string const& msg = std::string()) :
      std::out_of_range(std::string("Adjustment out of range: " + msg)) {}
  };

  typedef boost::date_time::dst_adjustment_offsets<boost::posix_time::time_duration> dst_adjustment_offsets;

  //! A time zone class constructed from a POSIX time zone string
  /*! A POSIX time zone string takes the form of:<br>
   * "std offset dst [offset],start[/time],end[/time]" (w/no spaces)
   * 'std' specifies the abbrev of the time zone.<br>
   * 'offset' is the offset from UTC.<br>
   * 'dst' specifies the abbrev of the time zone during daylight savings time.<br>
   * The second offset is how many hours changed during DST. Default=1<br>
   * 'start' and'end' are the dates when DST goes into (and out of) effect.<br>
   * 'offset' takes the form of: [+|-]hh[:mm[:ss]] {h=0-23, m/s=0-59}<br>
   * 'time' and 'offset' take the same form. Time defaults=02:00:00<br>
   * 'start' and 'end' can be one of three forms:<br>
   * Mm.w.d {month=1-12, week=1-5 (5 is always last), day=0-6}<br>
   * Jn {n=1-365 Feb29 is never counted}<br>
   * n  {n=0-365 Feb29 is counted in leap years}<br>
   * Example "PST-5PDT01:00:00,M4.1.0/02:00:00,M10.1.0/02:00:00"
   * <br>
   * Exceptions will be thrown under these conditions:<br>
   * An invalid date spec (see date class)<br>
   * A boost::local_time::bad_offset exception will be thrown for:<br>
   * A DST start or end offset that is negative or more than 24 hours<br>
   * A UTC zone that is greater than +14 or less than -12 hours<br>
   * A boost::local_time::bad_adjustment exception will be thrown for:<br>
   * A DST adjustment that is 24 hours or more (positive or negative)<br>
   *
   * Note that UTC zone offsets can be greater than +12:
   * http://www.worldtimezone.com/utc/utc+1200.html
   */
  template<class CharT>
  class BOOST_SYMBOL_VISIBLE posix_time_zone_base : public date_time::time_zone_base<posix_time::ptime,CharT> {
  public:
    typedef boost::posix_time::time_duration time_duration_type;
    typedef date_time::time_zone_names_base<CharT> time_zone_names;
    typedef date_time::time_zone_base<posix_time::ptime,CharT> base_type;
    typedef typename base_type::string_type string_type;
    typedef CharT char_type;
    typedef typename base_type::stringstream_type stringstream_type;
    typedef boost::char_separator<char_type, std::char_traits<char_type> > char_separator_type;
    typedef boost::tokenizer<char_separator_type,
                             typename string_type::const_iterator,
                             string_type> tokenizer_type;
    typedef typename tokenizer_type::iterator tokenizer_iterator_type;

    //! Construct from a POSIX time zone string
    posix_time_zone_base(const string_type& s) :
      //zone_names_("std_name","std_abbrev","no-dst","no-dst"),
      zone_names_(),
      has_dst_(false),
      base_utc_offset_(posix_time::hours(0)),
      dst_offsets_(posix_time::hours(0),posix_time::hours(0),posix_time::hours(0)),
      dst_calc_rules_()
    {
#ifdef __HP_aCC
      // Work around bug in aC++ compiler: see QXCR1000880488 in the
      // HP bug tracking system
      const char_type sep_chars[2] = {',',0};
#else
      const char_type sep_chars[2] = {','};
#endif
      char_separator_type sep(sep_chars);
      tokenizer_type tokens(s, sep);
      tokenizer_iterator_type it = tokens.begin(), end = tokens.end();
      if (it == end)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Could not parse time zone name"));
      calc_zone(*it++);
      if(has_dst_)
      {
        if (it == end)
          BOOST_THROW_EXCEPTION(std::invalid_argument("Could not parse DST begin time"));
        string_type dst_begin = *it++;

        if (it == end)
          BOOST_THROW_EXCEPTION(std::invalid_argument("Could not parse DST end time"));
        string_type dst_end = *it;
        calc_rules(dst_begin, dst_end);
      }
    }
    virtual ~posix_time_zone_base() {}
    //!String for the zone when not in daylight savings (eg: EST)
    virtual string_type std_zone_abbrev()const
    {
      return zone_names_.std_zone_abbrev();
    }
    //!String for the timezone when in daylight savings (eg: EDT)
    /*! For those time zones that have no DST, an empty string is used */
    virtual string_type dst_zone_abbrev() const
    {
      return zone_names_.dst_zone_abbrev();
    }
    //!String for the zone when not in daylight savings (eg: Eastern Standard Time)
    /*! The full STD name is not extracted from the posix time zone string.
     * Therefore, the STD abbreviation is used in it's place */
    virtual string_type std_zone_name()const
    {
      return zone_names_.std_zone_name();
    }
    //!String for the timezone when in daylight savings (eg: Eastern Daylight Time)
    /*! The full DST name is not extracted from the posix time zone string.
     * Therefore, the STD abbreviation is used in it's place. For time zones
     * that have no DST, an empty string is used */
    virtual string_type dst_zone_name()const
    {
      return zone_names_.dst_zone_name();
    }
    //! True if zone uses daylight savings adjustments otherwise false
    virtual bool has_dst()const
    {
      return has_dst_;
    }
    //! Local time that DST starts -- NADT if has_dst is false
    virtual posix_time::ptime dst_local_start_time(gregorian::greg_year y)const
    {
      gregorian::date d(gregorian::not_a_date_time);
      if(has_dst_)
      {
        d = dst_calc_rules_->start_day(y);
      }
      return posix_time::ptime(d, dst_offsets_.dst_start_offset_);
    }
    //! Local time that DST ends -- NADT if has_dst is false
    virtual posix_time::ptime dst_local_end_time(gregorian::greg_year y)const
    {
      gregorian::date d(gregorian::not_a_date_time);
      if(has_dst_)
      {
        d = dst_calc_rules_->end_day(y);
      }
      return posix_time::ptime(d, dst_offsets_.dst_end_offset_);
    }
    //! Base offset from UTC for zone (eg: -07:30:00)
    virtual time_duration_type base_utc_offset()const
    {
      return base_utc_offset_;
    }
    //! Adjustment forward or back made while DST is in effect
    virtual time_duration_type dst_offset()const
    {
      return dst_offsets_.dst_adjust_;
    }

    //! Returns a POSIX time_zone string for this object
    virtual string_type to_posix_string() const
    {
      // std offset dst [offset],start[/time],end[/time] - w/o spaces
      stringstream_type ss;
      ss.fill('0');
      boost::shared_ptr<dst_calc_rule> no_rules;
      // std
      ss << std_zone_abbrev();
      // offset
      if(base_utc_offset().is_negative()) {
        // inverting the sign guarantees we get two digits
        ss << '-' << std::setw(2) << base_utc_offset().invert_sign().hours();
      }
      else {
        ss << '+' << std::setw(2) << base_utc_offset().hours();
      }
      if(base_utc_offset().minutes() != 0 || base_utc_offset().seconds() != 0) {
        ss << ':' << std::setw(2) << base_utc_offset().minutes();
        if(base_utc_offset().seconds() != 0) {
          ss << ':' << std::setw(2) << base_utc_offset().seconds();
        }
      }
      if(dst_calc_rules_ != no_rules) {
        // dst
        ss << dst_zone_abbrev();
        // dst offset
        if(dst_offset().is_negative()) {
        // inverting the sign guarantees we get two digits
          ss << '-' << std::setw(2) << dst_offset().invert_sign().hours();
        }
        else {
          ss << '+' << std::setw(2) << dst_offset().hours();
        }
        if(dst_offset().minutes() != 0 || dst_offset().seconds() != 0) {
          ss << ':' << std::setw(2) << dst_offset().minutes();
          if(dst_offset().seconds() != 0) {
            ss << ':' << std::setw(2) << dst_offset().seconds();
          }
        }
        // start/time
        ss << ',' << date_time::convert_string_type<char, char_type>(dst_calc_rules_->start_rule_as_string()) << '/'
           << std::setw(2) << dst_offsets_.dst_start_offset_.hours() << ':'
           << std::setw(2) << dst_offsets_.dst_start_offset_.minutes();
        if(dst_offsets_.dst_start_offset_.seconds() != 0) {
          ss << ':' << std::setw(2) << dst_offsets_.dst_start_offset_.seconds();
        }
        // end/time
        ss << ',' << date_time::convert_string_type<char, char_type>(dst_calc_rules_->end_rule_as_string()) << '/'
           << std::setw(2) << dst_offsets_.dst_end_offset_.hours() << ':'
           << std::setw(2) << dst_offsets_.dst_end_offset_.minutes();
        if(dst_offsets_.dst_end_offset_.seconds() != 0) {
          ss << ':' << std::setw(2) << dst_offsets_.dst_end_offset_.seconds();
        }
      }

      return ss.str();
    }
  private:
    time_zone_names zone_names_;
    bool has_dst_;
    time_duration_type base_utc_offset_;
    dst_adjustment_offsets dst_offsets_;
    boost::shared_ptr<dst_calc_rule> dst_calc_rules_;

    /*! Extract time zone abbreviations for STD & DST as well
     * as the offsets for the time shift that occurs and how
     * much of a shift. At this time full time zone names are
     * NOT extracted so the abbreviations are used in their place */
    void calc_zone(const string_type& obj){
      const char_type empty_string[2] = {'\0'};
      stringstream_type ss(empty_string);
      typename string_type::const_pointer sit = obj.c_str(), obj_end = sit + obj.size();
      string_type l_std_zone_abbrev, l_dst_zone_abbrev;

      // get 'std' name/abbrev
      while(std::isalpha(*sit)){
        ss << *sit++;
      }
      l_std_zone_abbrev = ss.str();
      ss.str(empty_string);

      // get UTC offset
      if(sit != obj_end){
        // get duration
        while(sit != obj_end && !std::isalpha(*sit)){
          ss << *sit++;
        }
        base_utc_offset_ = date_time::str_from_delimited_time_duration<time_duration_type,char_type>(ss.str());
        ss.str(empty_string);

        // base offset must be within range of -12 hours to +14 hours
        if(base_utc_offset_ < time_duration_type(-12,0,0) ||
          base_utc_offset_ > time_duration_type(14,0,0))
        {
          boost::throw_exception(bad_offset(posix_time::to_simple_string(base_utc_offset_)));
        }
      }

      // get DST data if given
      if(sit != obj_end){
        has_dst_ = true;

        // get 'dst' name/abbrev
        while(sit != obj_end && std::isalpha(*sit)){
          ss << *sit++;
        }
        l_dst_zone_abbrev = ss.str();
        ss.str(empty_string);

        // get DST offset if given
        if(sit != obj_end){
          // get duration
          while(sit != obj_end && !std::isalpha(*sit)){
            ss << *sit++;
          }
          dst_offsets_.dst_adjust_ = date_time::str_from_delimited_time_duration<time_duration_type,char_type>(ss.str());
          ss.str(empty_string);
        }
        else{ // default DST offset
          dst_offsets_.dst_adjust_ = posix_time::hours(1);
        }

        // adjustment must be within +|- 1 day
        if(dst_offsets_.dst_adjust_ <= time_duration_type(-24,0,0) ||
            dst_offsets_.dst_adjust_ >= time_duration_type(24,0,0))
        {
          boost::throw_exception(bad_adjustment(posix_time::to_simple_string(dst_offsets_.dst_adjust_)));
        }
      }
      // full names not extracted so abbrevs used in their place
      zone_names_ = time_zone_names(l_std_zone_abbrev, l_std_zone_abbrev, l_dst_zone_abbrev, l_dst_zone_abbrev);
    }

    void calc_rules(const string_type& start, const string_type& end){
#ifdef __HP_aCC
      // Work around bug in aC++ compiler: see QXCR1000880488 in the
      // HP bug tracking system
      const char_type sep_chars[2] = {'/',0};
#else
      const char_type sep_chars[2] = {'/'};
#endif
      char_separator_type sep(sep_chars);
      tokenizer_type st_tok(start, sep);
      tokenizer_type et_tok(end, sep);
      tokenizer_iterator_type sit = st_tok.begin();
      tokenizer_iterator_type eit = et_tok.begin();

      // generate date spec
      char_type x = string_type(*sit).at(0);
      if(x == 'M'){
        M_func(*sit, *eit);
      }
      else if(x == 'J'){
        julian_no_leap(*sit, *eit);
      }
      else{
        julian_day(*sit, *eit);
      }

      ++sit;
      ++eit;
      // generate durations
      // starting offset
      if(sit != st_tok.end()){
        dst_offsets_.dst_start_offset_ =  date_time::str_from_delimited_time_duration<time_duration_type,char_type>(*sit);
      }
      else{
        // default
        dst_offsets_.dst_start_offset_ = posix_time::hours(2);
      }
      // start/end offsets must fall on given date
      if(dst_offsets_.dst_start_offset_ < time_duration_type(0,0,0) ||
          dst_offsets_.dst_start_offset_ >= time_duration_type(24,0,0))
      {
        boost::throw_exception(bad_offset(posix_time::to_simple_string(dst_offsets_.dst_start_offset_)));
      }

      // ending offset
      if(eit != et_tok.end()){
        dst_offsets_.dst_end_offset_ =  date_time::str_from_delimited_time_duration<time_duration_type,char_type>(*eit);
      }
      else{
        // default
        dst_offsets_.dst_end_offset_ = posix_time::hours(2);
      }
      // start/end offsets must fall on given date
      if(dst_offsets_.dst_end_offset_ < time_duration_type(0,0,0) ||
        dst_offsets_.dst_end_offset_ >= time_duration_type(24,0,0))
      {
        boost::throw_exception(bad_offset(posix_time::to_simple_string(dst_offsets_.dst_end_offset_)));
      }
    }

    /* Parses out a start/end date spec from a posix time zone string.
     * Date specs come in three possible formats, this function handles
     * the 'M' spec. Ex "M2.2.4" => 2nd month, 2nd week, 4th day .
     */
    void M_func(const string_type& s, const string_type& e){
      typedef gregorian::nth_kday_of_month nkday;
      unsigned short sm=0,sw=0,sd=0,em=0,ew=0,ed=0; // start/end month,week,day
#ifdef __HP_aCC
      // Work around bug in aC++ compiler: see QXCR1000880488 in the
      // HP bug tracking system
      const char_type sep_chars[3] = {'M','.',0};
#else
      const char_type sep_chars[3] = {'M','.'};
#endif
      char_separator_type sep(sep_chars);
      tokenizer_type stok(s, sep), etok(e, sep);

      tokenizer_iterator_type it = stok.begin();
      sm = lexical_cast<unsigned short>(*it++);
      sw = lexical_cast<unsigned short>(*it++);
      sd = lexical_cast<unsigned short>(*it);

      it = etok.begin();
      em = lexical_cast<unsigned short>(*it++);
      ew = lexical_cast<unsigned short>(*it++);
      ed = lexical_cast<unsigned short>(*it);

      dst_calc_rules_ = shared_ptr<dst_calc_rule>(
        new nth_kday_dst_rule(
          nth_last_dst_rule::start_rule(
            static_cast<nkday::week_num>(sw),sd,sm),
          nth_last_dst_rule::start_rule(
            static_cast<nkday::week_num>(ew),ed,em)
          )
      );
    }

    //! Julian day. Feb29 is never counted, even in leap years
    // expects range of 1-365
    void julian_no_leap(const string_type& s, const string_type& e){
      typedef gregorian::gregorian_calendar calendar;
      const unsigned short year = 2001; // Non-leap year
      unsigned short sm=1;
      int sd=0;
      sd = lexical_cast<int>(s.substr(1)); // skip 'J'
      while(sd >= calendar::end_of_month_day(year,sm)){
        sd -= calendar::end_of_month_day(year,sm++);
      }
      unsigned short em=1;
      int ed=0;
      ed = lexical_cast<int>(e.substr(1)); // skip 'J'
      while(ed > calendar::end_of_month_day(year,em)){
        ed -= calendar::end_of_month_day(year,em++);
      }

      dst_calc_rules_ = shared_ptr<dst_calc_rule>(
        new partial_date_dst_rule(
          partial_date_dst_rule::start_rule(
            static_cast<unsigned short>(sd), static_cast<date_time::months_of_year>(sm)),
          partial_date_dst_rule::end_rule(
            static_cast<unsigned short>(ed), static_cast<date_time::months_of_year>(em))
          )
      );
    }

    //! Julian day. Feb29 is always counted, but exception thrown in non-leap years
    // expects range of 0-365
    void julian_day(const string_type& s, const string_type& e){
      int sd=0, ed=0;
      sd = lexical_cast<int>(s);
      ed = lexical_cast<int>(e);
      dst_calc_rules_ = shared_ptr<dst_calc_rule>(
        new partial_date_dst_rule(
          partial_date_dst_rule::start_rule(++sd),// args are 0-365
          partial_date_dst_rule::end_rule(++ed) // pd expects 1-366
          )
      );
    }

    //! helper function used when throwing exceptions
    static std::string td_as_string(const time_duration_type& td)
    {
      std::string s;
#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO)
      s = posix_time::to_simple_string(td);
#else
      std::stringstream ss;
      ss << td;
      s = ss.str();
#endif
      return s;
    }
  };

  typedef posix_time_zone_base<char> posix_time_zone;

} } // namespace boost::local_time


#endif // _DATE_TIME_POSIX_TIME_ZONE__
