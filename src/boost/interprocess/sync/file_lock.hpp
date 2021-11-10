//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FILE_LOCK_HPP
#define BOOST_INTERPROCESS_FILE_LOCK_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#include <boost/interprocess/sync/detail/locks.hpp>
#include <boost/move/utility_core.hpp>

//!\file
//!Describes a class that wraps file locking capabilities.

namespace boost {
namespace interprocess {


//!A file lock, is a mutual exclusion utility similar to a mutex using a
//!file. A file lock has sharable and exclusive locking capabilities and
//!can be used with scoped_lock and sharable_lock classes.
//!A file lock can't guarantee synchronization between threads of the same
//!process so just use file locks to synchronize threads from different processes.
class file_lock
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(file_lock)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Constructs an empty file mapping.
   //!Does not throw
   file_lock() BOOST_NOEXCEPT
      :  m_file_hnd(file_handle_t(ipcdetail::invalid_file()))
   {}

   //!Opens a file lock. Throws interprocess_exception if the file does not
   //!exist or there are no operating system resources.
   file_lock(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!Opens a file lock. Throws interprocess_exception if the file does not
   //!exist or there are no operating system resources.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   file_lock(const wchar_t *name);
   #endif

   //!Moves the ownership of "moved"'s file mapping object to *this.
   //!After the call, "moved" does not represent any file mapping object.
   //!Does not throw
   file_lock(BOOST_RV_REF(file_lock) moved) BOOST_NOEXCEPT
      :  m_file_hnd(file_handle_t(ipcdetail::invalid_file()))
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s file mapping to *this.
   //!After the call, "moved" does not represent any file mapping.
   //!Does not throw
   file_lock &operator=(BOOST_RV_REF(file_lock) moved) BOOST_NOEXCEPT
   {
      file_lock tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Closes a file lock. Does not throw.
   ~file_lock();

   //!Swaps two file_locks.
   //!Does not throw.
   void swap(file_lock &other) BOOST_NOEXCEPT
   {
      file_handle_t tmp = m_file_hnd;
      m_file_hnd = other.m_file_hnd;
      other.m_file_hnd = tmp;
   }

   //Exclusive locking

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to obtain exclusive ownership of the mutex,
   //!   and if another thread has exclusive, or sharable ownership of
   //!   the mutex, it waits until it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   void lock();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire exclusive ownership of the mutex
   //!   without waiting. If no other thread has exclusive, or sharable
   //!   ownership of the mutex this succeeds.
   //!Returns: If it can acquire exclusive ownership immediately returns true.
   //!   If it has to wait, returns false.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   bool try_lock();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire exclusive ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive, or sharable
   //!   ownership of the mutex or abs_time is reached.
   //!Returns: If acquires exclusive ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);

   //!Precondition: The thread must have exclusive ownership of the mutex.
   //!Effects: The calling thread releases the exclusive ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock();

   //Sharable locking

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to obtain sharable ownership of the mutex,
   //!   and if another thread has exclusive ownership of the mutex, waits until
   //!   it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   //!
   //!Note: A program may deadlock if the thread that owns a mutex object calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   void lock_sharable();

   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   without waiting. If no other thread has exclusive ownership of the
   //!   mutex this succeeds.
   //!Returns: If it can acquire sharable ownership immediately returns true. If it
   //!   has to wait, returns false.
   //!Throws: interprocess_exception on error.
   bool try_lock_sharable();

   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive ownership of
   //!   the mutex or abs_time is reached.
   //!Returns: If acquires sharable ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   template<class TimePoint>
   bool timed_lock_sharable(const TimePoint &abs_time);

   //!Precondition: The thread must have sharable ownership of the mutex.
   //!Effects: The calling thread releases the sharable ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_sharable();
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   file_handle_t m_file_hnd;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

inline file_lock::file_lock(const char *name)
{
   m_file_hnd = ipcdetail::open_existing_file(name, read_write);

   if(m_file_hnd == ipcdetail::invalid_file()){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline file_lock::file_lock(const wchar_t *name)
{
   m_file_hnd = ipcdetail::open_existing_file(name, read_write);

   if(m_file_hnd == ipcdetail::invalid_file()){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

#endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline file_lock::~file_lock()
{
   if(m_file_hnd != ipcdetail::invalid_file()){
      ipcdetail::close_file(m_file_hnd);
      m_file_hnd = ipcdetail::invalid_file();
   }
}

inline void file_lock::lock()
{
   if(!ipcdetail::acquire_file_lock(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline bool file_lock::try_lock()
{
   bool result;
   if(!ipcdetail::try_acquire_file_lock(m_file_hnd, result)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

template<class TimePoint>
inline bool file_lock::timed_lock(const TimePoint &abs_time)
{  return ipcdetail::try_based_timed_lock(*this, abs_time);   }

inline void file_lock::unlock()
{
   if(!ipcdetail::release_file_lock(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void file_lock::lock_sharable()
{
   if(!ipcdetail::acquire_file_lock_sharable(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline bool file_lock::try_lock_sharable()
{
   bool result;
   if(!ipcdetail::try_acquire_file_lock_sharable(m_file_hnd, result)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

template<class TimePoint>
inline bool file_lock::timed_lock_sharable(const TimePoint &abs_time)
{
   ipcdetail::lock_to_sharable<file_lock> lsh(*this);
   return ipcdetail::try_based_timed_lock(lsh, abs_time);
}

inline void file_lock::unlock_sharable()
{
   if(!ipcdetail::release_file_lock_sharable(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_FILE_LOCK_HPP
