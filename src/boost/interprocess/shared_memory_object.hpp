//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP
#define BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP

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
#include <boost/interprocess/exceptions.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/detail/char_wchar_holder.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <cstddef>

#if defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)
#  include <string>
#  include <fcntl.h>        //posix_fallocate, O_CREAT, O_*...
#  include <sys/mman.h>     //shm_xxx
#  include <unistd.h>       //ftruncate, close
#  include <sys/stat.h>     //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#  if defined(BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
#     if defined(__FreeBSD__)
#        include <sys/sysctl.h>
#     endif
#  endif
#else
//
#endif

//!\file
//!Describes a shared memory object management class.

namespace boost {
namespace interprocess {

//!A class that wraps a shared memory mapping that can be used to
//!create mapped regions from the mapped files
class shared_memory_object
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable and non-assignable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(shared_memory_object)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Default constructor. Represents an empty shared_memory_object.
   shared_memory_object() BOOST_NOEXCEPT;

   //!Creates a shared memory object with name "name" and mode "mode", with the access mode "mode"
   //!If the file previously exists, throws an error.*/
   shared_memory_object(create_only_t, const char *name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, perm);  }

   //!Tries to create a shared memory object with name "name" and mode "mode", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   shared_memory_object(open_or_create_t, const char *name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, name, mode, perm);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode".
   //!If the file does not previously exist, it throws an error.
   shared_memory_object(open_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, permissions());  }

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Creates a shared memory object with name "name" and mode "mode", with the access mode "mode"
   //!If the file previously exists, throws an error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   shared_memory_object(create_only_t, const wchar_t*name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, perm);  }

   //!Tries to create a shared memory object with name "name" and mode "mode", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   shared_memory_object(open_or_create_t, const wchar_t*name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, name, mode, perm);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode".
   //!If the file does not previously exist, it throws an error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   shared_memory_object(open_only_t, const wchar_t*name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, permissions());  }

   #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Moves the ownership of "moved"'s shared memory object to *this.
   //!After the call, "moved" does not represent any shared memory object.
   //!Does not throw
   shared_memory_object(BOOST_RV_REF(shared_memory_object) moved) BOOST_NOEXCEPT
      :  m_handle(file_handle_t(ipcdetail::invalid_file()))
      ,  m_mode(read_only)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory.
   //!Does not throw
   shared_memory_object &operator=(BOOST_RV_REF(shared_memory_object) moved) BOOST_NOEXCEPT
   {
      shared_memory_object tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps the shared_memory_objects. Does not throw
   void swap(shared_memory_object &moved) BOOST_NOEXCEPT;

   //!Erases a shared memory object from the system.
   //!Returns false on error. Never throws
   static bool remove(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Erases a shared memory object from the system.
   //!Returns false on error. Never throws
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool remove(const wchar_t *name);

   #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Sets the size of the shared memory mapping
   void truncate(offset_t length);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. All mapped regions are still
   //!valid after destruction. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~shared_memory_object();

   //!Returns the name of the shared memory object.
   const char *get_name() const BOOST_NOEXCEPT;

   //!Returns true if the size of the shared memory object
   //!can be obtained and writes the size in the passed reference
   bool get_size(offset_t &size) const BOOST_NOEXCEPT;

   //!Returns access mode
   mode_t get_mode() const BOOST_NOEXCEPT;

   //!Returns mapping handle. Never throws.
   mapping_handle_t get_mapping_handle() const BOOST_NOEXCEPT;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   //!Closes a previously opened file mapping. Never throws.
   void priv_close();

   //!Opens or creates a shared memory object.
   template<class CharT>
   bool priv_open_or_create(ipcdetail::create_enum_t type, const CharT *filename, mode_t mode, const permissions &perm);

   file_handle_t     m_handle;
   mode_t            m_mode;
   char_wchar_holder m_filename;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline shared_memory_object::shared_memory_object() BOOST_NOEXCEPT
   :  m_handle(file_handle_t(ipcdetail::invalid_file()))
   ,  m_mode(read_only)
{}

inline shared_memory_object::~shared_memory_object()
{  this->priv_close(); }


inline const char *shared_memory_object::get_name() const BOOST_NOEXCEPT
{  return m_filename.getn(); }

inline bool shared_memory_object::get_size(offset_t &size) const BOOST_NOEXCEPT
{  return ipcdetail::get_file_size((file_handle_t)m_handle, size);  }

inline void shared_memory_object::swap(shared_memory_object &other) BOOST_NOEXCEPT
{
   boost::adl_move_swap(m_handle, other.m_handle);
   boost::adl_move_swap(m_mode,   other.m_mode);
   m_filename.swap(other.m_filename);
}

inline mapping_handle_t shared_memory_object::get_mapping_handle() const BOOST_NOEXCEPT
{
   return ipcdetail::mapping_handle_from_file_handle(m_handle);
}

inline mode_t shared_memory_object::get_mode() const BOOST_NOEXCEPT
{  return m_mode; }

#if !defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)

template<class CharT>
inline bool shared_memory_object::priv_open_or_create
   (ipcdetail::create_enum_t type, const CharT *filename, mode_t mode, const permissions &perm)
{
   m_filename = filename;
   std::basic_string<CharT> shmfile;
   ipcdetail::create_shared_dir_cleaning_old_and_get_filepath(filename, shmfile);

   //Set accesses
   if (mode != read_write && mode != read_only){
      error_info err = other_error;
      throw interprocess_exception(err);
   }

   switch(type){
      case ipcdetail::DoOpen:
         m_handle = ipcdetail::open_existing_file(shmfile.c_str(), mode, true);
      break;
      case ipcdetail::DoCreate:
         m_handle = ipcdetail::create_new_file(shmfile.c_str(), mode, perm, true);
      break;
      case ipcdetail::DoOpenOrCreate:
         m_handle = ipcdetail::create_or_open_file(shmfile.c_str(), mode, perm, true);
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   //Check for error
   if(m_handle == ipcdetail::invalid_file()){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   m_mode = mode;
   return true;
}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES)

inline bool shared_memory_object::remove(const wchar_t *filename)
{
   try{
      //Make sure a temporary path is created for shared memory
      std::wstring shmfile;
      ipcdetail::shared_filepath(filename, shmfile);
      return ipcdetail::delete_file(shmfile.c_str());
   }
   catch(...){
      return false;
   }
}

#endif

inline bool shared_memory_object::remove(const char *filename)
{
   try{
      //Make sure a temporary path is created for shared memory
      std::string shmfile;
      ipcdetail::shared_filepath(filename, shmfile);
      return ipcdetail::delete_file(shmfile.c_str());
   }
   catch(...){
      return false;
   }
}

inline void shared_memory_object::truncate(offset_t length)
{
   if(!ipcdetail::truncate_file(m_handle, length)){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline void shared_memory_object::priv_close()
{
   if(m_handle != ipcdetail::invalid_file()){
      ipcdetail::close_file(m_handle);
      m_handle = ipcdetail::invalid_file();
   }
}

#else //!defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)

namespace shared_memory_object_detail {

#ifdef BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY

#if defined(__FreeBSD__)

inline bool use_filesystem_based_posix()
{
   int jailed = 0;
   std::size_t len = sizeof(jailed);
   ::sysctlbyname("security.jail.jailed", &jailed, &len, NULL, 0);
   return jailed != 0;
}

#else
#error "Not supported platform for BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY"
#endif

#endif

}  //shared_memory_object_detail

template<class CharT>
inline bool shared_memory_object::priv_open_or_create
   (ipcdetail::create_enum_t type,
    const CharT *filename,
    mode_t mode, const permissions &perm)
{
   #if defined(BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
   const bool add_leading_slash = false;
   #elif defined(BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
   const bool add_leading_slash = !shared_memory_object_detail::use_filesystem_based_posix();
   #else
   const bool add_leading_slash = true;
   #endif
   std::basic_string<CharT> fname;
   if(add_leading_slash){
      ipcdetail::add_leading_slash(filename, fname);
   }
   else{
      ipcdetail::create_shared_dir_cleaning_old_and_get_filepath(filename, fname);
   }

   //Create new mapping
   int oflag = 0;
   if(mode == read_only){
      oflag |= O_RDONLY;
   }
   else if(mode == read_write){
      oflag |= O_RDWR;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }
   int unix_perm = perm.get_permissions();

   switch(type){
      case ipcdetail::DoOpen:
      {
         //No oflag addition
         m_handle = shm_open(fname.c_str(), oflag, unix_perm);
      }
      break;
      case ipcdetail::DoCreate:
      {
         oflag |= (O_CREAT | O_EXCL);
         m_handle = shm_open(fname.c_str(), oflag, unix_perm);
         if(m_handle >= 0){
            ::fchmod(m_handle, unix_perm);
         }
      }
      break;
      case ipcdetail::DoOpenOrCreate:
      {
         //We need a create/open loop to change permissions correctly using fchmod, since
         //with "O_CREAT" only we don't know if we've created or opened the shm.
         while(true){
            //Try to create shared memory
            m_handle = shm_open(fname.c_str(), oflag | (O_CREAT | O_EXCL), unix_perm);
            //If successful change real permissions
            if(m_handle >= 0){
               ::fchmod(m_handle, unix_perm);
            }
            //If already exists, try to open
            else if(errno == EEXIST){
               m_handle = shm_open(fname.c_str(), oflag, unix_perm);
               //If open fails and errno tells the file does not exist
               //(shm was removed between creation and opening tries), just retry
               if(m_handle < 0 && errno == ENOENT){
                  continue;
               }
            }
            //Exit retries
            break;
         }
      }
      break;
      default:
      {
         error_info err = other_error;
         throw interprocess_exception(err);
      }
   }

   //Check for error
   if(m_handle < 0){
      error_info err = errno;
      this->priv_close();
      throw interprocess_exception(err);
   }

   m_filename = filename;
   m_mode = mode;
   return true;
}

inline bool shared_memory_object::remove(const char *filename)
{
   try{
      std::string filepath;
      #if defined(BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
      const bool add_leading_slash = false;
      #elif defined(BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
      const bool add_leading_slash = !shared_memory_object_detail::use_filesystem_based_posix();
      #else
      const bool add_leading_slash = true;
      #endif
      if(add_leading_slash){
         ipcdetail::add_leading_slash(filename, filepath);
      }
      else{
         ipcdetail::shared_filepath(filename, filepath);
      }
      return 0 == shm_unlink(filepath.c_str());
   }
   catch(...){
      return false;
   }
}

inline void shared_memory_object::truncate(offset_t length)
{
   #ifdef BOOST_INTERPROCESS_POSIX_FALLOCATE
   int ret = posix_fallocate(m_handle, 0, length);

   if (ret && ret != EOPNOTSUPP){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   //ftruncate fallback
   #endif //BOOST_INTERPROCESS_POSIX_FALLOCATE
   if(0 != ftruncate(m_handle, length)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void shared_memory_object::priv_close()
{
   if(m_handle != -1){
      ::close(m_handle);
      m_handle = -1;
   }
}

#endif

//!A class that stores the name of a shared memory
//!and calls shared_memory_object::remove(name) in its destructor
//!Useful to remove temporary shared memory objects in the presence
//!of exceptions
class remove_shared_memory_on_destroy
{
   const char * m_name;
   public:
   remove_shared_memory_on_destroy(const char *name)
      :  m_name(name)
   {}

   ~remove_shared_memory_on_destroy()
   {  shared_memory_object::remove(m_name);  }
};

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP
