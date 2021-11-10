#ifndef POSIX_PTIME_HPP___
#define POSIX_PTIME_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 * $Date$
 */

#include <boost/date_time/posix_time/posix_time_system.hpp>
#include <boost/date_time/time.hpp>
#include <boost/date_time/compiler_config.hpp>

namespace boost {

namespace posix_time {
 
  //bring special enum values into the namespace
  using date_time::special_values;
  using date_time::not_special;
  using date_time::neg_infin;
  using date_time::pos_infin;
  using date_time::not_a_date_time;
  using date_time::max_date_time;
  using date_time::min_date_time; 

  //! Time type with no timezone or other adjustments
  /*! \ingroup time_basics
   */
  class BOOST_SYMBOL_VISIBLE ptime : public date_time::base_time<ptime, posix_time_system>
  {
  public:
    typedef posix_time_system time_system_type;
    typedef time_system_type::time_rep_type time_rep_type;
    typedef time_system_type::time_duration_type time_duration_type;
    typedef ptime time_type;
    //! Construct with date and offset in day
    BOOST_CXX14_CONSTEXPR 
    ptime(gregorian::date d,time_duration_type td) :
      date_time::base_time<time_type,time_system_type>(d,td)
    {}
    //! Construct a time at start of the given day (midnight)
    BOOST_CXX14_CONSTEXPR 
    explicit ptime(gregorian::date d) :
      date_time::base_time<time_type,time_system_type>(d,time_duration_type(0,0,0))
    {}
    //! Copy from time_rep
    BOOST_CXX14_CONSTEXPR 
    ptime(const time_rep_type& rhs):
      date_time::base_time<time_type,time_system_type>(rhs)
    {}
    //! Construct from special value
    BOOST_CXX14_CONSTEXPR 
    ptime(const special_values sv) :
      date_time::base_time<time_type,time_system_type>(sv)
    {}
#if !defined(DATE_TIME_NO_DEFAULT_CONSTRUCTOR)
    // Default constructor constructs to not_a_date_time
    BOOST_CXX14_CONSTEXPR 
    ptime() :
      date_time::base_time<time_type,time_system_type>(gregorian::date(not_a_date_time),
                                                       time_duration_type(not_a_date_time))
    {}
#endif // DATE_TIME_NO_DEFAULT_CONSTRUCTOR

    friend BOOST_CXX14_CONSTEXPR
    bool operator==(const ptime& lhs, const ptime& rhs);

  };

  inline BOOST_CXX14_CONSTEXPR
  bool operator==(const ptime& lhs, const ptime& rhs)
  {
    return ptime::time_system_type::is_equal(lhs.time_,rhs.time_);
  }


} }//namespace posix_time


#endif

