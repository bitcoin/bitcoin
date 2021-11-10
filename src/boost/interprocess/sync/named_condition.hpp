//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_CONDITION_HPP
#define BOOST_INTERPROCESS_NAMED_CONDITION_HPP

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
#include <boost/interprocess/sync/detail/locks.hpp>
#if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_WINDOWS)
   #include <boost/interprocess/sync/windows/named_condition.hpp>
   #define BOOST_INTERPROCESS_NAMED_CONDITION_USE_WINAPI
#else
   #include <boost/interprocess/sync/shm/named_condition.hpp>
#endif

//!\file
//!Describes a named condition class for inter-process synchronization

namespace boost {
namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
namespace ipcdetail{ class interprocess_tester; }
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//! A global condition variable that can be created by name.
//! This condition variable is designed to work with named_mutex and
//! can't be placed in shared memory or memory mapped files.
class named_condition
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   named_condition();
   named_condition(const named_condition &);
   named_condition &operator=(const named_condition &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:

   //!Creates a global condition with a name.
   //!If the condition can't be created throws interprocess_exception
   named_condition(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global condition with a name.
   //!If the condition is created, this call is equivalent to
   //!named_condition(create_only_t, ... )
   //!If the condition is already created, this call is equivalent
   //!named_condition(open_only_t, ... )
   //!Does not throw
   named_condition(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global condition with a name if that condition is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_condition(open_only_t open_only, const char *name);

   //!Opens a global condition with a name if that condition is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!Creates a global condition with a name.
   //!If the condition can't be created throws interprocess_exception
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_condition(create_only_t create_only, const wchar_t *name, const permissions &perm = permissions());

   //!Opens or creates a global condition with a name.
   //!If the condition is created, this call is equivalent to
   //!named_condition(create_only_t, ... )
   //!If the condition is already created, this call is equivalent
   //!named_condition(open_only_t, ... )
   //!Does not throw
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_condition(open_or_create_t open_or_create, const wchar_t *name, const permissions &perm = permissions());

   //!Opens a global condition with a name if that condition is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   named_condition(open_only_t open_only, const wchar_t *name);

   #endif //#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_condition();

   //!If there is a thread waiting on *this, change that
   //!thread's state to ready. Otherwise there is no effect.*/
   void notify_one();

   //!Change the state of all threads waiting on *this to ready.
   //!If there are no waiting threads, notify_all() has no effect.
   void notify_all();

   //!Releases the lock on the named_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), and then reacquires the lock.
   template <typename L>
   void wait(L& lock);

   //!The same as:
   //!while (!pred()) wait(lock)
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred);

   //!Releases the lock on the named_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), or until time abs_time is reached,
   //!and then reacquires the lock.
   //!Returns: false if time abs_time is reached, otherwise true.
   template <typename L, class TimePoint>
   bool timed_wait(L& lock, const TimePoint &abs_time);

   //!The same as:   while (!pred()) {
   //!                  if (!timed_wait(lock, abs_time)) return pred();
   //!               } return true;
   template <typename L, class TimePoint, typename Pr>
   bool timed_wait(L& lock, const TimePoint &abs_time, Pr pred);

   //!Erases a named condition from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Erases a named condition from the system.
   //!Returns false on error. Never throws.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool remove(const wchar_t *name);

   #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   #if defined(BOOST_INTERPROCESS_NAMED_CONDITION_USE_WINAPI)
   typedef ipcdetail::winapi_named_condition   condition_type;
   #else
   typedef ipcdetail::shm_named_condition       condition_type;
   #endif
   condition_type m_cond;

   friend class ipcdetail::interprocess_tester;
   void dont_close_on_destruction()
   {  ipcdetail::interprocess_tester::dont_close_on_destruction(m_cond); }
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline named_condition::~named_condition()
{}

inline named_condition::named_condition(create_only_t, const char *name, const permissions &perm)
   :  m_cond(create_only_t(), name, perm)
{}

inline named_condition::named_condition(open_or_create_t, const char *name, const permissions &perm)
   :  m_cond(open_or_create_t(), name, perm)
{}

inline named_condition::named_condition(open_only_t, const char *name)
   :  m_cond(open_only_t(), name)
{}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline named_condition::named_condition(create_only_t, const wchar_t *name, const permissions &perm)
   :  m_cond(create_only_t(), name, perm)
{}

inline named_condition::named_condition(open_or_create_t, const wchar_t *name, const permissions &perm)
   :  m_cond(open_or_create_t(), name, perm)
{}

inline named_condition::named_condition(open_only_t, const wchar_t *name)
   :  m_cond(open_only_t(), name)
{}

#endif //#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)


inline void named_condition::notify_one()
{  m_cond.notify_one();  }

inline void named_condition::notify_all()
{  m_cond.notify_all();  }

template <typename L>
inline void named_condition::wait(L& lock)
{
   ipcdetail::internal_mutex_lock<L> internal_lock(lock);
   m_cond.wait(internal_lock);
}

template <typename L, typename Pr>
inline void named_condition::wait(L& lock, Pr pred)
{
   ipcdetail::internal_mutex_lock<L> internal_lock(lock);
   m_cond.wait(internal_lock, pred);
}

template <typename L, typename TimePoint>
inline bool named_condition::timed_wait
   (L& lock, const TimePoint &abs_time)
{
   ipcdetail::internal_mutex_lock<L> internal_lock(lock);
   return m_cond.timed_wait(internal_lock, abs_time);
}

template <typename L, typename TimePoint, typename Pr>
inline bool named_condition::timed_wait
   (L& lock, const TimePoint &abs_time, Pr pred)
{
   ipcdetail::internal_mutex_lock<L> internal_lock(lock);
   return m_cond.timed_wait(internal_lock, abs_time, pred);
}

inline bool named_condition::remove(const char *name)
{
   return condition_type::remove(name);
}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline bool named_condition::remove(const wchar_t *name)
{
   return condition_type::remove(name);
}

#endif


#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_NAMED_CONDITION_HPP
