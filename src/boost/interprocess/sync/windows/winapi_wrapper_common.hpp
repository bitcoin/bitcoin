 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_WINAPI_WRAPPER_COMMON_HPP
#define BOOST_INTERPROCESS_DETAIL_WINAPI_WRAPPER_COMMON_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/win32_api.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/timed_utils.hpp>
#include <limits>

namespace boost {
namespace interprocess {
namespace ipcdetail {

inline bool do_winapi_wait(void *handle, unsigned long dwMilliseconds)
{
   unsigned long ret = winapi::wait_for_single_object(handle, dwMilliseconds);
   if(ret == winapi::wait_object_0){
      return true;
   }
   else if(ret == winapi::wait_timeout){
      return false;
   }
   else if(ret == winapi::wait_abandoned){ //Special case for orphaned mutexes
      winapi::release_mutex(handle);
      throw interprocess_exception(owner_dead_error);
   }
   else{
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

template<class TimePoint>
inline bool winapi_wrapper_timed_wait_for_single_object(void *handle, const TimePoint &abs_time)
{
   //Windows uses relative wait times so check for negative waits
   //and implement as 0 wait to allow try-semantics as POSIX mandates.
   unsigned long time_ms = 0u;
   if (ipcdetail::is_pos_infinity(abs_time)){
      time_ms = winapi::infinite_time;
   }
   else {
      const TimePoint cur_time = microsec_clock<TimePoint>::universal_time();
      if(abs_time > cur_time){
         time_ms = static_cast<unsigned long>(duration_to_milliseconds(abs_time - cur_time));
      }
   }
   return do_winapi_wait(handle, time_ms);
}

inline void winapi_wrapper_wait_for_single_object(void *handle)
{
   (void)do_winapi_wait(handle, winapi::infinite_time);
}

inline bool winapi_wrapper_try_wait_for_single_object(void *handle)
{
   return do_winapi_wait(handle, 0u);
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_WINAPI_WRAPPER_COMMON_HPP
