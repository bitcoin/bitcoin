/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */
#ifndef DATE_TIME_DATE_DST_TRANSITION_DAY_GEN_HPP__
#define DATE_TIME_DATE_DST_TRANSITION_DAY_GEN_HPP__

#include <string>

namespace boost {
namespace date_time {

    //! Defines base interface for calculating start and end date of daylight savings 
    template<class date_type>
    class dst_day_calc_rule 
    {
    public:
      typedef typename date_type::year_type year_type;
      virtual ~dst_day_calc_rule() {}
      virtual date_type start_day(year_type y) const=0;
      virtual std::string start_rule_as_string() const=0;
      virtual date_type end_day(year_type y) const=0;
      virtual std::string end_rule_as_string() const=0;

    };

    //! Canonical form for a class that provides day rule calculation
    /*! This class is used to generate specific sets of dst rules
     *  
     *@tparam spec Provides a specifiction of the function object types used
     *            to generate start and end days of daylight savings as well
     *            as the date type.
     */
    template<class spec>
    class day_calc_dst_rule : public dst_day_calc_rule<typename spec::date_type>
    {
    public:
      typedef typename spec::date_type date_type;
      typedef typename date_type::year_type year_type;
      typedef typename spec::start_rule start_rule;
      typedef typename spec::end_rule  end_rule;
      day_calc_dst_rule(start_rule dst_start,
                        end_rule dst_end) :
        dst_start_(dst_start),
        dst_end_(dst_end)
      {}
      virtual date_type start_day(year_type y) const
      {
        return dst_start_.get_date(y);
      }
      virtual std::string start_rule_as_string() const
      {
        return dst_start_.to_string();
      }
      virtual date_type end_day(year_type y) const
      {
        return dst_end_.get_date(y);
      }
      virtual std::string end_rule_as_string() const
      {
        return dst_end_.to_string();
      }
    private:
      start_rule dst_start_;
      end_rule dst_end_;
    };


} }//namespace



#endif
