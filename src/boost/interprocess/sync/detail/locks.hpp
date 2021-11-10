//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_LOCKS_HPP
#define BOOST_INTERPROCESS_DETAIL_LOCKS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

template<class Lock>
class internal_mutex_lock
{
   typedef void (internal_mutex_lock::*unspecified_bool_type)();
   public:

   typedef typename Lock::mutex_type::internal_mutex_type  mutex_type;


   BOOST_INTERPROCESS_FORCEINLINE internal_mutex_lock(Lock &l)
      : l_(l)
   {}

   BOOST_INTERPROCESS_FORCEINLINE mutex_type* mutex() const
   {  return l_ ? &l_.mutex()->internal_mutex() : 0;  }

   BOOST_INTERPROCESS_FORCEINLINE void lock()    { l_.lock(); }

   BOOST_INTERPROCESS_FORCEINLINE void unlock()  { l_.unlock(); }

   BOOST_INTERPROCESS_FORCEINLINE operator unspecified_bool_type() const
   {  return l_ ? &internal_mutex_lock::lock : 0;  }

   private:
   Lock &l_;
};

template <class Lock>
class lock_inverter
{
   Lock &l_;
   public:
   BOOST_INTERPROCESS_FORCEINLINE lock_inverter(Lock &l)
      :  l_(l)
   {}

   BOOST_INTERPROCESS_FORCEINLINE void lock()    {   l_.unlock();   }

   BOOST_INTERPROCESS_FORCEINLINE void unlock()  {   l_.lock();     }
};

template <class Lock>
class lock_to_sharable
{
   Lock &l_;

   public:
   BOOST_INTERPROCESS_FORCEINLINE explicit lock_to_sharable(Lock &l)
      :  l_(l)
   {}

   BOOST_INTERPROCESS_FORCEINLINE void lock()    {  l_.lock_sharable();     }

   BOOST_INTERPROCESS_FORCEINLINE bool try_lock(){  return l_.try_lock_sharable(); }

   BOOST_INTERPROCESS_FORCEINLINE void unlock()  {  l_.unlock_sharable();   }
};

template <class Lock>
class lock_to_wait
{
   Lock &l_;

   public:
   BOOST_INTERPROCESS_FORCEINLINE explicit lock_to_wait(Lock &l)
      :  l_(l)
   {}
   BOOST_INTERPROCESS_FORCEINLINE void lock()     {  l_.wait();     }

   BOOST_INTERPROCESS_FORCEINLINE bool try_lock() {  return l_.try_wait(); }

   template<class TimePoint>
   BOOST_INTERPROCESS_FORCEINLINE bool timed_lock(const TimePoint &abs_time)
      {  return l_.timed_wait(abs_time);   }
};

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_LOCKS_HPP
