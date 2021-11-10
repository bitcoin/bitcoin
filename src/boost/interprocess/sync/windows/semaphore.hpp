//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_WINDOWS_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_DETAIL_WINDOWS_SEMAPHORE_HPP

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
#include <boost/interprocess/detail/windows_intermodule_singleton.hpp>
#include <boost/interprocess/sync/windows/sync_utils.hpp>
#include <boost/interprocess/sync/windows/winapi_semaphore_wrapper.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/assert.hpp>


namespace boost {
namespace interprocess {
namespace ipcdetail {

class winapi_semaphore
{
   winapi_semaphore(const winapi_semaphore &);
   winapi_semaphore &operator=(const winapi_semaphore &);
   public:

   winapi_semaphore(unsigned int initialCount);
   ~winapi_semaphore();

   void post(unsigned int release_count = 1);
   void wait();
   bool try_wait();
   template<class TimePoint> bool timed_wait(const TimePoint &abs_time);

   private:
   const sync_id id_;
   const unsigned initial_count_;
};

inline winapi_semaphore::winapi_semaphore(unsigned int initialCount)
   : id_(this), initial_count_(initialCount)
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   //Force smeaphore creation with the initial count
   bool open_or_created;
   handles.obtain_semaphore(this->id_, initialCount, &open_or_created);
   //The semaphore must be created, never opened
   BOOST_ASSERT(open_or_created);
   BOOST_ASSERT(open_or_created && winapi::get_last_error() != winapi::error_already_exists);
   (void)open_or_created;
}

inline winapi_semaphore::~winapi_semaphore()
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   handles.destroy_handle(this->id_);
}

inline void winapi_semaphore::wait()
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   //This can throw
   winapi_semaphore_functions sem(handles.obtain_semaphore(this->id_, initial_count_));
   sem.wait();
}

inline bool winapi_semaphore::try_wait()
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   //This can throw
   winapi_semaphore_functions sem(handles.obtain_semaphore(this->id_, initial_count_));
   return sem.try_wait();
}

template<class TimePoint>
inline bool winapi_semaphore::timed_wait(const TimePoint &abs_time)
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   //This can throw
   winapi_semaphore_functions sem(handles.obtain_semaphore(this->id_, initial_count_));
   return sem.timed_wait(abs_time);
}

inline void winapi_semaphore::post(unsigned release_count)
{
   sync_handles &handles =
      windows_intermodule_singleton<sync_handles>::get();
   winapi_semaphore_functions sem(handles.obtain_semaphore(this->id_, initial_count_));
   sem.post(release_count);
}


}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_WINDOWS_SEMAPHORE_HPP
