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

#ifndef BOOST_INTERPROCESS_UPGRADABLE_LOCK_HPP
#define BOOST_INTERPROCESS_UPGRADABLE_LOCK_HPP

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
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>

#include <boost/interprocess/exceptions.hpp>
#include <boost/move/utility_core.hpp>

//!\file
//!Describes the upgradable_lock class that serves to acquire the upgradable
//!lock of a mutex.

namespace boost {
namespace interprocess {

//!upgradable_lock is meant to carry out the tasks for read-locking, unlocking,
//!try-read-locking and timed-read-locking (recursive or not) for the Mutex.
//!Additionally the upgradable_lock can transfer ownership to a scoped_lock
//!using transfer_lock syntax. The Mutex need not supply all of the functionality.
//!If the client of upgradable_lock<Mutex> does not use functionality which the
//!Mutex does not supply, no harm is done. Mutex ownership can be shared among
//!read_locks, and a single upgradable_lock. upgradable_lock does not support
//!copy semantics. However upgradable_lock supports ownership transfer from
//!a upgradable_locks or scoped_locks via transfer_lock syntax.
template <class UpgradableMutex>
class upgradable_lock
{
   public:
   typedef UpgradableMutex mutex_type;
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef upgradable_lock<UpgradableMutex> this_type;
   explicit upgradable_lock(scoped_lock<mutex_type>&);
   typedef bool this_type::*unspecified_bool_type;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(upgradable_lock)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:

   //!Effects: Default constructs a upgradable_lock.
   //!Postconditions: owns() == false and mutex() == 0.
   upgradable_lock() BOOST_NOEXCEPT
      : mp_mutex(0), m_locked(false)
   {}

   explicit upgradable_lock(mutex_type& m)
      : mp_mutex(&m), m_locked(false)
   {  mp_mutex->lock_upgradable();   m_locked = true;  }

   //!Postconditions: owns() == false, and mutex() == &m.
   //!Notes: The constructor will not take ownership of the mutex. There is no effect
   //!   required on the referenced mutex.
   upgradable_lock(mutex_type& m, defer_lock_type)
      : mp_mutex(&m), m_locked(false)
   {}

   //!Postconditions: owns() == true, and mutex() == &m.
   //!Notes: The constructor will suppose that the mutex is already upgradable
   //!   locked. There is no effect required on the referenced mutex.
   upgradable_lock(mutex_type& m, accept_ownership_type)
      : mp_mutex(&m), m_locked(true)
   {}

   //!Effects: m.try_lock_upgradable().
   //!Postconditions: mutex() == &m. owns() == the return value of the
   //!   m.try_lock_upgradable() executed within the constructor.
   //!Notes: The constructor will take upgradable-ownership of the mutex
   //!   if it can do so without waiting. Whether or not this constructor
   //!   handles recursive locking depends upon the mutex. If the mutex_type
   //!   does not support try_lock_upgradable, this constructor will fail at
   //!   compile time if instantiated, but otherwise have no effect.
   upgradable_lock(mutex_type& m, try_to_lock_type)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->try_lock_upgradable();   }

   //!Effects: m.timed_lock_upgradable(abs_time)
   //!Postconditions: mutex() == &m. owns() == the return value of the
   //!   m.timed_lock_upgradable() executed within the constructor.
   //!Notes: The constructor will take upgradable-ownership of the mutex if it
   //!   can do so within the time specified. Whether or not this constructor
   //!   handles recursive locking depends upon the mutex. If the mutex_type
   //!   does not support timed_lock_upgradable, this constructor will fail
   //!   at compile time if instantiated, but otherwise have no effect.
   template<class TimePoint>
   upgradable_lock(mutex_type& m, const TimePoint& abs_time)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->timed_lock_upgradable(abs_time);  }

   //!Effects: No effects on the underlying mutex.
   //!Postconditions: mutex() == the value upgr.mutex() had before the
   //!   construction. upgr.mutex() == 0. owns() == upgr.owns() before the
   //!   construction. upgr.owns() == false.
   //!Notes: If upgr is locked, this constructor will lock this upgradable_lock
   //!   while unlocking upgr. If upgr is unlocked, then this upgradable_lock will
   //!   be unlocked as well. Only a moved upgradable_lock's will match this
   //!   signature. An non-moved upgradable_lock can be moved with the
   //!   expression: "boost::move(lock);". This constructor does not alter the
   //!   state of the mutex, only potentially who owns it.
   upgradable_lock(BOOST_RV_REF(upgradable_lock<mutex_type>) upgr) BOOST_NOEXCEPT
      : mp_mutex(0), m_locked(upgr.owns())
   {  mp_mutex = upgr.release(); }

   //!Effects: If scop.owns(), m_.unlock_and_lock_upgradable().
   //!Postconditions: mutex() == the value scop.mutex() had before the construction.
   //!   scop.mutex() == 0. owns() == scop.owns() before the constructor. After the
   //!   construction, scop.owns() == false.
   //!Notes: If scop is locked, this constructor will transfer the exclusive-ownership
   //!   to an upgradable-ownership of this upgradable_lock.
   //!   Only a moved sharable_lock's will match this
   //!   signature. An non-moved sharable_lock can be moved with the
   //!   expression: "boost::move(lock);".
   template<class T>
   upgradable_lock(BOOST_RV_REF(scoped_lock<T>) scop
                  , typename ipcdetail::enable_if< ipcdetail::is_same<T, UpgradableMutex> >::type * = 0)
      : mp_mutex(0), m_locked(false)
   {
      scoped_lock<mutex_type> &u_lock = scop;
      if(u_lock.owns()){
         u_lock.mutex()->unlock_and_lock_upgradable();
         m_locked = true;
      }
      mp_mutex = u_lock.release();
   }

   //!Effects: If shar.owns() then calls try_unlock_sharable_and_lock_upgradable()
   //!   on the referenced mutex.
   //!   a)if try_unlock_sharable_and_lock_upgradable() returns true then mutex()
   //!      obtains the value from shar.release() and owns() is set to true.
   //!   b)if try_unlock_sharable_and_lock_upgradable() returns false then shar is
   //!      unaffected and this upgradable_lock construction has the same
   //!      effects as a default construction.
   //!   c)Else shar.owns() is false. mutex() obtains the value from shar.release()
   //!      and owns() is set to false.
   //!Notes: This construction will not block. It will try to obtain mutex
   //!   ownership from shar immediately, while changing the lock type from a
   //!   "read lock" to an "upgradable lock". If the "read lock" isn't held
   //!   in the first place, the mutex merely changes type to an unlocked
   //!   "upgradable lock". If the "read lock" is held, then mutex transfer
   //!   occurs only if it can do so in a non-blocking manner.
   template<class T>
   upgradable_lock( BOOST_RV_REF(sharable_lock<T>) shar, try_to_lock_type
                  , typename ipcdetail::enable_if< ipcdetail::is_same<T, UpgradableMutex> >::type * = 0)
      : mp_mutex(0), m_locked(false)
   {
      sharable_lock<mutex_type> &s_lock = shar;
      if(s_lock.owns()){
         if((m_locked = s_lock.mutex()->try_unlock_sharable_and_lock_upgradable()) == true){
            mp_mutex = s_lock.release();
         }
      }
      else{
         s_lock.release();
      }
   }

   //!Effects: if (owns()) m_->unlock_upgradable().
   //!Notes: The destructor behavior ensures that the mutex lock is not leaked.
   ~upgradable_lock()
   {
      try{
         if(m_locked && mp_mutex)   mp_mutex->unlock_upgradable();
      }
      catch(...){}
   }

   //!Effects: If owns(), then unlock_upgradable() is called on mutex().
   //!   *this gets the state of upgr and upgr gets set to a default constructed state.
   //!Notes: With a recursive mutex it is possible that both this and upgr own the
   //!   mutex before the assignment. In this case, this will own the mutex
   //!   after the assignment (and upgr will not), but the mutex's upgradable lock
   //!   count will be decremented by one.
   upgradable_lock &operator=(BOOST_RV_REF(upgradable_lock) upgr)
   {
      if(this->owns())
         this->unlock();
      m_locked = upgr.owns();
      mp_mutex = upgr.release();
      return *this;
   }

   //!Effects: If mutex() == 0 or if already locked, throws a lock_exception()
   //!   exception. Calls lock_upgradable() on the referenced mutex.
   //!Postconditions: owns() == true.
   //!Notes: The sharable_lock changes from a state of not owning the mutex,
   //!   to owning the mutex, blocking if necessary.
   void lock()
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      mp_mutex->lock_upgradable();
      m_locked = true;
   }

   //!Effects: If mutex() == 0 or if already locked, throws a lock_exception()
   //!   exception. Calls try_lock_upgradable() on the referenced mutex.
   //!Postconditions: owns() == the value returned from
   //!   mutex()->try_lock_upgradable().
   //!Notes: The upgradable_lock changes from a state of not owning the mutex,
   //!   to owning the mutex, but only if blocking was not required. If the
   //!   mutex_type does not support try_lock_upgradable(), this function will
   //!   fail at compile time if instantiated, but otherwise have no effect.
   bool try_lock()
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->try_lock_upgradable();
      return m_locked;
   }

   //!Effects: If mutex() == 0 or if already locked, throws a lock_exception()
   //!   exception. Calls timed_lock_upgradable(abs_time) on the referenced mutex.
   //!Postconditions: owns() == the value returned from
   //!   mutex()->timed_lock_upgradable(abs_time).
   //!Notes: The upgradable_lock changes from a state of not owning the mutex,
   //!   to owning the mutex, but only if it can obtain ownership within the
   //!   specified time. If the mutex_type does not support
   //!   timed_lock_upgradable(abs_time), this function will fail at compile
   //!   time if instantiated, but otherwise have no effect.
   template<class TimePoint>
   bool timed_lock(const TimePoint& abs_time)
   {
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->timed_lock_upgradable(abs_time);
      return m_locked;
   }

   //!Effects: If mutex() == 0 or if not locked, throws a lock_exception()
   //!   exception. Calls unlock_upgradable() on the referenced mutex.
   //!Postconditions: owns() == false.
   //!Notes: The upgradable_lock changes from a state of owning the mutex,
   //!   to not owning the mutex.
   void unlock()
   {
      if(!mp_mutex || !m_locked)
         throw lock_exception();
      mp_mutex->unlock_upgradable();
      m_locked = false;
   }

   //!Effects: Returns true if this scoped_lock has acquired the
   //!referenced mutex.
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
   void swap(upgradable_lock<mutex_type> &other) BOOST_NOEXCEPT
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

#endif // BOOST_INTERPROCESS_UPGRADABLE_LOCK_HPP
