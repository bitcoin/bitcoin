//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP
#define BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/assert.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class spin_mutex
{
   spin_mutex(const spin_mutex &);
   spin_mutex &operator=(const spin_mutex &);
   public:

   spin_mutex();
   ~spin_mutex();

   void lock();
   bool try_lock();
   template<class TimePoint> bool timed_lock(const TimePoint &abs_time);
   void unlock();
   void take_ownership(){}
   private:
   volatile boost::uint32_t m_s;

   struct common_lock_wrapper
   {
      common_lock_wrapper(spin_mutex &sp)
         : m_sp(sp)
      {}

      void lock()
      {
         ipcdetail::try_based_lock(m_sp);
      }

      template<class TimePoint>
      bool timed_lock(const TimePoint &abs_time)
      {  return m_sp.timed_lock(abs_time);   }

      spin_mutex &m_sp;
   };
};

inline spin_mutex::spin_mutex()
   : m_s(0)
{
   //Note that this class is initialized to zero.
   //So zeroed memory can be interpreted as an
   //initialized mutex
}

inline spin_mutex::~spin_mutex()
{
   //Trivial destructor
}

inline void spin_mutex::lock(void)
{
   common_lock_wrapper clw(*this);
   ipcdetail::timeout_when_locking_aware_lock(clw);
}

inline bool spin_mutex::try_lock(void)
{
   boost::uint32_t prev_s = ipcdetail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 1, 0);
   return m_s == 1 && prev_s == 0;
}

template<class TimePoint>
inline bool spin_mutex::timed_lock(const TimePoint &abs_time)
{  return ipcdetail::try_based_timed_lock(*this, abs_time); }

inline void spin_mutex::unlock(void)
{  ipcdetail::atomic_cas32(const_cast<boost::uint32_t*>(&m_s), 0, 1);   }

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP
