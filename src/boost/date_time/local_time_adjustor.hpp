#ifndef DATE_TIME_LOCAL_TIME_ADJUSTOR_HPP__
#define DATE_TIME_LOCAL_TIME_ADJUSTOR_HPP__

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */

/*! @file local_time_adjustor.hpp
  Time adjustment calculations for local times
*/

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/date_generators.hpp>
#include <boost/date_time/dst_rules.hpp>
#include <boost/date_time/time_defs.hpp> // boost::date_time::dst_flags
#include <boost/date_time/special_defs.hpp> // not_a_date_time

namespace boost {
  namespace date_time {


    //! Provides a base offset adjustment from utc
    template<class time_duration_type, 
             short hours, unsigned short minutes = 0>
    class utc_adjustment 
    {
    public:
      static time_duration_type local_to_utc_base_offset()
      {
        time_duration_type td(hours,minutes,0);
        return td.invert_sign();
      }
      static time_duration_type utc_to_local_base_offset()
      {
        return time_duration_type(hours,minutes,0);
      }
    };



    //! Allow sliding utc adjustment with fixed dst rules
    template<class time_type, class dst_rules>
    class dynamic_local_time_adjustor : public dst_rules
    {
    public:
      typedef typename time_type::time_duration_type time_duration_type;
      typedef typename time_type::date_type date_type;

      dynamic_local_time_adjustor(time_duration_type utc_offset) :
        utc_offset_(utc_offset)
      {}
      
      //! Presumes local time
      time_duration_type utc_offset(bool is_dst) 
      { 
        if (is_dst) {
          return utc_offset_ + this->dst_offset();
        }
        else {
          return utc_offset_;
        }

      }
    private:
      time_duration_type utc_offset_;

    };



    //! Embed the rules for local time adjustments at compile time
    template<class time_type, class dst_rules, class utc_offset_rules>
    class static_local_time_adjustor: public dst_rules, public utc_offset_rules
    {
    public:
      typedef typename time_type::time_duration_type time_duration_type;
      typedef typename time_type::date_type date_type;

      //! Calculates the offset from a utc time to local based on dst and utc offset
      /*! @param t UTC time to calculate offset to local time
       *  This adjustment depends on the following observations about the
       *  workings of the DST boundary offset.  Since UTC time labels are
       *  monotonically increasing we can determine if a given local time
       *  is in DST or not and therefore adjust the offset appropriately.
       * 
       *  The logic is as follows.  Starting with UTC time use the offset to
       *  create a label for an non-dst adjusted local time.  Then call
       *  dst_rules::local_is_dst with the non adjust local time.  The
       *  results of this function will either unabiguously decide that
       *  the initial local time is in dst or return an illegal or
       *  ambiguous result.  An illegal result only occurs at the end
       *  of dst (where labels are skipped) and indicates that dst has
       *  ended.  An ambiguous result means that we need to recheck by
       *  making a dst adjustment and then rechecking.  If the dst offset
       *  is added to the utc time and the recheck proves non-ambiguous
       *  then we are past the boundary.  If it is still ambiguous then
       *  we are ahead of the boundary and dst is still in effect.
       *
       *  TODO -- check if all dst offsets are positive.  If not then
       *  the algorithm needs to check for this and reverse the 
       *  illegal/ambiguous logic.
       */
      static time_duration_type utc_to_local_offset(const time_type& t)
      {
        //get initial local time guess by applying utc offset
        time_type initial = t + utc_offset_rules::utc_to_local_base_offset();
        time_is_dst_result dst_flag = 
          dst_rules::local_is_dst(initial.date(), initial.time_of_day());
        switch(dst_flag) {
        case is_in_dst:        return utc_offset_rules::utc_to_local_base_offset() + dst_rules::dst_offset();
        case is_not_in_dst:    return utc_offset_rules::utc_to_local_base_offset();
        case invalid_time_label:return utc_offset_rules::utc_to_local_base_offset() + dst_rules::dst_offset();
        case ambiguous: {
          time_type retry = initial + dst_rules::dst_offset();
          dst_flag = dst_rules::local_is_dst(retry.date(), retry.time_of_day());
          //if still ambibuous then the utc time still translates to a dst time
          if (dst_flag == ambiguous) {
            return utc_offset_rules::utc_to_local_base_offset() + dst_rules::dst_offset();
          }
          // we are past the dst boundary
          else {
            return utc_offset_rules::utc_to_local_base_offset();
          }
        }
        }//case
        //TODO  better exception type
        boost::throw_exception(std::out_of_range("Unreachable case"));
        BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(return time_duration_type(not_a_date_time)); // should never reach
      }

      //! Get the offset to UTC given a local time
      static time_duration_type local_to_utc_offset(const time_type& t, 
                                                    date_time::dst_flags dst=date_time::calculate) 
      {
        switch (dst) {
        case is_dst:
          return utc_offset_rules::local_to_utc_base_offset() - dst_rules::dst_offset();
        case not_dst:
          return utc_offset_rules::local_to_utc_base_offset();
        case calculate:
          time_is_dst_result res = 
            dst_rules::local_is_dst(t.date(), t.time_of_day());
          switch(res) {
          case is_in_dst:      return utc_offset_rules::local_to_utc_base_offset() - dst_rules::dst_offset();
          case is_not_in_dst:      return utc_offset_rules::local_to_utc_base_offset();
          case ambiguous:          return utc_offset_rules::local_to_utc_base_offset();
          case invalid_time_label: break;
          }
        }
        boost::throw_exception(std::out_of_range("Time label invalid"));
        BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(return time_duration_type(not_a_date_time)); // should never reach
      }


    private:

    };

    void dummy_to_prevent_msvc6_ice(); //why ask why?

    //! Template that simplifies the creation of local time calculator 
    /*! Use this template to create the timezone to utc convertors as required.
     * 
     *  This class will also work for other regions that don't use dst and
     *  have a utc offset which is an integral number of hours.
     *
     *  <b>Template Parameters</b>
     *  -time_type  -- Time class to use
     *  -utc_offset -- Number hours local time is adjust from utc
     *  -use_dst -- true (default) if region uses dst, false otherwise
     *  For example:
     *  @code
     *  //eastern timezone is utc-5
     typedef date_time::local_adjustor<ptime, -5, us_dst> us_eastern;
     typedef date_time::local_adjustor<ptime, -6, us_dst> us_central;
     typedef date_time::local_adjustor<ptime, -7, us_dst> us_mountain;
     typedef date_time::local_adjustor<ptime, -8, us_dst> us_pacific;
     typedef date_time::local_adjustor<ptime, -7, no_dst> us_arizona;
     @endcode
      
    */
    template<class time_type, short utc_offset, class dst_rule>
    class local_adjustor
    {
    public:
      typedef typename time_type::time_duration_type time_duration_type;
      typedef typename time_type::date_type date_type;
      typedef static_local_time_adjustor<time_type, 
                                         dst_rule,
                                         utc_adjustment<time_duration_type, 
                                                        utc_offset> > dst_adjustor;
      //! Convert a utc time to local time
      static time_type utc_to_local(const time_type& t)
      {
        time_duration_type td = dst_adjustor::utc_to_local_offset(t);
        return t + td;
      }
      //! Convert a local time to utc
      static time_type local_to_utc(const time_type& t, 
                                    date_time::dst_flags dst=date_time::calculate)
      {
        time_duration_type td = dst_adjustor::local_to_utc_offset(t, dst);
        return t + td;
      }
    };


  } } //namespace date_time



#endif
