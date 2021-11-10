//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_TIMEPOINT_TO_TIMESPEC_HPP
#define BOOST_INTERPROCESS_DETAIL_TIMEPOINT_TO_TIMESPEC_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/timed_utils.hpp>

namespace boost {

namespace interprocess {

namespace ipcdetail {

template<class TimePoint>
inline timespec timepoint_to_timespec ( const TimePoint &tm
                                  , typename enable_if_ptime<TimePoint>::type * = 0)
{
   typedef typename TimePoint::date_type            date_type;
   typedef typename TimePoint::time_duration_type   time_duration_type;

   const TimePoint epoch(date_type(1970,1,1));

   //Avoid negative absolute times
   time_duration_type duration  = (tm <= epoch) ? time_duration_type(epoch - epoch)
                                                : time_duration_type(tm - epoch);
   timespec ts;
   ts.tv_sec  = duration.total_seconds();
   ts.tv_nsec = duration.total_nanoseconds() % 1000000000;
   return ts;
}

inline timespec timepoint_to_timespec (const ustime &tm)
{
   timespec ts;
   ts.tv_sec  = tm.get_microsecs()/1000000u;
   ts.tv_nsec = (tm.get_microsecs()%1000000u)*1000u;
   return ts;
}

template<class TimePoint>
inline timespec timepoint_to_timespec ( const TimePoint &tm
                                       , typename enable_if_time_point<TimePoint>::type * = 0)
{
   typedef typename TimePoint::duration duration_t;
   duration_t d(tm.time_since_epoch());

   timespec ts;
   BOOST_IF_CONSTEXPR(duration_t::period::num == 1 && duration_t::period::den == 1000000000)
   {
      ts.tv_sec  = d.count()/duration_t::period::den;
      ts.tv_nsec = d.count()%duration_t::period::den;
   }
   else
   {
      const double factor = double(duration_t::period::num)/double(duration_t::period::den);
      const double res = d.count()*factor;
      ts.tv_sec  = static_cast<boost::uint64_t>(res);
      ts.tv_nsec = static_cast<boost::uint64_t>(res - double(ts.tv_sec));
   }
   return ts;
}

}  //namespace ipcdetail {

}  //namespace interprocess {

}  //namespace boost {

#endif   //ifndef BOOST_INTERPROCESS_DETAIL_TIMEPOINT_TO_TIMESPEC_HPP
