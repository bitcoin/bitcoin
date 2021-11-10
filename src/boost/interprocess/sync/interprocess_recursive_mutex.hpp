//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code:
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#include <boost/assert.hpp>

#if   !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && \
       defined(BOOST_INTERPROCESS_POSIX_PROCESS_SHARED) && \
       defined (BOOST_INTERPROCESS_POSIX_RECURSIVE_MUTEXES)
   //Experimental...
   #include <boost/interprocess/sync/posix/recursive_mutex.hpp>
   #define BOOST_INTERPROCESS_RECURSIVE_MUTEX_USE_POSIX
#elif !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_WINDOWS)
   #include <boost/interprocess/sync/windows/recursive_mutex.hpp>
   #define BOOST_INTERPROCESS_RECURSIVE_MUTEX_USE_WINAPI
#else
   //spin_recursive_mutex is used
   #include <boost/interprocess/sync/spin/recursive_mutex.hpp>
   namespace boost {
   namespace interprocess {
   namespace ipcdetail{
   namespace robust_emulation_helpers {

   template<class T>
   class mutex_traits;

   }}}}
#endif

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!\file
//!Describes interprocess_recursive_mutex and shared_recursive_try_mutex classes

namespace boost {
namespace interprocess {

//!Wraps a interprocess_mutex that can be placed in shared memory and can be
//!shared between processes. Allows several locking calls by the same
//!process. Allows timed lock tries
class interprocess_recursive_mutex
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   interprocess_recursive_mutex(const interprocess_recursive_mutex &);
   interprocess_recursive_mutex &operator=(const interprocess_recursive_mutex &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   //!Constructor.
   //!Throws interprocess_exception on error.
   interprocess_recursive_mutex();

   //!Destructor. If any process uses the mutex after the destructor is called
   //!the result is undefined. Does not throw.
  ~interprocess_recursive_mutex();

   //!Effects: The calling thread tries to obtain ownership of the mutex, and
   //!   if another thread has ownership of the mutex, it waits until it can
   //!   obtain the ownership. If a thread takes ownership of the mutex the
   //!   mutex must be unlocked by the same mutex. The mutex must be unlocked
   //!   the same number of times it is locked.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program shall not deadlock if the thread that has ownership calls 
   //!   this function. 
   void lock();

   //!Tries to lock the interprocess_mutex, returns false when interprocess_mutex
   //!is already locked, returns true when success. The mutex must be unlocked
   //!the same number of times it is locked.
   //!Throws: interprocess_exception if a severe error is found
   //! 
   //!Note: A program shall not deadlock if the thread that has ownership calls 
   //!   this function. 
   bool try_lock();

   //!Tries to lock the interprocess_mutex, if interprocess_mutex can't be locked before
   //!abs_time time, returns false. The mutex must be unlocked
   //!   the same number of times it is locked.
   //!Throws: interprocess_exception if a severe error is found
   //! 
   //!Note: A program shall not deadlock if the thread that has ownership calls 
   //!   this function.
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);

   //!Effects: The calling thread releases the exclusive ownership of the mutex.
   //!   If the mutex supports recursive locking, the mutex must be unlocked the
   //!   same number of times it is locked.
   //!Throws: interprocess_exception on error.
   void unlock();
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   #if defined(BOOST_INTERPROCESS_RECURSIVE_MUTEX_USE_POSIX)
      ipcdetail::posix_recursive_mutex mutex;
   #elif defined(BOOST_INTERPROCESS_RECURSIVE_MUTEX_USE_WINAPI)
      ipcdetail::winapi_recursive_mutex mutex;
   #else
      void take_ownership(){ mutex.take_ownership(); }
      friend class ipcdetail::robust_emulation_helpers::mutex_traits<interprocess_recursive_mutex>;
      ipcdetail::spin_recursive_mutex mutex;
   #endif
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

}  //namespace interprocess {
}  //namespace boost {

namespace boost {
namespace interprocess {

inline interprocess_recursive_mutex::interprocess_recursive_mutex(){}

inline interprocess_recursive_mutex::~interprocess_recursive_mutex(){}

inline void interprocess_recursive_mutex::lock()
{  ipcdetail::timeout_when_locking_aware_lock(mutex);  }

inline bool interprocess_recursive_mutex::try_lock()
{ return mutex.try_lock(); }

template<class TimePoint>
inline bool interprocess_recursive_mutex::timed_lock(const TimePoint &abs_time)
{ return mutex.timed_lock(abs_time); }

inline void interprocess_recursive_mutex::unlock()
{ mutex.unlock(); }

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_RECURSIVE_MUTEX_HPP
