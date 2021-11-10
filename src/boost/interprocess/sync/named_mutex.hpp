//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_MUTEX_HPP

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
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/permissions.hpp>

#if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
   #include <boost/interprocess/sync/posix/named_mutex.hpp>
   #define BOOST_INTERPROCESS_NAMED_MUTEX_USE_POSIX
#elif !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_WINDOWS)
   #include <boost/interprocess/sync/windows/named_mutex.hpp>
   #define BOOST_INTERPROCESS_NAMED_MUTEX_USE_WINAPI
#else
   #include <boost/interprocess/sync/shm/named_mutex.hpp>
#endif

//!\file
//!Describes a named mutex class for inter-process synchronization

namespace boost {
namespace interprocess {

class named_condition;

//!A mutex with a global name, so it can be found from different
//!processes. This mutex can't be placed in shared memory, and
//!each process should have it's own named_mutex.
class named_mutex
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Non-copyable
   named_mutex();
   named_mutex(const named_mutex &);
   named_mutex &operator=(const named_mutex &);
   friend class named_condition;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Creates a global mutex with a name.
   //!Throws interprocess_exception on error.
   named_mutex(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global mutex with a name.
   //!If the mutex is created, this call is equivalent to
   //!named_mutex(create_only_t, ... )
   //!If the mutex is already created, this call is equivalent
   //!named_mutex(open_only_t, ... )
   //!Does not throw
   named_mutex(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global mutex with a name if that mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_mutex(open_only_t open_only, const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Creates a global mutex with a name.
   //!Throws interprocess_exception on error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_mutex(create_only_t create_only, const wchar_t *name, const permissions &perm = permissions());

   //!Opens or creates a global mutex with a name.
   //!If the mutex is created, this call is equivalent to
   //!named_mutex(create_only_t, ... )
   //!If the mutex is already created, this call is equivalent
   //!named_mutex(open_only_t, ... )
   //!Does not throw
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_mutex(open_or_create_t open_or_create, const wchar_t *name, const permissions &perm = permissions());

   //!Opens a global mutex with a name if that mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_mutex(open_only_t open_only, const wchar_t *name);

   #endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_mutex();

   //!Unlocks a previously locked
   //!mutex.
   void unlock();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Locks the mutex, sleeps when the mutex is already locked.
   //!Throws interprocess_exception if a severe error is found
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   void lock();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Tries to lock the mutex, returns false when the mutex
   //!is already locked, returns true when success.
   //!Throws interprocess_exception if a severe error is found
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   bool try_lock();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Tries to lock the the mutex until time abs_time,
   //!Returns false when timeout expires, returns true when locks.
   //!Throws interprocess_exception if a severe error is found
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);

   //!Erases a named mutex from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Erases a named mutex from the system.
   //!Returns false on error. Never throws.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool remove(const wchar_t *name);

   #endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   friend class ipcdetail::interprocess_tester;
   void dont_close_on_destruction();

   public:
   #if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USE_POSIX)
      typedef ipcdetail::posix_named_mutex      internal_mutex_type;
   #elif defined(BOOST_INTERPROCESS_NAMED_MUTEX_USE_WINAPI)
      typedef ipcdetail::winapi_named_mutex    internal_mutex_type;
   #else
      typedef ipcdetail::shm_named_mutex        internal_mutex_type;
   #endif
   internal_mutex_type &internal_mutex()
   {  return m_mut; }

   internal_mutex_type m_mut;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline named_mutex::named_mutex(create_only_t, const char *name, const permissions &perm)
   :  m_mut(create_only_t(), name, perm)
{}

inline named_mutex::named_mutex(open_or_create_t, const char *name, const permissions &perm)
   :  m_mut(open_or_create_t(), name, perm)
{}

inline named_mutex::named_mutex(open_only_t, const char *name)
   :  m_mut(open_only_t(), name)
{}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline named_mutex::named_mutex(create_only_t, const wchar_t *name, const permissions &perm)
   :  m_mut(create_only_t(), name, perm)
{}

inline named_mutex::named_mutex(open_or_create_t, const wchar_t *name, const permissions &perm)
   :  m_mut(open_or_create_t(), name, perm)
{}

inline named_mutex::named_mutex(open_only_t, const wchar_t *name)
   :  m_mut(open_only_t(), name)
{}

#endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline void named_mutex::dont_close_on_destruction()
{  ipcdetail::interprocess_tester::dont_close_on_destruction(m_mut); }

inline named_mutex::~named_mutex()
{}

inline void named_mutex::lock()
{  m_mut.lock();  }

inline void named_mutex::unlock()
{  m_mut.unlock();  }

inline bool named_mutex::try_lock()
{  return m_mut.try_lock();  }

template<class TimePoint>
inline bool named_mutex::timed_lock(const TimePoint &abs_time)
{  return m_mut.timed_lock(abs_time);  }

inline bool named_mutex::remove(const char *name)
{  return internal_mutex_type::remove(name);   }

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline bool named_mutex::remove(const wchar_t *name)
{  return internal_mutex_type::remove(name);   }

#endif

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_MUTEX_HPP
