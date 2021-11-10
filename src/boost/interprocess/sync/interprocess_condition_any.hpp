//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CONDITION_ANY_HPP
#define BOOST_INTERPROCESS_CONDITION_ANY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/detail/condition_any_algorithm.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!\file
//!Describes process-shared variables interprocess_condition_any class

namespace boost {
namespace interprocess {

//!This class is a condition variable that can be placed in shared memory or
//!memory mapped files.
//!
//!The interprocess_condition_any class is a generalization of interprocess_condition.
//!Whereas interprocess_condition works only on Locks with mutex_type == interprocess_mutex
//!interprocess_condition_any can operate on any user-defined lock that meets the BasicLockable
//!requirements (lock()/unlock() member functions).
//!
//!Unlike std::condition_variable_any in C++11, it is NOT safe to invoke the destructor if all
//!threads have been only notified. It is required that they have exited their respective wait
//!functions.
class interprocess_condition_any
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   interprocess_condition_any(const interprocess_condition_any &);
   interprocess_condition_any &operator=(const interprocess_condition_any &);

   class members
   {
      public:
      typedef interprocess_condition   condvar_type;
      typedef interprocess_mutex       mutex_type;

      condvar_type &get_condvar() {  return m_cond;  }
      mutex_type   &get_mutex()   {  return m_mut; }

      private:
      condvar_type   m_cond;
      mutex_type     m_mut;
   };

   ipcdetail::condition_any_wrapper<members>   m_cond;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   //!Constructs a interprocess_condition_any. On error throws interprocess_exception.
   interprocess_condition_any(){}

   //!Destroys *this
   //!liberating system resources.
   ~interprocess_condition_any(){}

   //!If there is a thread waiting on *this, change that
   //!thread's state to ready. Otherwise there is no effect.
   void notify_one()
   {  m_cond.notify_one();  }

   //!Change the state of all threads waiting on *this to ready.
   //!If there are no waiting threads, notify_all() has no effect.
   void notify_all()
   {  m_cond.notify_all();  }

   //!Releases the lock on the interprocess_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), and then reacquires the lock.
   template <typename L>
   void wait(L& lock)
   {  m_cond.wait(lock);  }

   //!The same as:
   //!while (!pred()) wait(lock)
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {  m_cond.wait(lock, pred);  }

   //!Releases the lock on the interprocess_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), or until time abs_time is reached,
   //!and then reacquires the lock.
   //!Returns: false if time abs_time is reached, otherwise true.
   template <typename L, class TimePoint>
   bool timed_wait(L& lock, const TimePoint &abs_time)
   {  return m_cond.timed_wait(lock, abs_time);  }

   //!The same as:   while (!pred()) {
   //!                  if (!timed_wait(lock, abs_time)) return pred();
   //!               } return true;
   template <typename L, class TimePoint, typename Pr>
   bool timed_wait(L& lock, const TimePoint &abs_time, Pr pred)
   {  return m_cond.timed_wait(lock, abs_time, pred);  }
};

}  //namespace interprocess
}  // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_CONDITION_ANY_HPP
