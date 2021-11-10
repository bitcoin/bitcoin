//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if !defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)
#error "This header can't be used in operating systems without XSI (System V) shared memory support"
#endif

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>

#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/xsi_key.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/detail/simple_swap.hpp>
// move
#include <boost/move/utility_core.hpp>
// other boost
#include <boost/cstdint.hpp>
// std
#include <cstddef>
// OS
#include <sys/shm.h>


//!\file
//!Describes a class representing a native xsi shared memory.

namespace boost {
namespace interprocess {

//!A class that wraps XSI (System V) shared memory.
//!Unlike shared_memory_object, xsi_shared_memory needs a valid
//!xsi_key to identify a shared memory object.
//!
//!Warning: XSI shared memory and interprocess portable
//!shared memory (boost::interprocess::shared_memory_object)
//!can't communicate between them.
class xsi_shared_memory
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable and non-assignable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(xsi_shared_memory)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Default constructor.
   //!Represents an empty xsi_shared_memory.
   xsi_shared_memory() BOOST_NOEXCEPT;

   //!Initializes *this with a shmid previously obtained (possibly from another process)
   //!This lower-level initializer allows shared memory mapping without having a key.
   xsi_shared_memory(open_only_t, int shmid)
      : m_shmid (shmid)
   {}

   //!Creates a new XSI shared memory from 'key', with size "size" and permissions "perm".
   //!If the shared memory previously exists, throws an error.
   xsi_shared_memory(create_only_t, const xsi_key &key, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, key, perm, size);  }

   //!Opens an existing shared memory with identifier 'key' or creates a new XSI shared memory from
   //!identifier 'key', with size "size" and permissions "perm".
   xsi_shared_memory(open_or_create_t, const xsi_key &key, std::size_t size, const permissions& perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, key, perm, size);  }

   //!Tries to open a XSI shared memory with identifier 'key'
   //!If the shared memory does not previously exist, it throws an error.
   xsi_shared_memory(open_only_t, const xsi_key &key)
   {  this->priv_open_or_create(ipcdetail::DoOpen, key, permissions(), 0);  }

   //!Moves the ownership of "moved"'s shared memory object to *this.
   //!After the call, "moved" does not represent any shared memory object.
   //!Does not throw
   xsi_shared_memory(BOOST_RV_REF(xsi_shared_memory) moved) BOOST_NOEXCEPT
      : m_shmid(-1)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory.
   //!Does not throw
   xsi_shared_memory &operator=(BOOST_RV_REF(xsi_shared_memory) moved) BOOST_NOEXCEPT
   {
      xsi_shared_memory tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps two xsi_shared_memorys. Does not throw
   void swap(xsi_shared_memory &other) BOOST_NOEXCEPT;

   //!Destroys *this. The shared memory won't be destroyed, just
   //!this connection to it. Use remove() to destroy the shared memory.
   ~xsi_shared_memory();

   //!Returns the shared memory ID that
   //!identifies the shared memory
   int get_shmid() const BOOST_NOEXCEPT;

   //!Returns the mapping handle.
   //!Never throws
   mapping_handle_t get_mapping_handle() const BOOST_NOEXCEPT;

   //!Erases the XSI shared memory object identified by shmid
   //!from the system.
   //!Returns false on error. Never throws
   static bool remove(int shmid);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   //!Closes a previously opened file mapping. Never throws.
   bool priv_open_or_create( ipcdetail::create_enum_t type
                           , const xsi_key &key
                           , const permissions& perm
                           , std::size_t size);
   int            m_shmid;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline xsi_shared_memory::xsi_shared_memory() BOOST_NOEXCEPT
   :  m_shmid(-1)
{}

inline xsi_shared_memory::~xsi_shared_memory()
{}

inline int xsi_shared_memory::get_shmid() const BOOST_NOEXCEPT
{  return m_shmid; }

inline void xsi_shared_memory::swap(xsi_shared_memory &other) BOOST_NOEXCEPT
{
   (simple_swap)(m_shmid, other.m_shmid);
}

inline mapping_handle_t xsi_shared_memory::get_mapping_handle() const BOOST_NOEXCEPT
{  mapping_handle_t mhnd = { m_shmid, true};   return mhnd;   }

inline bool xsi_shared_memory::priv_open_or_create
   (ipcdetail::create_enum_t type, const xsi_key &key, const permissions& permissions, std::size_t size)
{
   int perm = permissions.get_permissions();
   perm &= 0x01FF;
   int shmflg = perm;

   switch(type){
      case ipcdetail::DoOpen:
         shmflg |= 0;
      break;
      case ipcdetail::DoCreate:
         shmflg |= IPC_CREAT | IPC_EXCL;
      break;
      case ipcdetail::DoOpenOrCreate:
         shmflg |= IPC_CREAT;
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   int ret = ::shmget(key.get_key(), size, shmflg);
   int shmid = ret;
   if((type == ipcdetail::DoOpen) && (-1 != ret)){
      //Now get the size
      ::shmid_ds xsi_ds;
      ret = ::shmctl(ret, IPC_STAT, &xsi_ds);
      size = xsi_ds.shm_segsz;
   }
   if(-1 == ret){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }

   m_shmid = shmid;
   return true;
}

inline bool xsi_shared_memory::remove(int shmid)
{  return -1 != ::shmctl(shmid, IPC_RMID, 0); }

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_XSI_SHARED_MEMORY_HPP
