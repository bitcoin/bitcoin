#ifndef LOCAL_TIME_CUSTOM_TIME_ZONE_HPP__
#define LOCAL_TIME_CUSTOM_TIME_ZONE_HPP__

/* Copyright (c) 2003-2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include "boost/date_time/time_zone_base.hpp"
#include "boost/date_time/time_zone_names.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/dst_transition_day_rules.hpp"
#include "boost/date_time/string_convert.hpp"
//#include "boost/date_time/special_defs.hpp"
#include "boost/shared_ptr.hpp"

namespace boost {
namespace local_time {

  //typedef boost::date_time::time_zone_names time_zone_names;
  typedef boost::date_time::dst_adjustment_offsets<boost::posix_time::time_duration> dst_adjustment_offsets;
  //typedef boost::date_time::time_zone_base<boost::posix_time::ptime> time_zone;
  typedef boost::shared_ptr<dst_calc_rule> dst_calc_rule_ptr;

  //! A real time zone
  template<class CharT>
  class custom_time_zone_base : public date_time::time_zone_base<posix_time::ptime,CharT> {
  public:
    typedef boost::posix_time::time_duration time_duration_type;
    typedef date_time::time_zone_base<posix_time::ptime,CharT> base_type;
    typedef typename base_type::string_type string_type;
    typedef typename base_type::stringstream_type stringstream_type;
    typedef date_time::time_zone_names_base<CharT> time_zone_names;
    typedef CharT char_type;

    custom_time_zone_base(const time_zone_names& zone_names,
                     const time_duration_type& utc_offset,
                     const dst_adjustment_offsets& dst_shift,
                     boost::shared_ptr<dst_calc_rule> calc_rule) :
      zone_names_(zone_names),
      base_utc_offset_(utc_offset),
      dst_offsets_(dst_shift),
      dst_calc_rules_(calc_rule)
    {}
    virtual ~custom_time_zone_base() {}
    virtual string_type dst_zone_abbrev() const
    {
      return zone_names_.dst_zone_abbrev();
    }
    virtual string_type std_zone_abbrev() const
    {
      return zone_names_.std_zone_abbrev();
    }
    virtual string_type dst_zone_name() const
    {
      return zone_names_.dst_zone_name();
    }
    virtual string_type std_zone_name() const
    {
      return zone_names_.std_zone_name();
    }
    //! True if zone uses daylight savings adjustments
    virtual bool has_dst() const
    {
      return (bool) dst_calc_rules_; //if calc_rule is set the tz has dst
    }
    //! Local time that DST starts -- NADT if has_dst is false
    virtual posix_time::ptime dst_local_start_time(gregorian::greg_year y) const
    {
      gregorian::date d(gregorian::not_a_date_time);
      if (dst_calc_rules_) {
        d = dst_calc_rules_->start_day(y);
      }
      return posix_time::ptime(d, dst_offsets_.dst_start_offset_);
    }
    //! Local time that DST ends -- NADT if has_dst is false
    virtual posix_time::ptime dst_local_end_time(gregorian::greg_year y) const
    {
      gregorian::date d(gregorian::not_a_date_time);
      if (dst_calc_rules_) {
        d = dst_calc_rules_->end_day(y);
      }
      return posix_time::ptime(d, dst_offsets_.dst_end_offset_);
    }
    //! Base offset from UTC for zone (eg: -07:30:00)
    virtual time_duration_type base_utc_offset() const
    {
      return base_utc_offset_;
    }
    //! Adjustment forward or back made while DST is in effect
    virtual time_duration_type dst_offset() const
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
    time_duration_type base_utc_offset_;
    dst_adjustment_offsets dst_offsets_;
    boost::shared_ptr<dst_calc_rule> dst_calc_rules_;
  };

  typedef custom_time_zone_base<char> custom_time_zone;

} }//namespace



#endif
