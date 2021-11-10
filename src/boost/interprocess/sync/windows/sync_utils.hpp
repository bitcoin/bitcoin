//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SYNC_UTILS_HPP
#define BOOST_INTERPROCESS_DETAIL_SYNC_UTILS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/win32_api.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/windows/winapi_semaphore_wrapper.hpp>
#include <boost/interprocess/sync/windows/winapi_mutex_wrapper.hpp>

//Shield against external warnings
#include <boost/interprocess/detail/config_external_begin.hpp>
   #include <boost/unordered/unordered_map.hpp>
#include <boost/interprocess/detail/config_external_end.hpp>


#include <boost/container/map.hpp>
#include <cstddef>

namespace boost {
namespace interprocess {
namespace ipcdetail {

inline bool bytes_to_str(const void *mem, const std::size_t mem_length, char *out_str, std::size_t &out_length)
{
   const std::size_t need_mem = mem_length*2+1;
   if(out_length < need_mem){
      out_length = need_mem;
      return false;
   }

   const char Characters [] =
      { '0', '1', '2', '3', '4', '5', '6', '7'
      , '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

   std::size_t char_counter = 0;
   const char *buf = (const char *)mem;
   for(std::size_t i = 0; i != mem_length; ++i){
      out_str[char_counter++] = Characters[(buf[i]&0xF0)>>4];
      out_str[char_counter++] = Characters[(buf[i]&0x0F)];
   }
   out_str[char_counter] = 0;
   return true;
}

inline bool bytes_to_str(const void *mem, const std::size_t mem_length, wchar_t *out_str, std::size_t &out_length)
{
   const std::size_t need_mem = mem_length*2+1;
   if(out_length < need_mem){
      out_length = need_mem;
      return false;
   }

   const wchar_t Characters [] =
      { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7'
      , L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

   std::size_t char_counter = 0;
   const char *buf = (const char *)mem;
   for(std::size_t i = 0; i != mem_length; ++i){
      out_str[char_counter++] = Characters[(buf[i]&0xF0)>>4];
      out_str[char_counter++] = Characters[(buf[i]&0x0F)];
   }
   out_str[char_counter] = 0;
   return true;
}

class sync_id
{
   public:
   typedef __int64 internal_type;
   sync_id(const void *map_addr)
      : map_addr_(map_addr)
   {  winapi::query_performance_counter(&rand_);  }

   explicit sync_id(internal_type val, const void *map_addr)
      : map_addr_(map_addr)
   {  rand_ = val;  }

   const internal_type &internal_pod() const
   {  return rand_;  }

   internal_type &internal_pod()
   {  return rand_;  }

   const void *map_address() const
   {  return map_addr_;  }

   friend std::size_t hash_value(const sync_id &m)
   {  return boost::hash_value(m.rand_);  }

   friend bool operator==(const sync_id &l, const sync_id &r)
   {  return l.rand_ == r.rand_ && l.map_addr_ == r.map_addr_;  }

   private:
   internal_type rand_;
   const void * const map_addr_;
};

class sync_handles
{
   public:
   enum type { MUTEX, SEMAPHORE };

   private:
   struct address_less
   {
      bool operator()(sync_id const * const l, sync_id const * const r) const
      {  return l->map_address() <  r->map_address(); }
   };

   typedef boost::unordered_map<sync_id, void*> umap_type;
   typedef boost::container::map<const sync_id*, umap_type::iterator, address_less> map_type;
   static const std::size_t LengthOfGlobal = sizeof("Global\\boost.ipc")-1;
   static const std::size_t StrSize        = LengthOfGlobal + (sizeof(sync_id)*2+1);
   typedef char NameBuf[StrSize];


   void fill_name(NameBuf &name, const sync_id &id)
   {
      const char *n = "Global\\boost.ipc";
      std::size_t i = 0;
      do{
         name[i] = n[i];
         ++i;
      } while(n[i]);
      std::size_t len = sizeof(NameBuf) - LengthOfGlobal;
      bytes_to_str(&id.internal_pod(), sizeof(id.internal_pod()), &name[LengthOfGlobal], len);
   }

   void throw_if_error(void *hnd_val)
   {
      if(!hnd_val){
         error_info err(winapi::get_last_error());
         throw interprocess_exception(err);
      }
   }

   void* open_or_create_semaphore(const sync_id &id, unsigned int initial_count)
   {
      NameBuf name;
      fill_name(name, id);
      permissions unrestricted_security;
      unrestricted_security.set_unrestricted();
      winapi_semaphore_wrapper sem_wrapper;
      bool created;
      sem_wrapper.open_or_create
         (name, (long)initial_count, winapi_semaphore_wrapper::MaxCount, unrestricted_security, created);
      throw_if_error(sem_wrapper.handle());
      return sem_wrapper.release();
   }

   void* open_or_create_mutex(const sync_id &id)
   {
      NameBuf name;
      fill_name(name, id);
      permissions unrestricted_security;
      unrestricted_security.set_unrestricted();
      winapi_mutex_wrapper mtx_wrapper;
      mtx_wrapper.open_or_create(name, unrestricted_security);
      throw_if_error(mtx_wrapper.handle());
      return mtx_wrapper.release();
   }

   public:
   void *obtain_mutex(const sync_id &id, bool *popen_created = 0)
   {
      umap_type::value_type v(id, (void*)0);
      scoped_lock<spin_mutex> lock(mtx_);
      umap_type::iterator it = umap_.insert(v).first;
      void *&hnd_val = it->second;
      if(!hnd_val){
         map_[&it->first] = it;
         hnd_val = open_or_create_mutex(id);
         if(popen_created) *popen_created = true;
      }
      else if(popen_created){
         *popen_created = false;
      }
      return hnd_val;
   }

   void *obtain_semaphore(const sync_id &id, unsigned int initial_count, bool *popen_created = 0)
   {
      umap_type::value_type v(id, (void*)0);
      scoped_lock<spin_mutex> lock(mtx_);
      umap_type::iterator it = umap_.insert(v).first;
      void *&hnd_val = it->second;
      if(!hnd_val){
         map_[&it->first] = it;
         hnd_val = open_or_create_semaphore(id, initial_count);
         if(popen_created) *popen_created = true;
      }
      else if(popen_created){
         *popen_created = false;
      }
      return hnd_val;
   }

   void destroy_handle(const sync_id &id)
   {
      scoped_lock<spin_mutex> lock(mtx_);
      umap_type::iterator it = umap_.find(id);
      umap_type::iterator itend = umap_.end();

      if(it != itend){
         winapi::close_handle(it->second);
         const map_type::key_type &k = &it->first;
         map_.erase(k);
         umap_.erase(it);
      }
   }

   void destroy_syncs_in_range(const void *addr, std::size_t size)
   {
      const sync_id low_id(addr);
      const sync_id hig_id(static_cast<const char*>(addr)+size);
      scoped_lock<spin_mutex> lock(mtx_);
      map_type::iterator itlow(map_.lower_bound(&low_id)),
                         ithig(map_.lower_bound(&hig_id));
      while(itlow != ithig){
         void * const hnd = umap_[*itlow->first];
         winapi::close_handle(hnd);
         umap_.erase(*itlow->first);
         itlow = map_.erase(itlow);
      }
   }

   private:
   spin_mutex mtx_;
   umap_type umap_;
   map_type map_;
};


}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SYNC_UTILS_HPP
