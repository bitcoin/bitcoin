//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_BASIC_GLOBAL_MEMORY_HPP
#define BOOST_INTERPROCESS_BASIC_GLOBAL_MEMORY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>
#include <boost/interprocess/sync/spin/recursive_mutex.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/permissions.hpp>

namespace boost{
namespace interprocess{
namespace ipcdetail{

struct intermodule_singleton_mutex_family
{
   typedef boost::interprocess::ipcdetail::spin_mutex              mutex_type;
   typedef boost::interprocess::ipcdetail::spin_recursive_mutex    recursive_mutex_type;
};

struct intermodule_types
{
   //We must use offset_ptr since a loaded DLL can map the singleton holder shared memory
   //at a different address than other DLLs or the main executable
   typedef rbtree_best_fit<intermodule_singleton_mutex_family, offset_ptr<void> > mem_algo;
   template<class Device, bool FileBased>
   struct open_or_create
   {
      typedef managed_open_or_create_impl
            <Device, mem_algo::Alignment, FileBased, false> type;
   };
};

//we must implement our own managed shared memory to avoid circular dependencies
template<class Device, bool FileBased>
class basic_managed_global_memory
   : public basic_managed_memory_impl
      < char
      , intermodule_types::mem_algo
      , iset_index
      , intermodule_types::open_or_create<Device, FileBased>::type::ManagedOpenOrCreateUserOffset
      >
   , private intermodule_types::open_or_create<Device, FileBased>::type
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   typedef typename intermodule_types::template open_or_create<Device, FileBased>::type base2_t;

   typedef basic_managed_memory_impl
      < char
      , intermodule_types::mem_algo
      , iset_index
      , base2_t::ManagedOpenOrCreateUserOffset
      > base_t;

   typedef create_open_func<base_t>        create_open_func_t;

   basic_managed_global_memory *get_this_pointer()
   {  return this;   }

   public:
   typedef typename base_t::size_type              size_type;

   private:
   typedef typename base_t::char_ptr_holder_t   char_ptr_holder_t;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_managed_global_memory)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public: //functions

   basic_managed_global_memory (open_or_create_t open_or_create,
                              const char *name, size_type size,
                              const void *addr = 0, const permissions& perm = permissions())
      : base_t()
      , base2_t(open_or_create, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                DoOpenOrCreate), perm)
   {}

   basic_managed_global_memory (open_only_t open_only, const char* name,
                                const void *addr = 0)
      : base_t()
      , base2_t(open_only, name, read_write, addr,
                create_open_func_t(get_this_pointer(),
                DoOpen))
   {}
};


}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_BASIC_GLOBAL_MEMORY_HPP
