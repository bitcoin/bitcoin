#ifndef GREG_DURATION_TYPES_HPP___
#define GREG_DURATION_TYPES_HPP___

/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Subject to Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/int_adapter.hpp>
#include <boost/date_time/adjust_functors.hpp>
#include <boost/date_time/date_duration_types.hpp>
#include <boost/date_time/gregorian/greg_duration.hpp>

namespace boost {
namespace gregorian {

  //! config struct for additional duration types (ie months_duration<> & years_duration<>)
  struct BOOST_SYMBOL_VISIBLE greg_durations_config {
    typedef date date_type;
    typedef date_time::int_adapter<int> int_rep;
    typedef date_time::month_functor<date_type> month_adjustor_type; 
  };

  typedef date_time::months_duration<greg_durations_config> months;
  typedef date_time::years_duration<greg_durations_config> years;

  class BOOST_SYMBOL_VISIBLE weeks_duration : public date_duration {
  public:
    BOOST_CXX14_CONSTEXPR weeks_duration(duration_rep w) 
      : date_duration(w * 7) {}
    BOOST_CXX14_CONSTEXPR weeks_duration(date_time::special_values sv) 
      : date_duration(sv) {}
  };

  typedef weeks_duration weeks;

}} // namespace boost::gregorian

#endif // GREG_DURATION_TYPES_HPP___
