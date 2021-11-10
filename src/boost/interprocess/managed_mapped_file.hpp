//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP
#define BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP

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
#include <boost/interprocess/detail/file_wrapper.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/permissions.hpp>
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
struct mfile_open_or_create
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
      < file_wrapper
      , final_segment_manager_alignment
      , true
      , false> type;
};

}  //namespace ipcdetail {

//!A basic mapped file named object creation class. Initializes the
//!mapped file. Inherits all basic functionality from
//!basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>
template
      <
         class CharType,
         class AllocationAlgorithm,
         template<class IndexConfig> class IndexType
      >
class basic_managed_mapped_file
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   : public ipcdetail::basic_managed_memory_impl
      < CharType, AllocationAlgorithm, IndexType
      , ipcdetail::mfile_open_or_create
         <CharType, AllocationAlgorithm, IndexType>::type::ManagedOpenOrCreateUserOffset>
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   public:
   typedef ipcdetail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType,
      ipcdetail::mfile_open_or_create<CharType, AllocationAlgorithm, IndexType>
         ::type::ManagedOpenOrCreateUserOffset>   base_t;
   typedef ipcdetail::file_wrapper device_type;

   private:

   typedef ipcdetail::create_open_func<base_t>        create_open_func_t;

   basic_managed_mapped_file *get_this_pointer()
   {  return this;   }

   private:
   typedef typename base_t::char_ptr_holder_t   char_ptr_holder_t;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_managed_mapped_file)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public: //functions

   //!Unsigned integral type enough to represent
   //!the size of a basic_managed_mapped_file.
   typedef typename BOOST_INTERPROCESS_IMPDEF(base_t::size_type) size_type;

   //!Creates mapped file and creates and places the segment manager.
   //!This can throw.
   basic_managed_mapped_file() BOOST_NOEXCEPT
   {}

   //!Creates mapped file and creates and places the segment manager.
   //!This can throw.
   basic_managed_mapped_file(create_only_t, const char *name,
                             size_type size, const void *addr = 0, const permissions &perm = permissions())
      : m_mfile(create_only, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoCreate), perm)
   {}

   //!Creates mapped file and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   basic_managed_mapped_file (open_or_create_t,
                              const char *name, size_type size,
                              const void *addr = 0, const permissions &perm = permissions())
      : m_mfile(open_or_create, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpenOrCreate), perm)
   {}

   //!Connects to a created mapped file and its segment manager.
   //!This can throw.
   basic_managed_mapped_file (open_only_t, const char* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created mapped file and its segment manager
   //!in copy_on_write mode.
   //!This can throw.
   basic_managed_mapped_file (open_copy_on_write_t, const char* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, copy_on_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created mapped file and its segment manager
   //!in read-only mode.
   //!This can throw.
   basic_managed_mapped_file (open_read_only_t, const char* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, read_only, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Creates mapped file and creates and places the segment manager.
   //!This can throw.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   basic_managed_mapped_file(create_only_t, const wchar_t *name,
                             size_type size, const void *addr = 0, const permissions &perm = permissions())
      : m_mfile(create_only, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoCreate), perm)
   {}

   //!Creates mapped file and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   basic_managed_mapped_file (open_or_create_t,
                              const wchar_t *name, size_type size,
                              const void *addr = 0, const permissions &perm = permissions())
      : m_mfile(open_or_create, name, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpenOrCreate), perm)
   {}

   //!Connects to a created mapped file and its segment manager.
   //!This can throw.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   basic_managed_mapped_file (open_only_t, const wchar_t* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created mapped file and its segment manager
   //!in copy_on_write mode.
   //!This can throw.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   basic_managed_mapped_file (open_copy_on_write_t, const wchar_t* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, copy_on_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created mapped file and its segment manager
   //!in read-only mode.
   //!This can throw.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   basic_managed_mapped_file (open_read_only_t, const wchar_t* name,
                              const void *addr = 0)
      : m_mfile(open_only, name, read_only, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_mapped_file(BOOST_RV_REF(basic_managed_mapped_file) moved) BOOST_NOEXCEPT
   {
      this->swap(moved);
   }

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_mapped_file &operator=(BOOST_RV_REF(basic_managed_mapped_file) moved) BOOST_NOEXCEPT
   {
      basic_managed_mapped_file tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~basic_managed_mapped_file()
   {}

   //!Swaps the ownership of the managed mapped memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_mapped_file &other) BOOST_NOEXCEPT
   {
      base_t::swap(other);
      m_mfile.swap(other.m_mfile);
   }

   //!Flushes cached data to file.
   //!Never throws
   bool flush()
   {  return m_mfile.flush();  }

   //!Tries to resize mapped file so that we have room for
   //!more objects.
   //!
   //!This function is not synchronized so no other thread or process should
   //!be reading or writing the file
   static bool grow(const char *filename, size_type extra_bytes)
   {
      return base_t::template grow
         <basic_managed_mapped_file>(filename, extra_bytes);
   }

   //!Tries to resize mapped file to minimized the size of the file.
   //!
   //!This function is not synchronized so no other thread or process should
   //!be reading or writing the file
   static bool shrink_to_fit(const char *filename)
   {
      return base_t::template shrink_to_fit
         <basic_managed_mapped_file>(filename);
   }

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Tries to resize mapped file so that we have room for
   //!more objects.
   //!
   //!This function is not synchronized so no other thread or process should
   //!be reading or writing the file
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool grow(const wchar_t *filename, size_type extra_bytes)
   {
      return base_t::template grow
         <basic_managed_mapped_file>(filename, extra_bytes);
   }

   //!Tries to resize mapped file to minimized the size of the file.
   //!
   //!This function is not synchronized so no other thread or process should
   //!be reading or writing the file
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool shrink_to_fit(const wchar_t *filename)
   {
      return base_t::template shrink_to_fit
         <basic_managed_mapped_file>(filename);
   }

   #endif

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, size_type> find  (char_ptr_holder_t name)
   {
      if(m_mfile.get_mapped_region().get_mode() == read_only){
         return base_t::template find_no_lock<T>(name);
      }
      else{
         return base_t::template find<T>(name);
      }
   }

   private:
   typename ipcdetail::mfile_open_or_create
      <CharType, AllocationAlgorithm, IndexType>::type m_mfile;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Typedef for a default basic_managed_mapped_file
//!of narrow characters
typedef basic_managed_mapped_file
   <char
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
managed_mapped_file;

//!Typedef for a default basic_managed_mapped_file
//!of wide characters
typedef basic_managed_mapped_file
   <wchar_t
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
wmanaged_mapped_file;

#endif   //#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP
