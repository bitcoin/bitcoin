//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_WINDOWS_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_MANAGED_WINDOWS_SHARED_MEMORY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/move/utility_core.hpp>
//These includes needed to fulfill default template parameters of
//predeclarations in interprocess_fwd.hpp
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>

namespace boost {
namespace interprocess {

namespace ipcdetail {

template
      <
         class CharType,
         class AllocationAlgorithm,
         template<class IndexConfig> class IndexType
      >
struct wshmem_open_or_create
{
   static const std::size_t segment_manager_alignment = boost::move_detail::alignment_of
         < segment_manager
               < CharType
               , AllocationAlgorithm
               , IndexType>
         >::value;
   static const std::size_t final_segment_manager_alignment
      = segment_manager_alignment > AllocationAlgorithm::Alignment
      ? segment_manager_alignment : AllocationAlgorithm::Alignment;

   typedef  ipcdetail::managed_open_or_create_impl
      < windows_shared_memory
      , final_segment_manager_alignment
      , false
      , false> type;
};


}  //namespace ipcdetail {

//!A basic managed windows shared memory creation class. Initializes the
//!shared memory segment. Inherits all basic functionality from
//!basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>
//!Unlike basic_managed_shared_memory, it has
//!no kernel persistence and the shared memory is destroyed
//!when all processes destroy all their windows_shared_memory
//!objects and mapped regions for the same shared memory
//!or the processes end/crash.
//!
//!Warning: basic_managed_windows_shared_memory and
//!basic_managed_shared_memory can't communicate between them.
template
      <
         class CharType,
         class AllocationAlgorithm,
         template<class IndexConfig> class IndexType
      >
class basic_managed_windows_shared_memory
   : public ipcdetail::basic_managed_memory_impl
      < CharType, AllocationAlgorithm, IndexType
      , ipcdetail::wshmem_open_or_create
         <CharType, AllocationAlgorithm, IndexType>::type::ManagedOpenOrCreateUserOffset>
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef ipcdetail::basic_managed_memory_impl
      < CharType, AllocationAlgorithm, IndexType
      , ipcdetail::wshmem_open_or_create
         <CharType, AllocationAlgorithm, IndexType>::type::ManagedOpenOrCreateUserOffset>   base_t;
   typedef ipcdetail::create_open_func<base_t>        create_open_func_t;

   basic_managed_windows_shared_memory *get_this_pointer()
   {  return this;   }

   private:
   typedef typename base_t::char_ptr_holder_t   char_ptr_holder_t;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_managed_windows_shared_memory)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public: //functions
   typedef typename base_t::size_type              size_type;

   //!Default constructor. Does nothing.
   //!Useful in combination with move semantics
   basic_managed_windows_shared_memory() BOOST_NOEXCEPT
   {}

   //!Creates shared memory and creates and places the segment manager.
   //!This can throw.
   basic_managed_windows_shared_memory
      (create_only_t, const char *name,
     size_type size, const void *addr = 0, const permissions &perm = permissions())
      : m_wshm(create_only, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoCreate), perm)
   {}

   //!Creates shared memory and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_or_create_t,
      const char *name, size_type size,
      const void *addr = 0,
      const permissions &perm = permissions())
      : m_wshm(open_or_create, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpenOrCreate), perm)
   {}

   //!Connects to a created shared memory and its segment manager.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_only_t, const char* name, const void *addr = 0)
      : m_wshm(open_only, name, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created shared memory and its segment manager
   //!in copy_on_write mode.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_copy_on_write_t, const char* name, const void *addr = 0)
      : m_wshm(open_only, name, copy_on_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoOpen))
   {}

   //!Connects to a created shared memory and its segment manager
   //!in read-only mode.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_read_only_t, const char* name, const void *addr = 0)
      : base_t()
      , m_wshm(open_only, name, read_only, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoOpen))
   {}

   //!Creates shared memory and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_or_create_t,
      const wchar_t *name, size_type size,
      const void *addr = 0,
      const permissions &perm = permissions())
      : m_wshm(open_or_create, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpenOrCreate), perm)
   {}

   //!Connects to a created shared memory and its segment manager.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_only_t, const wchar_t* name, const void *addr = 0)
      : m_wshm(open_only, name, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created shared memory and its segment manager
   //!in copy_on_write mode.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_copy_on_write_t, const wchar_t* name, const void *addr = 0)
      : m_wshm(open_only, name, copy_on_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoOpen))
   {}

   //!Connects to a created shared memory and its segment manager
   //!in read-only mode.
   //!This can throw.
   basic_managed_windows_shared_memory
      (open_read_only_t, const wchar_t* name, const void *addr = 0)
      : base_t()
      , m_wshm(open_only, name, read_only, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoOpen))
   {}

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_windows_shared_memory
      (BOOST_RV_REF(basic_managed_windows_shared_memory) moved) BOOST_NOEXCEPT
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_windows_shared_memory &operator=(BOOST_RV_REF(basic_managed_windows_shared_memory) moved) BOOST_NOEXCEPT
   {
      basic_managed_windows_shared_memory tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. All mapped regions are still valid after
   //!destruction. When all mapped regions and basic_managed_windows_shared_memory
   //!objects referring the shared memory are destroyed, the
   //!operating system will destroy the shared memory.
   ~basic_managed_windows_shared_memory()
   {}

   //!Swaps the ownership of the managed mapped memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_windows_shared_memory &other) BOOST_NOEXCEPT
   {
      base_t::swap(other);
      m_wshm.swap(other.m_wshm);
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, size_type> find  (char_ptr_holder_t name)
   {
      if(m_wshm.get_mapped_region().get_mode() == read_only){
         return base_t::template find_no_lock<T>(name);
      }
      else{
         return base_t::template find<T>(name);
      }
   }

   private:
   typename ipcdetail::wshmem_open_or_create
      <CharType, AllocationAlgorithm, IndexType>::type m_wshm;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Typedef for a default basic_managed_windows_shared_memory
//!of narrow characters
typedef basic_managed_windows_shared_memory
   <char
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
managed_windows_shared_memory;

//!Typedef for a default basic_managed_windows_shared_memory
//!of wide characters
typedef basic_managed_windows_shared_memory
   <wchar_t
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
wmanaged_windows_shared_memory;

#endif   //#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED


}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_WINDOWS_SHARED_MEMORY_HPP
