//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHM_NAMED_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_SHM_NAMED_RECURSIVE_MUTEX_HPP

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
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/shm/named_creation_functor.hpp>
#include <boost/interprocess/permissions.hpp>

//!\file
//!Describes a named shm_named_recursive_mutex class for inter-process synchronization

namespace boost {
namespace interprocess {
namespace ipcdetail {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
class interprocess_tester;
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

class shm_named_recursive_mutex
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   shm_named_recursive_mutex();
   shm_named_recursive_mutex(const shm_named_recursive_mutex &);
   shm_named_recursive_mutex &operator=(const shm_named_recursive_mutex &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:

   //!Creates a global recursive_mutex with a name.
   //!If the recursive_mutex can't be created throws interprocess_exception
   shm_named_recursive_mutex(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global recursive_mutex with a name.
   //!If the recursive_mutex is created, this call is equivalent to
   //!shm_named_recursive_mutex(create_only_t, ... )
   //!If the recursive_mutex is already created, this call is equivalent
   //!shm_named_recursive_mutex(open_only_t, ... )
   //!Does not throw
   shm_named_recursive_mutex(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global recursive_mutex with a name if that recursive_mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   shm_named_recursive_mutex(open_only_t open_only, const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Creates a global recursive_mutex with a name.
   //!If the recursive_mutex can't be created throws interprocess_exception
   shm_named_recursive_mutex(create_only_t create_only, const wchar_t *name, const permissions &perm = permissions());

   //!Opens or creates a global recursive_mutex with a name.
   //!If the recursive_mutex is created, this call is equivalent to
   //!shm_named_recursive_mutex(create_only_t, ... )
   //!If the recursive_mutex is already created, this call is equivalent
   //!shm_named_recursive_mutex(open_only_t, ... )
   //!Does not throw
   shm_named_recursive_mutex(open_or_create_t open_or_create, const wchar_t *name, const permissions &perm = permissions());

   //!Opens a global recursive_mutex with a name if that recursive_mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   shm_named_recursive_mutex(open_only_t open_only, const wchar_t *name);

   #endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~shm_named_recursive_mutex();

   //!Unlocks a previously locked
   //!shm_named_recursive_mutex.
   void unlock();

   //!Locks shm_named_recursive_mutex, sleeps when shm_named_recursive_mutex is already locked.
   //!Throws interprocess_exception if a severe error is found.
   void lock();

   //!Tries to lock the shm_named_recursive_mutex, returns false when shm_named_recursive_mutex
   //!is already locked, returns true when success.
   //!Throws interprocess_exception if a severe error is found.
   bool try_lock();

   //!Tries to lock the shm_named_recursive_mutex until time abs_time,
   //!Returns false when timeout expires, returns true when locks.
   //!Throws interprocess_exception if a severe error is found
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);

   //!Erases a named recursive mutex
   //!from the system
   static bool remove(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Erases a named recursive mutex
   //!from the system
   static bool remove(const wchar_t *name);

   #endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   friend class interprocess_tester;
   void dont_close_on_destruction();

   interprocess_recursive_mutex *mutex() const
   {  return static_cast<interprocess_recursive_mutex*>(m_shmem.get_user_address()); }
   typedef ipcdetail::managed_open_or_create_impl<shared_memory_object, 0, true, false> open_create_impl_t;
   open_create_impl_t m_shmem;
   typedef named_creation_functor<interprocess_recursive_mutex> construct_func_t;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

inline shm_named_recursive_mutex::~shm_named_recursive_mutex()
{}

inline void shm_named_recursive_mutex::dont_close_on_destruction()
{  interprocess_tester::dont_close_on_destruction(m_shmem);  }

inline shm_named_recursive_mutex::shm_named_recursive_mutex(create_only_t, const char *name, const permissions &perm)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoCreate)
               ,perm)
{}

inline shm_named_recursive_mutex::shm_named_recursive_mutex(open_or_create_t, const char *name, const permissions &perm)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoOpenOrCreate)
               ,perm)
{}

inline shm_named_recursive_mutex::shm_named_recursive_mutex(open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(DoOpen))
{}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline shm_named_recursive_mutex::shm_named_recursive_mutex(create_only_t, const wchar_t *name, const permissions &perm)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoCreate)
               ,perm)
{}

inline shm_named_recursive_mutex::shm_named_recursive_mutex(open_or_create_t, const wchar_t *name, const permissions &perm)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_recursive_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoOpenOrCreate)
               ,perm)
{}

inline shm_named_recursive_mutex::shm_named_recursive_mutex(open_only_t, const wchar_t *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(DoOpen))
{}

#endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline void shm_named_recursive_mutex::lock()
{  this->mutex()->lock();  }

inline void shm_named_recursive_mutex::unlock()
{  this->mutex()->unlock();  }

inline bool shm_named_recursive_mutex::try_lock()
{  return this->mutex()->try_lock();  }

template<class TimePoint>
inline bool shm_named_recursive_mutex::timed_lock(const TimePoint &abs_time)
{  return this->mutex()->timed_lock(abs_time);  }

inline bool shm_named_recursive_mutex::remove(const char *name)
{  return shared_memory_object::remove(name); }

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline bool shm_named_recursive_mutex::remove(const wchar_t *name)
{  return shared_memory_object::remove(name); }

#endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SHM_NAMED_RECURSIVE_MUTEX_HPP
