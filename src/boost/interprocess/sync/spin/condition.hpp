//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP
#define BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/detail/timed_utils.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class spin_condition
{
   spin_condition(const spin_condition &);
   spin_condition &operator=(const spin_condition &);

   public:
   spin_condition()
   {
      //Note that this class is initialized to zero.
      //So zeroed memory can be interpreted as an initialized
      //condition variable
      m_command      = SLEEP;
      m_num_waiters  = 0;
   }

   ~spin_condition()
   {
      //Notify all waiting threads
      //to allow POSIX semantics on condition destruction
      this->notify_all();
   }

   void notify_one()
   {  this->notify(NOTIFY_ONE);  }

   void notify_all()
   {  this->notify(NOTIFY_ALL);  }

   template <typename L, typename TimePoint>
   bool timed_wait(L& lock, const TimePoint &abs_time)
   {
      if (!lock)
         throw lock_exception();
      //Handle infinity absolute time here to avoid complications in do_timed_wait
      if(is_pos_infinity(abs_time)){
         this->wait(lock);
         return true;
      }
      return this->do_timed_wait_impl<true>(abs_time, *lock.mutex());
   }

   template <typename L, typename TimePoint, typename Pr>
   bool timed_wait(L& lock, const TimePoint &abs_time, Pr pred)
   {
      if (!lock)
         throw lock_exception();
      //Handle infinity absolute time here to avoid complications in do_timed_wait
      if(is_pos_infinity(abs_time)){
         this->wait(lock, pred);
         return true;
      }
      while (!pred()){
         if (!this->do_timed_wait_impl<true>(abs_time, *lock.mutex()))
            return pred();
      }
      return true;
   }

   template <typename L>
   void wait(L& lock)
   {
      if (!lock)
         throw lock_exception();
      this->do_timed_wait_impl<false>(0, *lock.mutex());
   }

   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {
      if (!lock)
         throw lock_exception();

      while (!pred())
         this->do_timed_wait_impl<false>(0, *lock.mutex());
   }

   private:

   template<bool TimeoutEnabled, class InterprocessMutex, class TimePoint>
   bool do_timed_wait_impl(const TimePoint &abs_time, InterprocessMutex &mut)
   {
      typedef boost::interprocess::scoped_lock<spin_mutex> InternalLock;
      //The enter mutex guarantees that while executing a notification,
      //no other thread can execute the do_timed_wait method.
      {
         //---------------------------------------------------------------
         InternalLock lock;
         get_lock(bool_<TimeoutEnabled>(), m_enter_mut, lock, abs_time);

         if(!lock)
            return false;
         //---------------------------------------------------------------
         //We increment the waiting thread count protected so that it will be
         //always constant when another thread enters the notification logic.
         //The increment marks this thread as "waiting on spin_condition"
         atomic_inc32(const_cast<boost::uint32_t*>(&m_num_waiters));

         //We unlock the external mutex atomically with the increment
         mut.unlock();
      }

      //By default, we suppose that no timeout has happened
      bool timed_out  = false, unlock_enter_mut= false;

      //Loop until a notification indicates that the thread should
      //exit or timeout occurs
      while(1){
         //The thread sleeps/spins until a spin_condition commands a notification
         //Notification occurred, we will lock the checking mutex so that
         spin_wait swait;
         while(atomic_read32(&m_command) == SLEEP){
            swait.yield();

            //Check for timeout
            if(TimeoutEnabled){
               TimePoint now = get_now<TimePoint>(bool_<TimeoutEnabled>());

               if(now >= abs_time){
                  //If we can lock the mutex it means that no notification
                  //is being executed in this spin_condition variable
                  timed_out = m_enter_mut.try_lock();

                  //If locking fails, indicates that another thread is executing
                  //notification, so we play the notification game
                  if(!timed_out){
                     //There is an ongoing notification, we will try again later
                     continue;
                  }
                  //No notification in execution, since enter mutex is locked.
                  //We will execute time-out logic, so we will decrement count,
                  //release the enter mutex and return false.
                  break;
               }
            }
         }

         //If a timeout occurred, the mutex will not execute checking logic
         if(TimeoutEnabled && timed_out){
            //Decrement wait count
            atomic_dec32(const_cast<boost::uint32_t*>(&m_num_waiters));
            unlock_enter_mut = true;
            break;
         }
         else{
            boost::uint32_t result = atomic_cas32
                           (const_cast<boost::uint32_t*>(&m_command), SLEEP, NOTIFY_ONE);
            if(result == SLEEP){
               //Other thread has been notified and since it was a NOTIFY one
               //command, this thread must sleep again
               continue;
            }
            else if(result == NOTIFY_ONE){
               //If it was a NOTIFY_ONE command, only this thread should
               //exit. This thread has atomically marked command as sleep before
               //so no other thread will exit.
               //Decrement wait count.
               unlock_enter_mut = true;
               atomic_dec32(const_cast<boost::uint32_t*>(&m_num_waiters));
               break;
            }
            else{
               //If it is a NOTIFY_ALL command, all threads should return
               //from do_timed_wait function. Decrement wait count.
               unlock_enter_mut = 1 == atomic_dec32(const_cast<boost::uint32_t*>(&m_num_waiters));
               //Check if this is the last thread of notify_all waiters
               //Only the last thread will release the mutex
               if(unlock_enter_mut){
                  atomic_cas32(const_cast<boost::uint32_t*>(&m_command), SLEEP, NOTIFY_ALL);
               }
               break;
            }
         }
      }

      //Unlock the enter mutex if it is a single notification, if this is
      //the last notified thread in a notify_all or a timeout has occurred
      if(unlock_enter_mut){
         m_enter_mut.unlock();
      }

      //Lock external again before returning from the method
      mut.lock();
      return !timed_out;
   }

   template <class TimePoint>
   static TimePoint get_now(bool_<true>)
   {  return microsec_clock<TimePoint>::universal_time();  }

   template <class TimePoint>
   static TimePoint get_now(bool_<false>)
   {  return TimePoint();  }

   template <class Mutex, class Lock, class TimePoint>
   static void  get_lock(bool_<true>, Mutex &m, Lock &lck, const TimePoint &abs_time)
   { 
      Lock dummy(m, abs_time);
      lck = boost::move(dummy);
   }

   template <class Mutex, class Lock, class TimePoint>
   static void get_lock(bool_<false>, Mutex &m, Lock &lck, const TimePoint &)
   { 
      Lock dummy(m);
      lck = boost::move(dummy);
   }

   void notify(boost::uint32_t command)
   {
      //This mutex guarantees that no other thread can enter to the
      //do_timed_wait method logic, so that thread count will be
      //constant until the function writes a NOTIFY_ALL command.
      //It also guarantees that no other notification can be signaled
      //on this spin_condition before this one ends
      m_enter_mut.lock();

      //Return if there are no waiters
      if(!atomic_read32(&m_num_waiters)) {
         m_enter_mut.unlock();
         return;
      }

      //Notify that all threads should execute wait logic
      spin_wait swait;
      while(SLEEP != atomic_cas32(const_cast<boost::uint32_t*>(&m_command), command, SLEEP)){
         swait.yield();
      }
      //The enter mutex will rest locked until the last waiting thread unlocks it
   }

   enum { SLEEP = 0, NOTIFY_ONE, NOTIFY_ALL };
   spin_mutex  m_enter_mut;
   volatile boost::uint32_t    m_command;
   volatile boost::uint32_t    m_num_waiters;
};

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP
