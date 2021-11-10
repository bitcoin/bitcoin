 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_WINAPI_SEMAPHORE_WRAPPER_HPP
#define BOOST_INTERPROCESS_DETAIL_WINAPI_SEMAPHORE_WRAPPER_HPP

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
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/detail/win32_api.hpp>
#include <boost/interprocess/sync/windows/winapi_wrapper_common.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <limits>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class winapi_semaphore_functions
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Non-copyable
   winapi_semaphore_functions(const winapi_semaphore_functions &);
   winapi_semaphore_functions &operator=(const winapi_semaphore_functions &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   winapi_semaphore_functions(void *hnd)
      : m_sem_hnd(hnd)
   {}

   void post(long count = 1)
   {
      long prev_count;
      winapi::release_semaphore(m_sem_hnd, count, &prev_count);
   }

   void wait()
   {  return winapi_wrapper_wait_for_single_object(m_sem_hnd);  }

   bool try_wait()
   {  return winapi_wrapper_try_wait_for_single_object(m_sem_hnd);  }

   template<class TimePoint>
   bool timed_wait(const TimePoint &abs_time)
   {  return winapi_wrapper_timed_wait_for_single_object(m_sem_hnd, abs_time);  }

   long value() const
   {
      long l_count, l_limit;
      if(!winapi::get_semaphore_info(m_sem_hnd, l_count, l_limit))
         return 0;
      return l_count;
   }

   long limit() const
   {
      long l_count, l_limit;
      if(!winapi::get_semaphore_info(m_sem_hnd, l_count, l_limit))
         return 0;
      return l_limit;
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   protected:
   void *m_sem_hnd;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};


//Swappable semaphore wrapper
class winapi_semaphore_wrapper
   : public winapi_semaphore_functions
{
   winapi_semaphore_wrapper(const winapi_semaphore_wrapper &);
   winapi_semaphore_wrapper &operator=(const winapi_semaphore_wrapper &);

   public:

   //Long is 32 bits in windows
   static const long MaxCount = long(0x7FFFFFFF);

   winapi_semaphore_wrapper(void *hnd = winapi::invalid_handle_value)
      : winapi_semaphore_functions(hnd)
   {}

   ~winapi_semaphore_wrapper()
   {  this->close(); }

   void *release()
   {
      void *hnd = m_sem_hnd;
      m_sem_hnd = winapi::invalid_handle_value;
      return hnd;
   }

   void *handle() const
   {  return m_sem_hnd; }

   template <class CharT>
   bool open_or_create( const CharT *name
                      , long sem_count
                      , long max_count
                      , const permissions &perm
                      , bool &created)
   {
      if(m_sem_hnd == winapi::invalid_handle_value){
         m_sem_hnd = winapi::open_or_create_semaphore
            ( name
            , sem_count
            , max_count
            , (winapi::interprocess_security_attributes*)perm.get_permissions()
            );
         created = winapi::get_last_error() != winapi::error_already_exists;
         return m_sem_hnd != winapi::invalid_handle_value;
      }
      else{
         return false;
      }
   }

   bool open_semaphore(const char *name)
   {
      if(m_sem_hnd == winapi::invalid_handle_value){
         m_sem_hnd = winapi::open_semaphore(name);
         return m_sem_hnd != winapi::invalid_handle_value;
      }
      else{
         return false;
      }
   }

   void close()
   {
      if(m_sem_hnd != winapi::invalid_handle_value){
         winapi::close_handle(m_sem_hnd);
         m_sem_hnd = winapi::invalid_handle_value;
      }
   }

   void swap(winapi_semaphore_wrapper &other)
   {  void *tmp = m_sem_hnd; m_sem_hnd = other.m_sem_hnd; other.m_sem_hnd = tmp;   }
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_WINAPI_SEMAPHORE_WRAPPER_HPP
