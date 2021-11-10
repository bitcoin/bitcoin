//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_SEMAPHORE_HPP

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

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/detail/locks.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>

#if   !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && \
       defined(BOOST_INTERPROCESS_POSIX_PROCESS_SHARED)    && \
       defined(BOOST_INTERPROCESS_POSIX_UNNAMED_SEMAPHORES)
   #include <boost/interprocess/sync/posix/semaphore.hpp>
   #define BOOST_INTERPROCESS_SEMAPHORE_USE_POSIX
#elif !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_WINDOWS)
   //Experimental...
   #include <boost/interprocess/sync/windows/semaphore.hpp>
   #define BOOST_INTERPROCESS_SEMAPHORE_USE_WINAPI
#else
   //spin_semaphore is used
   #include <boost/interprocess/sync/spin/semaphore.hpp>
#endif

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!\file
//!Describes a interprocess_semaphore class for inter-process synchronization

namespace boost {
namespace interprocess {

//!Wraps a interprocess_semaphore that can be placed in shared memory and can be
//!shared between processes. Allows timed lock tries
class interprocess_semaphore
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   interprocess_semaphore(const interprocess_semaphore &);
   interprocess_semaphore &operator=(const interprocess_semaphore &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   //!Creates a interprocess_semaphore with the given initial count.
   //!interprocess_exception if there is an error.*/
   interprocess_semaphore(unsigned int initialCount);

   //!Destroys the interprocess_semaphore.
   //!Does not throw
   ~interprocess_semaphore();

   //!Increments the interprocess_semaphore count. If there are processes/threads blocked waiting
   //!for the interprocess_semaphore, then one of these processes will return successfully from
   //!its wait function. If there is an error an interprocess_exception exception is thrown.
   void post();

   //!Decrements the interprocess_semaphore. If the interprocess_semaphore value is not greater than zero,
   //!then the calling process/thread blocks until it can decrement the counter.
   //!If there is an error an interprocess_exception exception is thrown.
   void wait();

   //!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater than zero
   //!and returns true. If the value is not greater than zero returns false.
   //!If there is an error an interprocess_exception exception is thrown.
   bool try_wait();

   //!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater
   //!than zero and returns true. Otherwise, waits for the interprocess_semaphore
   //!to the posted or the timeout expires. If the timeout expires, the
   //!function returns false. If the interprocess_semaphore is posted the function
   //!returns true. If there is an error throws sem_exception
   template<class TimePoint>
   bool timed_wait(const TimePoint &abs_time);

   //!Returns the interprocess_semaphore count
//   int get_count() const;
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   #if defined(BOOST_INTERPROCESS_SEMAPHORE_USE_POSIX)
      typedef ipcdetail::posix_semaphore internal_sem_t;
   #elif defined(BOOST_INTERPROCESS_SEMAPHORE_USE_WINAPI)
      typedef ipcdetail::winapi_semaphore internal_sem_t;
   #else
      typedef ipcdetail::spin_semaphore internal_sem_t;
   #endif   //#if defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION)
   internal_sem_t m_sem;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

}  //namespace interprocess {
}  //namespace boost {

namespace boost {
namespace interprocess {

inline interprocess_semaphore::interprocess_semaphore(unsigned int initialCount)
   : m_sem(initialCount)
{}

inline interprocess_semaphore::~interprocess_semaphore(){}

inline void interprocess_semaphore::wait()
{
   ipcdetail::lock_to_wait<internal_sem_t> ltw(m_sem);
   timeout_when_locking_aware_lock(ltw);
}

inline bool interprocess_semaphore::try_wait()
{ return m_sem.try_wait(); }

template<class TimePoint>
inline bool interprocess_semaphore::timed_wait(const TimePoint &abs_time)
{ return m_sem.timed_wait(abs_time); }

inline void interprocess_semaphore::post()
{ m_sem.post(); }

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SEMAPHORE_HPP
