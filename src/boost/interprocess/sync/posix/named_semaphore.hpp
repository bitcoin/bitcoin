//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_POSIX_NAMED_CONDITION_HPP
#define BOOST_INTERPROCESS_POSIX_NAMED_CONDITION_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/posix/semaphore_wrapper.hpp>

namespace boost {
namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
namespace ipcdetail{ class interprocess_tester; }
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

namespace ipcdetail {

class posix_named_semaphore
{
   posix_named_semaphore();
   posix_named_semaphore(const posix_named_semaphore&);
   posix_named_semaphore &operator= (const posix_named_semaphore &);

   public:
   posix_named_semaphore
      (create_only_t, const char *name, unsigned int initialCount, const permissions &perm = permissions())
   {  semaphore_open(mp_sem, DoCreate, name, initialCount, perm);   }

   posix_named_semaphore(open_or_create_t, const char *name, unsigned int initialCount, const permissions &perm = permissions())
   {  semaphore_open(mp_sem, DoOpenOrCreate, name, initialCount, perm);   }

   posix_named_semaphore(open_only_t, const char *name)
   {  semaphore_open(mp_sem, DoOpen, name);   }

   ~posix_named_semaphore()
   {
      if(mp_sem != BOOST_INTERPROCESS_POSIX_SEM_FAILED)
         semaphore_close(mp_sem);
   }

   void post()
   {  semaphore_post(mp_sem); }

   void wait()
   {  semaphore_wait(mp_sem); }

   bool try_wait()
   {  return semaphore_try_wait(mp_sem); }

   template<class TimePoint>
   bool timed_wait(const TimePoint &abs_time)
   {  return semaphore_timed_wait(mp_sem, abs_time); }

   static bool remove(const char *name)
   {  return semaphore_unlink(name);   }

   private:
   friend class ipcdetail::interprocess_tester;
   void dont_close_on_destruction()
   {  mp_sem = BOOST_INTERPROCESS_POSIX_SEM_FAILED; }

   sem_t      *mp_sem;
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_POSIX_NAMED_CONDITION_HPP
