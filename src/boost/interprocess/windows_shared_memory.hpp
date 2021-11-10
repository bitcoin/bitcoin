//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WINDOWS_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_WINDOWS_SHARED_MEMORY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/detail/simple_swap.hpp>
#include <boost/interprocess/detail/char_wchar_holder.hpp>

#if !defined(BOOST_INTERPROCESS_WINDOWS)
#error "This header can only be used in Windows operating systems"
#endif

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/win32_api.hpp>
#include <cstddef>
#include <boost/cstdint.hpp>


//!\file
//!Describes a class representing a native windows shared memory.

namespace boost {
namespace interprocess {

//!A class that wraps the native Windows shared memory
//!that is implemented as a file mapping of the paging file.
//!Unlike shared_memory_object, windows_shared_memory has
//!no kernel persistence and the shared memory is destroyed
//!when all processes destroy all their windows_shared_memory
//!objects and mapped regions for the same shared memory
//!or the processes end/crash.
//!
//!Warning: Windows native shared memory and interprocess portable
//!shared memory (boost::interprocess::shared_memory_object)
//!can't communicate between them.
class windows_shared_memory
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable and non-assignable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(windows_shared_memory)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Default constructor.
   //!Represents an empty windows_shared_memory.
   windows_shared_memory() BOOST_NOEXCEPT;

   //!Creates a new native shared memory with name "name" and at least size "size",
   //!with the access mode "mode".
   //!If the file previously exists, throws an error.
   windows_shared_memory(create_only_t, const char *name, mode_t mode, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, size, perm);  }

   //!Tries to create a shared memory object with name "name" and at least size "size", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   windows_shared_memory(open_or_create_t, const char *name, mode_t mode, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, name, mode, size, perm);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode".
   //!If the file does not previously exist, it throws an error.
   windows_shared_memory(open_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, 0, permissions());  }

   //!Creates a new native shared memory with name "name" and at least size "size",
   //!with the access mode "mode".
   //!If the file previously exists, throws an error.
   windows_shared_memory(create_only_t, const wchar_t *name, mode_t mode, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, size, perm);  }

   //!Tries to create a shared memory object with name "name" and at least size "size", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   windows_shared_memory(open_or_create_t, const wchar_t *name, mode_t mode, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, name, mode, size, perm);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode".
   //!If the file does not previously exist, it throws an error.
   windows_shared_memory(open_only_t, const wchar_t *name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, 0, permissions());  }

   //!Moves the ownership of "moved"'s shared memory object to *this.
   //!After the call, "moved" does not represent any shared memory object.
   //!Does not throw
   windows_shared_memory(BOOST_RV_REF(windows_shared_memory) moved) BOOST_NOEXCEPT
      : m_handle(0)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory.
   //!Does not throw
   windows_shared_memory &operator=(BOOST_RV_REF(windows_shared_memory) moved) BOOST_NOEXCEPT
   {
      windows_shared_memory tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps to shared_memory_objects. Does not throw
   void swap(windows_shared_memory &other) BOOST_NOEXCEPT;

   //!Destroys *this. All mapped regions are still valid after
   //!destruction. When all mapped regions and windows_shared_memory
   //!objects referring the shared memory are destroyed, the
   //!operating system will destroy the shared memory.
   ~windows_shared_memory();

   //!Returns the name of the shared memory.
   const char *get_name() const BOOST_NOEXCEPT;

   //!Returns access mode
   mode_t get_mode() const BOOST_NOEXCEPT;

   //!Returns the mapping handle. Never throws
   mapping_handle_t get_mapping_handle() const BOOST_NOEXCEPT;

   //!Returns the size of the windows shared memory. It will be a 4K rounded
   //!size of the "size" passed in the constructor.
   offset_t get_size() const BOOST_NOEXCEPT;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   //!Closes a previously opened file mapping. Never throws.
   void priv_close();

   //!Closes a previously opened file mapping. Never throws.
   template <class CharT>
   bool priv_open_or_create(ipcdetail::create_enum_t type, const CharT *filename, mode_t mode, std::size_t size, const permissions& perm = permissions());

   void *         m_handle;
   mode_t         m_mode;
   char_wchar_holder m_name;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline windows_shared_memory::windows_shared_memory() BOOST_NOEXCEPT
   :  m_handle(0)
{}

inline windows_shared_memory::~windows_shared_memory()
{  this->priv_close(); }

inline const char *windows_shared_memory::get_name() const BOOST_NOEXCEPT
{  return m_name.getn(); }

inline void windows_shared_memory::swap(windows_shared_memory &other) BOOST_NOEXCEPT
{
   (simple_swap)(m_handle,  other.m_handle);
   (simple_swap)(m_mode,    other.m_mode);
   m_name.swap(other.m_name);
}

inline mapping_handle_t windows_shared_memory::get_mapping_handle() const BOOST_NOEXCEPT
{  mapping_handle_t mhnd = { m_handle, true};   return mhnd;   }

inline mode_t windows_shared_memory::get_mode() const BOOST_NOEXCEPT
{  return m_mode; }

inline offset_t windows_shared_memory::get_size() const BOOST_NOEXCEPT
{
   offset_t size; //This shall never fail
   return (m_handle && winapi::get_file_mapping_size(m_handle, size)) ? size : 0;
}

template <class CharT>
inline bool windows_shared_memory::priv_open_or_create
   (ipcdetail::create_enum_t type, const CharT *filename, mode_t mode, std::size_t size, const permissions& perm)
{
   unsigned long protection = 0;
   unsigned long map_access = 0;

   switch(mode)
   {
      //"protection" is for "create_file_mapping"
      //"map_access" is for "open_file_mapping"
      //Add section query (strange that read or access does not grant it...)
      //to obtain the size of the mapping. copy_on_write is equal to section_query.
      case read_only:
         protection   |= winapi::page_readonly;
         map_access   |= winapi::file_map_read | winapi::section_query;
      break;
      case read_write:
         protection   |= winapi::page_readwrite;
         map_access   |= winapi::file_map_write | winapi::section_query;
      break;
      case copy_on_write:
         protection   |= winapi::page_writecopy;
         map_access   |= winapi::file_map_copy;
      break;
      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   switch(type){
      case ipcdetail::DoOpen:
         m_handle = winapi::open_file_mapping(map_access, filename);
      break;
      case ipcdetail::DoCreate:
      case ipcdetail::DoOpenOrCreate:
      {
         m_handle = winapi::create_file_mapping
            ( winapi::invalid_handle_value, protection, size, filename
            , (winapi::interprocess_security_attributes*)perm.get_permissions());
      }
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   if(!m_handle || (type == ipcdetail::DoCreate && winapi::get_last_error() == winapi::error_already_exists)){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   m_mode = mode;
   return true;
}

inline void windows_shared_memory::priv_close()
{
   if(m_handle){
      winapi::close_handle(m_handle);
      m_handle = 0;
   }
}

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_SHARED_MEMORY_HPP
