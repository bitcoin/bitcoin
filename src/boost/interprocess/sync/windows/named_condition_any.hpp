 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_ANY_HPP
#define BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_ANY_HPP

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
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/sync/windows/named_sync.hpp>
#include <boost/interprocess/sync/windows/winapi_semaphore_wrapper.hpp>
#include <boost/interprocess/sync/detail/condition_algorithm_8a.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

template<class CharT>
struct named_cond_callbacks_str;

template<>
struct named_cond_callbacks_str<char>
{
   static const char* ipc_cond()
   {  return "Global\\bipc.cond.";  }

   static const char* bq()
   {  return "_bq";  }

   static const char* bl()
   {  return "_bl";  }

   static const char* ul()
   {  return "_ul";  }
};

template<>
struct named_cond_callbacks_str<wchar_t>
{
   static const wchar_t* ipc_cond()
   {  return L"Global\\bipc.cond.";  }

   static const wchar_t* bq()
   {  return L"_bq";  }

   static const wchar_t* bl()
   {  return L"_bl";  }

   static const wchar_t* ul()
   {  return L"_ul";  }
};

class winapi_named_condition_any
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Non-copyable
   winapi_named_condition_any();
   winapi_named_condition_any(const winapi_named_condition_any &);
   winapi_named_condition_any &operator=(const winapi_named_condition_any &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   winapi_named_condition_any
      (create_only_t, const char *name, const permissions &perm = permissions())
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoCreate, name, perm, callbacks);
   }

   winapi_named_condition_any
      (open_or_create_t, const char *name, const permissions &perm = permissions())
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoOpenOrCreate, name, perm, callbacks);
   }

   winapi_named_condition_any(open_only_t, const char *name)
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoOpen, name, permissions(), callbacks);
   }

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   winapi_named_condition_any
      (create_only_t, const wchar_t *name, const permissions &perm = permissions())
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoCreate, name, perm, callbacks);
   }

   winapi_named_condition_any
      (open_or_create_t, const wchar_t *name, const permissions &perm = permissions())
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoOpenOrCreate, name, perm, callbacks);
   }

   winapi_named_condition_any(open_only_t, const wchar_t *name)
      : m_condition_data()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.open_or_create(DoOpen, name, permissions(), callbacks);
   }

   #endif   //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   ~winapi_named_condition_any()
   {
      named_cond_callbacks callbacks(m_condition_data.get_members());
      m_named_sync.close(callbacks);
   }

   void notify_one()
   {  m_condition_data.notify_one();   }

   void notify_all()
   {  m_condition_data.notify_all();   }

   template <typename L, typename TimePoint>
   bool timed_wait(L& lock, const TimePoint &abs_time)
   {  return m_condition_data.timed_wait(lock, abs_time);   }

   template <typename L, typename TimePoint, typename Pr>
   bool timed_wait(L& lock, const TimePoint &abs_time, Pr pred)
   {  return m_condition_data.timed_wait(lock, abs_time, pred);   }

   template <typename L>
   void wait(L& lock)
   {  m_condition_data.wait(lock);   }

   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {  m_condition_data.wait(lock, pred);   }

   static bool remove(const char *name)
   {  return windows_named_sync::remove(name);  }

   static bool remove(const wchar_t *name)
   {  return windows_named_sync::remove(name);  }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   void dont_close_on_destruction()
   {}

   friend class interprocess_tester;

   struct condition_data
   {
      typedef boost::int32_t           integer_type;
      typedef winapi_semaphore_wrapper semaphore_type;
      typedef winapi_mutex_wrapper     mutex_type;

      integer_type    &get_nwaiters_blocked()
      {  return m_nwaiters_blocked;  }

      integer_type    &get_nwaiters_gone()
      {  return m_nwaiters_gone;  }

      integer_type    &get_nwaiters_to_unblock()
      {  return m_nwaiters_to_unblock;  }

      semaphore_type  &get_sem_block_queue()
      {  return m_sem_block_queue;  }

      semaphore_type  &get_sem_block_lock()
      {  return m_sem_block_lock;  }

      mutex_type      &get_mtx_unblock_lock()
      {  return m_mtx_unblock_lock;  }

      integer_type               m_nwaiters_blocked;
      integer_type               m_nwaiters_gone;
      integer_type               m_nwaiters_to_unblock;
      winapi_semaphore_wrapper   m_sem_block_queue;
      winapi_semaphore_wrapper   m_sem_block_lock;
      winapi_mutex_wrapper       m_mtx_unblock_lock;
   };


   class named_cond_callbacks : public windows_named_sync_interface
   {
      typedef __int64 sem_count_t;
      mutable sem_count_t sem_counts [2];

      public:
      named_cond_callbacks(condition_data &cond_data)
         : m_condition_data(cond_data)
      {}

      virtual std::size_t get_data_size() const
      {  return sizeof(sem_counts);   }

      virtual const void *buffer_with_final_data_to_file()
      {
         sem_counts[0] = m_condition_data.m_sem_block_queue.value();
         sem_counts[1] = m_condition_data.m_sem_block_lock.value();
         return &sem_counts;
      }

      virtual const void *buffer_with_init_data_to_file()
      {
         sem_counts[0] = 0;
         sem_counts[1] = 1;
         return &sem_counts;
      }

      virtual void *buffer_to_store_init_data_from_file()
      {  return &sem_counts; }

      virtual bool open(create_enum_t op, const char *id_name)
      {  return this->open_impl(op, id_name);   }

      virtual bool open(create_enum_t op, const wchar_t *id_name)
      {  return this->open_impl(op, id_name);   }

      virtual void close()
      {
         m_condition_data.m_sem_block_queue.close();
         m_condition_data.m_sem_block_lock.close();
         m_condition_data.m_mtx_unblock_lock.close();
         m_condition_data.m_nwaiters_blocked = 0;
         m_condition_data.m_nwaiters_gone = 0;
         m_condition_data.m_nwaiters_to_unblock = 0;
      }

      virtual ~named_cond_callbacks()
      {}

      private:

      template<class CharT>
      bool open_impl(create_enum_t, const CharT *id_name)
      {
         typedef named_cond_callbacks_str<CharT> str_t;
         m_condition_data.m_nwaiters_blocked = 0;
         m_condition_data.m_nwaiters_gone = 0;
         m_condition_data.m_nwaiters_to_unblock = 0;

         //Now open semaphores and mutex.
         //Use local variables + swap to guarantee consistent
         //initialization and cleanup in case any opening fails
         permissions perm;
         perm.set_unrestricted();
         std::basic_string<CharT> aux_str  = str_t::ipc_cond();
         aux_str += id_name;
         std::size_t pos = aux_str.size();

         //sem_block_queue
         aux_str += str_t::bq();
         winapi_semaphore_wrapper sem_block_queue;
         bool created;
         if(!sem_block_queue.open_or_create
            (aux_str.c_str(), sem_counts[0], winapi_semaphore_wrapper::MaxCount, perm, created))
            return false;
         aux_str.erase(pos);

         //sem_block_lock
         aux_str += str_t::bl();
         winapi_semaphore_wrapper sem_block_lock;
         if(!sem_block_lock.open_or_create
            (aux_str.c_str(), sem_counts[1], winapi_semaphore_wrapper::MaxCount, perm, created))
            return false;
         aux_str.erase(pos);

         //mtx_unblock_lock
         aux_str += str_t::ul();
         winapi_mutex_wrapper mtx_unblock_lock;
         if(!mtx_unblock_lock.open_or_create(aux_str.c_str(), perm))
            return false;

         //All ok, commit data
         m_condition_data.m_sem_block_queue.swap(sem_block_queue);
         m_condition_data.m_sem_block_lock.swap(sem_block_lock);
         m_condition_data.m_mtx_unblock_lock.swap(mtx_unblock_lock);
         return true;
      }

      condition_data &m_condition_data;
   };

   windows_named_sync   m_named_sync;
   ipcdetail::condition_8a_wrapper<condition_data> m_condition_data;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_NAMED_CONDITION_ANY_HPP
