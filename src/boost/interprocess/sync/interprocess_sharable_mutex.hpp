//////////////////////////////////////////////////////////////////////////////
//  Code based on Howard Hinnant's shared_mutex class
//
// (C) Copyright Howard Hinnant 2007-2010. Distributed under the Boost
// Software License, Version 1.0. (see http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARABLE_MUTEX_HPP
#define BOOST_INTERPROCESS_SHARABLE_MUTEX_HPP

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
//!Describes interprocess_sharable_mutex class

namespace boost {
namespace interprocess {

//!Wraps a interprocess_sharable_mutex that can be placed in shared memory and can be
//!shared between processes. Allows timed lock tries
class interprocess_sharable_mutex
{
   //Non-copyable
   interprocess_sharable_mutex(const interprocess_sharable_mutex &);
   interprocess_sharable_mutex &operator=(const interprocess_sharable_mutex &);

   friend class interprocess_condition;
   public:

   //!Constructs the sharable lock.
   //!Throws interprocess_exception on error.
   interprocess_sharable_mutex();

   //!Destroys the sharable lock.
   //!Does not throw.
   ~interprocess_sharable_mutex();

   //Exclusive locking

   //!Requires: The calling thread does not own the mutex.
   //!
   //!Effects: The calling thread tries to obtain exclusive ownership of the mutex,
   //!   and if another thread has exclusive or sharable ownership of
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
   //!   without waiting. If no other thread has exclusive or sharable
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
   //!   waiting if necessary until no other thread has exclusive or sharable
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

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef scoped_lock<interprocess_mutex> scoped_lock_t;

   //Pack all the control data in a word to be able
   //to use atomic instructions in the future
   struct control_word_t
   {
      unsigned exclusive_in   : 1;
      unsigned num_shared     : sizeof(unsigned)*CHAR_BIT-1;
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

   template<int Dummy>
   struct base_constants_t
   {
      static const unsigned max_readers
         = ~(unsigned(1) << (sizeof(unsigned)*CHAR_BIT-1));
   };
   typedef base_constants_t<0> constants;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template <int Dummy>
const unsigned interprocess_sharable_mutex::base_constants_t<Dummy>::max_readers;

inline interprocess_sharable_mutex::interprocess_sharable_mutex()
{
   this->m_ctrl.exclusive_in  = 0;
   this->m_ctrl.num_shared   = 0;
}

inline interprocess_sharable_mutex::~interprocess_sharable_mutex()
{}

inline void interprocess_sharable_mutex::lock()
{
   scoped_lock_t lck(m_mut);

   //The exclusive lock must block in the first gate
   //if an exclusive lock has been acquired
   while (this->m_ctrl.exclusive_in){
      this->m_first_gate.wait(lck);
   }

   //Mark that exclusive lock has been acquired
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   exclusive_rollback rollback(this->m_ctrl, this->m_first_gate);

   //Now wait until all readers are gone
   while (this->m_ctrl.num_shared){
      this->m_second_gate.wait(lck);
   }
   rollback.release();
}

inline bool interprocess_sharable_mutex::try_lock()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //If we can't lock or any has there is any exclusive
   //or sharable mark return false;
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.num_shared){
      return false;
   }
   this->m_ctrl.exclusive_in = 1;
   return true;
}

template<class TimePoint>
inline bool interprocess_sharable_mutex::timed_lock
   (const TimePoint &abs_time)
{
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //The exclusive lock must block in the first gate
   //if an exclusive lock has been acquired
   while (this->m_ctrl.exclusive_in){
      //Mutexes and condvars handle just fine infinite abs_times
      //so avoid checking it here
      if(!this->m_first_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.exclusive_in){
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
   while (this->m_ctrl.num_shared){
      //Mutexes and condvars handle just fine infinite abs_times
      //so avoid checking it here
      if(!this->m_second_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.num_shared){
            return false;
         }
         break;
      }
   }
   rollback.release();
   return true;
}

inline void interprocess_sharable_mutex::unlock()
{
   scoped_lock_t lck(m_mut);
   this->m_ctrl.exclusive_in = 0;
   this->m_first_gate.notify_all();
}

//Sharable locking

inline void interprocess_sharable_mutex::lock_sharable()
{
   scoped_lock_t lck(m_mut);

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   while(this->m_ctrl.exclusive_in
        || this->m_ctrl.num_shared == constants::max_readers){
      this->m_first_gate.wait(lck);
   }

   //Increment sharable count
   ++this->m_ctrl.num_shared;
}

inline bool interprocess_sharable_mutex::try_lock_sharable()
{
   scoped_lock_t lck(m_mut, try_to_lock);

   //The sharable lock must fail
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   if(!lck.owns()
      || this->m_ctrl.exclusive_in
      || this->m_ctrl.num_shared == constants::max_readers){
      return false;
   }

   //Increment sharable count
   ++this->m_ctrl.num_shared;
   return true;
}

template<class TimePoint>
inline bool interprocess_sharable_mutex::timed_lock_sharable
   (const TimePoint &abs_time)
{
   scoped_lock_t lck(m_mut, abs_time);
   if(!lck.owns())   return false;

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
   while (this->m_ctrl.exclusive_in
         || this->m_ctrl.num_shared == constants::max_readers){
      //Mutexes and condvars handle just fine infinite abs_times
      //so avoid checking it here
      if(!this->m_first_gate.timed_wait(lck, abs_time)){
         if(this->m_ctrl.exclusive_in
               || this->m_ctrl.num_shared == constants::max_readers){
            return false;
         }
         break;
      }
   }

   //Increment sharable count
   ++this->m_ctrl.num_shared;
   return true;
}

inline void interprocess_sharable_mutex::unlock_sharable()
{
   scoped_lock_t lck(m_mut);
   //Decrement sharable count
   --this->m_ctrl.num_shared;
   if (this->m_ctrl.num_shared == 0){
      this->m_second_gate.notify_one();
   }
   //Check if there are blocked sharables because of
   //there were too many sharables
   else if(this->m_ctrl.num_shared == (constants::max_readers-1)){
      this->m_first_gate.notify_all();
   }
}

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SHARABLE_MUTEX_HPP
