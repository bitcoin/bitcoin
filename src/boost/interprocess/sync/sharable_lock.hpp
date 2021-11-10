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
// This interface is inspired by Howard Hinnant's lock proposal.
// http://home.twcny.rr.com/hinnant/cpp_extensions/threads_move.html
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARABLE_LOCK_HPP
#define BOOST_INTERPROCESS_SHARABLE_LOCK_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/sync/lock_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/simple_swap.hpp>
#include <boost/move/utility_core.hpp>

//!\file
//!Describes the upgradable_lock class that serves to acquire the upgradable
//!lock of a mutex.

namespace boost {
namespace interprocess {


//!sharable_lock is meant to carry out the tasks for sharable-locking
//!(such as read-locking), unlocking, try-sharable-locking and timed-sharable-locking
//!(recursive or not) for the Mutex. The Mutex need not supply all of this
//!functionality. If the client of sharable_lock<Mutex> does not use functionality which
//!the Mutex does not supply, no harm is done. Mutex ownership can be shared among
//!sharable_locks, and a single upgradable_lock. sharable_lock does not support
//!copy semantics. But sharable_lock supports ownership transfer from an sharable_lock,
//!upgradable_lock and scoped_lock via transfer_lock syntax.*/
template <class SharableMutex>
class sharable_lock
{
   public:
   typedef SharableMutex mutex_type;
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef sharable_lock<SharableMutex> this_type;
   explicit sharable_lock(scoped_lock<mutex_type>&);
   typedef bool this_type::*unspecified_bool_type;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(sharable_lock)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:

   //!Effects: Default constructs a sharable_lock.
   //!Postconditions: owns() == false and mutex() == 0.
   sharable_lock() BOOST_NOEXCEPT
      : mp_mutex(0), m_locked(false)
   {}

   //!Effects: m.lock_sharable().
   //!Postconditions: owns() == true and mutex() == &m.
   //!Notes: The constructor will take sharable-ownership of the mutex. If
   //!   another thread already owns the mutex with exclusive ownership
   //!   (scoped_lock), this thread will block until the mutex is released.
   //!   If another thread owns the mutex with sharable or upgradable ownership,
   //!   then no blocking will occur. Whether or not this constructor handles
   //!   recursive locking depends upon the mutex.
   explicit sharable_lock(mutex_type& m)
      : mp_mutex(&m), m_locked(false)
   {  mp_mutex->lock_sharable();   m_locked = true;  }

   //!Postconditions: owns() == false, and mutex() == &m.
   //!Notes: The constructor will not take ownership of the mutex. There is no effect
   //!   required on the referenced mutex.
   sharable_lock(mutex_type& m, defer_lock_type)
      : mp_mutex(&m), m_locked(false)
   {}

   //!Postconditions: owns() == true, and mutex() == &m.
   //!Notes: The constructor will suppose that the mutex is already sharable
   //!   locked. There is no effect required on the referenced mutex.
   sharable_lock(mutex_type& m, accept_ownership_type)
      : mp_mutex(&m), m_locked(true)
   {}

   //!Effects: m.try_lock_sharable()
   //!Postconditions: mutex() == &m. owns() == the return value of the
   //!   m.try_lock_sharable() executed within the constructor.
   //!Notes: The constructor will take sharable-ownership of the mutex if it
   //!   can do so without waiting. Whether or not this constructor handles
   //!   recursive locking depends upon the mutex. If the mutex_type does not
   //!   support try_lock_sharable, this constructor will fail at compile
   //!   time if instantiated, but otherwise have no effect.
   sharable_lock(mutex_type& m, try_to_lock_type)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->try_lock_sharable();   }

   //!Effects: m.timed_lock_sharable(abs_time)
   //!Postconditions: mutex() == &m. owns() == the return value of the
   //!   m.timed_lock_sharable() executed within the constructor.
   //!Notes: The constructor will take sharable-ownership of the mutex if it
   //!   can do so within the time specified. Whether or not this constructor
   //!   handles recursive locking depends upon the mutex. If the mutex_type
   //!   does not support timed_lock_sharable, this constructor will fail at
   //!   compile time if instantiated, but otherwise have no effect.
   template<class TimePoint>
   sharable_lock(mutex_type& m, const TimePoint& abs_time)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->timed_lock_sharable(abs_time);  }

   //!Postconditions: mutex() == upgr.mutex(). owns() == the value of upgr.owns()
   //!   before the construction. upgr.owns() == false after the construction.
   //!Notes: If the upgr sharable_lock owns the mutex, ownership is moved to this
   //!   sharable_lock with no blocking. If the upgr sharable_lock does not own the mutex, then
   //!   neither will this sharable_lock. Only a moved sharable_lock's will match this
   //!   signature. An non-moved sharable_lock can be moved with the expression:
   //!   "boost::move(lock);". This constructor does not alter the state of the mutex,
   //!   only potentially who owns it.
   sharable_lock(BOOST_RV_REF(sharable_lock<mutex_type>) upgr) BOOST_NOEXCEPT
      : mp_mutex(0), m_locked(upgr.owns())
   {  mp_mutex = upgr.release(); }

   //!Effects: If upgr.owns() then calls unlock_upgradable_and_lock_sharable() on the
   //!   referenced mutex.
   //!Postconditions: mutex() == the value upgr.mutex() had before the construction.
   //!   upgr.mutex() == 0 owns() == the value of upgr.owns() before construction.
   //!   upgr.owns() == false after the construction.
   //!Notes: If upgr is locked, this constructor will lock this sharable_lock while
   //!   unlocking upgr. Only a moved sharable_lock's will match this
   //!   signature. An non-moved upgradable_lock can be moved with the expression:
   //!   "boost::move(lock);".*/
   template<class T>
   sharable_lock(BOOST_RV_REF(upgradable_lock<T>) upgr
      , typename ipcdetail::enable_if< ipcdetail::is_same<T, SharableMutex> >::type * = 0)
      : mp_mutex(0), m_locked(false)
   {
      upgradable_lock<mutex_type> &u_lock = upgr;
      if(u_lock.owns()){
         u_lock.mutex()->unlock_upgradable_and_lock_sharable();
         m_locked = true;
      }
      mp_mutex = u_lock.release();
   }

   //!Effects: If scop.owns() then calls unlock_and_lock_sharable() on the
   //!   referenced mutex.
   //!Postconditions: mutex() == the value scop.mutex() had before the construction.
   //!   scop.mutex() == 0 owns() == scop.owns() before the constructor. After the
   //!   construction, scop.owns() == false.
   //!Notes: If scop is locked, this constructor will transfer the exclusive ownership
   //!   to a sharable-ownership of this sharable_lock.
   //!   Only a moved scoped_lock's will match this
   //!   signature. An non-moved scoped_lock can be moved with the expression:
   //!   "boost::move(lock);".
   template<class T>
   sharable_lock(BOOST_RV_REF(scoped_lock<T>) scop
               , typename ipcdetail::enable_if< ipcdetail::is_same<T, SharableMutex> >::type * = 0)
      : mp_mutex(0), m_locked(false)
   {
      scoped_lock<mutex_type> &e_lock = scop;
      if(e_lock.owns()){
         e_lock.mutex()->unlock_and_lock_sharable();
         m_locked = true;
      }
      mp_mutex = e_lock.release();
   }

   //!Effects: if (owns()) mp_mutex->unlock_sharable().
   //!Notes: The destructor behavior ensures that the mutex lock is not leaked.
   ~sharable_lock()
   {
      try{
         if(m_locked && mp_mutex)   mp_mutex->unlock_sharable();
      }
      catch(...){}
   }

   //!Effects: If owns() before the call, then unlock_sharable() is called on mutex().
   //!   *this gets the state of upgr and upgr gets set to a default constructed state.
   //!Notes: With a recursive mutex it is possible that both this and upgr own the mutex
   //!   before the assignment. In this case, this will own the mutex after the assignment
   //!   (and upgr will not), but the mutex's lock count will be decremented by one.
   sharable_lock &operator=(BOOST_RV_REF(sharable_lock<mutex_type>) upgr)
   {
      if(this->owns())
         this->unlock();
      m_locked = upgr.owns();
      mp_mutex = upgr.release();
      return *this;
   }

   //!Effects: If mutex() == 0 or already locked, throws a lock_exception()
   //!   exception. Calls lock_sharable() on the referenced mutex.
   //!Postconditions: owns() == true.
   //!Notes: The sharable_lock changes from a state of not owning the
   //!   mutex, to owning the mutex, blocking if necessary.
   void lock()
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      mp_mutex->lock_sharable();
      m_locked = true;
   }

   //!Effects: If mutex() == 0 or already locked, throws a lock_exception()
   //!   exception. Calls try_lock_sharable() on the referenced mutex.
   //!Postconditions: owns() == the value returned from
   //!   mutex()->try_lock_sharable().
   //!Notes: The sharable_lock changes from a state of not owning the mutex,
   //!   to owning the mutex, but only if blocking was not required. If the
   //!   mutex_type does not support try_lock_sharable(), this function will
   //!   fail at compile time if instantiated, but otherwise have no effect.
   bool try_lock()
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->try_lock_sharable();
      return m_locked;
   }

   //!Effects: If mutex() == 0 or already locked, throws a lock_exception()
   //!   exception. Calls timed_lock_sharable(abs_time) on the referenced mutex.
   //!Postconditions: owns() == the value returned from
   //!   mutex()->timed_lock_sharable(elps_time).
   //!Notes: The sharable_lock changes from a state of not owning the mutex,
   //!   to owning the mutex, but only if it can obtain ownership within the
   //!   specified time interval. If the mutex_type does not support
   //!   timed_lock_sharable(), this function will fail at compile time if
   //!   instantiated, but otherwise have no effect.
   template<class TimePoint>
   bool timed_lock(const TimePoint& abs_time)
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->timed_lock_sharable(abs_time);
      return m_locked;
   }

   //!Effects: If mutex() == 0 or not locked, throws a lock_exception() exception.
   //!   Calls unlock_sharable() on the referenced mutex.
   //!Postconditions: owns() == false.
   //!Notes: The sharable_lock changes from a state of owning the mutex, to
   //!   not owning the mutex.
   void unlock()
   {
      if(!mp_mutex || !m_locked)
         throw lock_exception();
      mp_mutex->unlock_sharable();
      m_locked = false;
   }

   //!Effects: Returns true if this scoped_lock has
   //!acquired the referenced mutex.
   bool owns() const BOOST_NOEXCEPT
   {  return m_locked && mp_mutex;  }

   //!Conversion to bool.
   //!Returns owns().
   operator unspecified_bool_type() const BOOST_NOEXCEPT
   {  return m_locked? &this_type::m_locked : 0;   }

   //!Effects: Returns a pointer to the referenced mutex, or 0 if
   //!there is no mutex to reference.
   mutex_type* mutex() const BOOST_NOEXCEPT
   {  return  mp_mutex;  }

   //!Effects: Returns a pointer to the referenced mutex, or 0 if there is no
   //!   mutex to reference.
   //!Postconditions: mutex() == 0 and owns() == false.
   mutex_type* release() BOOST_NOEXCEPT
   {
      mutex_type *mut = mp_mutex;
      mp_mutex = 0;
      m_locked = false;
      return mut;
   }

   //!Effects: Swaps state with moved lock.
   //!Throws: Nothing.
   void swap(sharable_lock<mutex_type> &other) BOOST_NOEXCEPT
   {
      (simple_swap)(mp_mutex, other.mp_mutex);
      (simple_swap)(m_locked, other.m_locked);
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   mutex_type *mp_mutex;
   bool        m_locked;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_SHARABLE_LOCK_HPP
