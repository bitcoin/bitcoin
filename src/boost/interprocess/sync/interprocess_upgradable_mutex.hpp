////////////////////////////////////////////////////////////////////////////////
//
//  Code based on Howard Hinnant's upgrade_mutex class
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP
#define BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <climits>


//!\file
//!Describes interprocess_upgradable_mutex class

namespace boost {
namespace interprocess {

//!Wraps a interprocess_upgradable_mutex that can be placed in shared memory and can be
//!shared between processes. Allows timed lock tries
class interprocess_upgradable_mutex
{
   //Non-copyable
   interprocess_upgradable_mutex(const interprocess_upgradable_mutex &);
   interprocess_upgradable_mutex &operator=(const interprocess_upgradable_mutex &);

   friend class interprocess_condition;
   public:

   //!Constructs the upgradable lock.
   //!Throws interprocess_exception on error.
   interprocess_upgradable_mutex();

   //!Destroys the upgradable lock.
   //!Does not throw.
   ~interprocess_upgradable_mutex();

   //Exclusive locking

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to obtain exclusive ownership of the mutex,
   //!   and if another thread has exclusive, sharable or upgradable ownership of
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
   //!   without waiting. If no other thread has exclusive, sharable or upgradable
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
   //!   waiting if necessary until no other thread has exclusive, sharable or
   //!   upgradable ownership of the mutex or abs_time is reached.
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
   //!   and if another thread has exclusive ownership of the mutex,
   //!   waits until it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   void lock_sharable();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   without waiting. If no other thread has exclusive ownership
   //!   of the mutex this succeeds.
   //!Returns: If it can acquire sharable ownership immediately returns true. If it
   //!   has to wait, returns false.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   bool try_lock_sharable();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive
   //!   ownership of the mutex or abs_time is reached.
   //!Returns: If acquires sharable ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   //! 
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   template<class TimePoint>
   bool timed_lock_sharable(const TimePoint &abs_time);

   //!Precondition: The thread must have sharable ownership of the mutex.
   //!Effects: The calling thread releases the sharable ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_sharable();

   //Upgradable locking

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to obtain upgradable ownership of the mutex,
   //!   and if another thread has exclusive or upgradable ownership of the mutex,
   //!   waits until it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   void lock_upgradable();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire upgradable ownership of the mutex
   //!   without waiting. If no other thread has exclusive or upgradable ownership
   //!   of the mutex this succeeds.
   //!Returns: If it can acquire upgradable ownership immediately returns true.
   //!   If it has to wait, returns false.
   //!Throws: interprocess_exception on error.
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   bool try_lock_upgradable();

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to acquire upgradable ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive or upgradable
   //!   ownership of the mutex or abs_time is reached.
   //!Returns: If acquires upgradable ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   //!
   //!Note: A program may deadlock if the thread that has ownership calls 
   //!   this function. If the implementation can detect the deadlock,
   //!   an exception could be thrown.
   template<class TimePoint>
   bool timed_lock_upgradable(const TimePoint &abs_time);

   //!Precondition: The thread must have upgradable ownership of the mutex.
   //!Effects: The calling thread releases the upgradable ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_upgradable();

   //Demotions

   //!Precondition: The thread must have exclusive ownership of the mutex.
   //!Effects: The thread atomically releases exclusive ownership and acquires
   //!   upgradable ownership. This operation is non-blocking.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_and_lock_upgradable();

   //!Precondition: The thread must have exclusive ownership of the mutex.
   //!Effects: The thread atomically releases exclusive ownership and acquires
   //!   sharable ownership. This operation is non-blocking.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_and_lock_sharable();

   //!Precondition: The thread must have upgradable ownership of the mutex.
   //!Effects: The thread atomically releases upgradable ownership and acquires
   //!   sharable ownership. This operation is non-blocking.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_upgradable_and_lock_sharable();

   //Promotions

   //!Precondition: The thread must have upgradable ownership of the mutex.
   //!Effects: The thread atomically releases upgradable ownership and acquires
   //!   exclusive ownership. This operation will block until all threads with
   //!   sharable ownership release their sharable lock.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_upgradable_and_lock();

   //!Precondition: The thread must have upgradable ownership of the mutex.
   //!Effects: The thread atomically releases upgradable ownership and tries to
   //!   acquire exclusive ownership. This operation will fail if there are threads
   //!   with sharable ownership, but it will maintain upgradable ownership.
   //!Returns: If acquires exclusive ownership, returns true. Otherwise returns false.
   //!Throws: An exception derived from interprocess_exception on error.
   bool try_unlock_upgradable_and_lock();

   //!Precondition: The thread must have upgradable ownership of the mutex.
   //!Effects: The thread atomically releases upgradable ownership and tries to acquire
   //!   exclusive ownership, waiting if necessary until abs_time. This operation will
   //!   fail if there are threads with sharable ownership or timeout reaches, but it
   //!   will maintain upgradable ownership.
   //!Returns: If acquires exclusive ownership, returns true. Otherwise returns false.
   //!Throws: An exception derived from interprocess_exception on error. */
   template<class TimePoint>
   bool timed_unlock_upgradable_and_lock(const TimePoint &abs_time);

   //!Precondition: The thread must have sharable ownership of the mutex.
   //!Effects: The thread atomically releases sharable ownership and tries to acquire
   //!   exclusive ownership. This operation will fail if there are threads with sharable
   //!   or upgradable ownership, but it will maintain sharable ownership.
   //!Returns: If acquires exclusive ownership, returns true. Otherwise returns false.
   //!Throws: An exception derived from interprocess_exception on error.
   bool try_unlock_sharable_and_lock();

   //!Precondition: The thread must have sharable ownership of the mutex.
   //!Effects: The thread atomically releases sharable ownership and tries to acquire
   //!   upgradable ownership. This operation will fail if there are threads with sharable
   //!   or upgradable ownership, but it will maintain sharable ownership.
   //!Returns: If acquires upgradable ownership, returns true. Otherwise returns false.
   //!Throws: An exception derived from interprocess_exception on error.
   bool try_unlock_sharable_and_lock_upgradable();

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef scoped_lock<interprocess_mutex> scoped_lock_t;

   //Pack all the control data in a word to be able
   //to use atomic instructions in the future
   struct control_word_t
   {
      unsigned exclusive_in         : 1;
      unsigned upgradable_in        : 1;
      unsigned num_upr_shar         : sizeof(unsigned)*CHAR_BIT-2;
   }                       m_ctrl;

   interprocess_mutex      m_mut;
   interprocess_condition  m_first_gate;
   interprocess_condition  m_second_gate;

   private:
   //Rollback structures for exceptions or failure return values
   struct exclusive_rollback
   {
      exclusive_rollback(control_word_t         &ctrl
                        ,interprocess_condition &first_gate)
         :  mp_ctrl(&ctrl), m_first_gate(first_gate)
      {}

      void release()
      {  mp_ctrl = 0;   }

      ~exclusive_rollback()
      {
         if(mp_ctrl){
            mp_ctrl->exclusive_in = 0;
            m_first_gate.notify_all();
         }
      }
      control_word_t          *mp_ctrl;
      interprocess_condition  &m_first_gate;
   };

   struct upgradable_to_exclusive_rollback
   {
      upgradable_to_exclusive_rollback(control_word_t         &ctrl)
         :  mp_ctrl(&ctrl)
      {}

      void release()
      {  mp_ctrl = 0;   }

      ~upgradable_to_exclusive_rollback()
      {
         if(mp_ctrl){
            //Recover upgradable lock
            mp_ctrl->upgradable_in = 1;
            ++mp_ctrl->num_upr_shar;
            //Execute the second half of exclusive locking
            mp_ctrl->exclusive_in = 0;
         }
      }
      control_word_t          *mp_ctrl;
   };

   template<int Dummy>
   struct base_constants_t
   {
      static const unsigned max_readers
         = ~(unsigned(3) << (sizeof(unsigned)*CHAR_BIT-2));
   };
   typedef base_constants_t<0> constants;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template <int Dummy>
const unsigned interprocess_upgradable_mutex::base_constants_t<Dummy>::max_readers;

inline interprocess_upgradable_mutex::interprocess_upgradable_mutex()
{
   this->m_ctrl.exclusive_in  = 0;
   this->m_ctrl.upgradable_in = 0;
   this->m_ctrl.num_upr_shar   = 0;
}

inline interprocess_upgradable_mutex::~interprocess_upgradable_mutex()
{}

inline void interprocess_upgradable_mutex::lock()
{
   scoped_lock_t lck(m_mut);

   //The exclusive lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   while (this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in){
      this->m_first_gate.wait(lck);
   }

   //Mark that exclusive lock has been acquired
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   exclusive_rollback rollback(this->m_ctrl, this->m_first_gate);

   //Now wait until all readers are gone
   while (this->m_ctrl.num_upr_shar){
      this->m_second_gate.wait(lck);
   }
   rollback.release();
}

inline bool interprocess_upgradable_mutex::try_lock()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //If we can't lock or any has there is any exclusive, upgradable
   //or sharable mark return false;
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.num_upr_shar){
      return false;
   }
   this->m_ctrl.exclusive_in = 1;
   return true;
}

template<class TimePoint>
bool interprocess_upgradable_mutex::timed_lock(const TimePoint &abs_time)
{
   //Mutexes and condvars handle just fine infinite abs_times
   //so avoid checking it here
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //The exclusive lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   while (this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in){
      if(!this->m_first_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in){
            return false;
         }
         break;
      }
   }

   //Mark that exclusive lock has been acquired
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   exclusive_rollback rollback(this->m_ctrl, this->m_first_gate);

   //Now wait until all readers are gone
   while (this->m_ctrl.num_upr_shar){
      if(!this->m_second_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.num_upr_shar){
            return false;
         }
         break;
      }
   }
   rollback.release();
   return true;
}

inline void interprocess_upgradable_mutex::unlock()
{
   scoped_lock_t lck(m_mut);
   this->m_ctrl.exclusive_in = 0;
   this->m_first_gate.notify_all();
}

//Upgradable locking

inline void interprocess_upgradable_mutex::lock_upgradable()
{
   scoped_lock_t lck(m_mut);

   //The upgradable lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
   while(this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in
         || this->m_ctrl.num_upr_shar == constants::max_readers){
      this->m_first_gate.wait(lck);
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
}

inline bool interprocess_upgradable_mutex::try_lock_upgradable()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //The upgradable lock must fail
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.upgradable_in
      || this->m_ctrl.num_upr_shar == constants::max_readers){
      return false;
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
   return true;
}

template<class TimePoint>
bool interprocess_upgradable_mutex::timed_lock_upgradable(const TimePoint &abs_time)
{
   //Mutexes and condvars handle just fine infinite abs_times
   //so avoid checking it here
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //The upgradable lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
   while(this->m_ctrl.exclusive_in
         || this->m_ctrl.upgradable_in
         || this->m_ctrl.num_upr_shar == constants::max_readers){
      if(!this->m_first_gate.timed_wait(lck, abs_time)){
         if((this->m_ctrl.exclusive_in
             || this->m_ctrl.upgradable_in
             || this->m_ctrl.num_upr_shar == constants::max_readers)){
            return false;
         }
         break;
      }
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
   return true;
}

inline void interprocess_upgradable_mutex::unlock_upgradable()
{
   scoped_lock_t lck(m_mut);
   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;
   this->m_first_gate.notify_all();
}

//Sharable locking

inline void interprocess_upgradable_mutex::lock_sharable()
{
   scoped_lock_t lck(m_mut);

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   while(this->m_ctrl.exclusive_in
        || this->m_ctrl.num_upr_shar == constants::max_readers){
      this->m_first_gate.wait(lck);
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
}

inline bool interprocess_upgradable_mutex::try_lock_sharable()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //The sharable lock must fail
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.num_upr_shar == constants::max_readers){
      return false;
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
   return true;
}

template<class TimePoint>
inline bool interprocess_upgradable_mutex::timed_lock_sharable(const TimePoint &abs_time)
{
   //Mutexes and condvars handle just fine infinite abs_times
   //so avoid checking it here
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   while (this->m_ctrl.exclusive_in
         || this->m_ctrl.num_upr_shar == constants::max_readers){
      if(!this->m_first_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.exclusive_in
            || this->m_ctrl.num_upr_shar == constants::max_readers){
            return false;
         }
         break;
      }
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
   return true;
}

inline void interprocess_upgradable_mutex::unlock_sharable()
{
   scoped_lock_t lck(m_mut);
   //Decrement sharable count
   --this->m_ctrl.num_upr_shar;
   if (this->m_ctrl.num_upr_shar == 0){
      this->m_second_gate.notify_one();
   }
   //Check if there are blocked sharables because of
   //there were too many sharables
   else if(this->m_ctrl.num_upr_shar == (constants::max_readers-1)){
      this->m_first_gate.notify_all();
   }
}

//Downgrading

inline void interprocess_upgradable_mutex::unlock_and_lock_upgradable()
{
   scoped_lock_t lck(m_mut);
   //Unmark it as exclusive
   this->m_ctrl.exclusive_in     = 0;
   //Mark it as upgradable
   this->m_ctrl.upgradable_in    = 1;
   //The sharable count should be 0 so increment it
   this->m_ctrl.num_upr_shar   = 1;
   //Notify readers that they can enter
   m_first_gate.notify_all();
}

inline void interprocess_upgradable_mutex::unlock_and_lock_sharable()
{
   scoped_lock_t lck(m_mut);
   //Unmark it as exclusive
   this->m_ctrl.exclusive_in   = 0;
   //The sharable count should be 0 so increment it
   this->m_ctrl.num_upr_shar   = 1;
   //Notify readers that they can enter
   m_first_gate.notify_all();
}

inline void interprocess_upgradable_mutex::unlock_upgradable_and_lock_sharable()
{
   scoped_lock_t lck(m_mut);
   //Unmark it as upgradable (we don't have to decrement count)
   this->m_ctrl.upgradable_in    = 0;
   //Notify readers/upgradable that they can enter
   m_first_gate.notify_all();
}

//Upgrading

inline void interprocess_upgradable_mutex::unlock_upgradable_and_lock()
{
   scoped_lock_t lck(m_mut);
   //Simulate unlock_upgradable() without
   //notifying sharables.
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;
   //Execute the second half of exclusive locking
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   upgradable_to_exclusive_rollback rollback(m_ctrl);

   while (this->m_ctrl.num_upr_shar){
      this->m_second_gate.wait(lck);
   }
   rollback.release();
}

inline bool interprocess_upgradable_mutex::try_unlock_upgradable_and_lock()
{
   scoped_lock_t lck(m_mut, try_to_lock);
   //Check if there are no readers
   if(!lck.owns()
      || this->m_ctrl.num_upr_shar != 1){
      return false;
   }
   //Now unlock upgradable and mark exclusive
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;
   this->m_ctrl.exclusive_in = 1;
   return true;
}

template<class TimePoint>
bool interprocess_upgradable_mutex::timed_unlock_upgradable_and_lock(const TimePoint &abs_time)
{
   //Mutexes and condvars handle just fine infinite abs_times
   //so avoid checking it here
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //Simulate unlock_upgradable() without
   //notifying sharables.
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;
   //Execute the second half of exclusive locking
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   upgradable_to_exclusive_rollback rollback(m_ctrl);

   while (this->m_ctrl.num_upr_shar){
      if(!this->m_second_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.num_upr_shar){
            return false;
         }
         break;
      }
   }
   rollback.release();
   return true;
}

inline bool interprocess_upgradable_mutex::try_unlock_sharable_and_lock()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //If we can't lock or any has there is any exclusive, upgradable
   //or sharable mark return false;
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.upgradable_in
      || this->m_ctrl.num_upr_shar != 1){
      return false;
   }
   this->m_ctrl.exclusive_in = 1;
   this->m_ctrl.num_upr_shar = 0;
   return true;
}

inline bool interprocess_upgradable_mutex::try_unlock_sharable_and_lock_upgradable()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //The upgradable lock must fail
   //if an exclusive or upgradable lock has been acquired
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.upgradable_in){
      return false;
   }

   //Mark that upgradable lock has been acquired
   this->m_ctrl.upgradable_in = 1;
   return true;
}

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP
