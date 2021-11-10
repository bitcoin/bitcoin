//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FILE_MAPPING_HPP
#define BOOST_INTERPROCESS_FILE_MAPPING_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if !defined(BOOST_INTERPROCESS_MAPPED_FILES)
#error "Boost.Interprocess: This platform does not support memory mapped files!"
#endif

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/simple_swap.hpp>
#include <boost/interprocess/detail/char_wchar_holder.hpp>
#include <boost/move/utility_core.hpp>

//!\file
//!Describes file_mapping and mapped region classes

namespace boost {
namespace interprocess {

//!A class that wraps a file-mapping that can be used to
//!create mapped regions from the mapped files
class file_mapping
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   BOOST_MOVABLE_BUT_NOT_COPYABLE(file_mapping)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Constructs an empty file mapping.
   //!Does not throw
   file_mapping() BOOST_NOEXCEPT;

   //!Opens a file mapping of file "filename", starting in offset
   //!"file_offset", and the mapping's size will be "size". The mapping
   //!can be opened for read-only "read_only" or read-write "read_write"
   //!modes. Throws interprocess_exception on error.
   file_mapping(const char *filename, mode_t mode);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!Opens a file mapping of file "filename", starting in offset
   //!"file_offset", and the mapping's size will be "size". The mapping
   //!can be opened for read-only "read_only" or read-write "read_write"
   //!modes. Throws interprocess_exception on error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   file_mapping(const wchar_t *filename, mode_t mode);
   #endif

   //!Moves the ownership of "moved"'s file mapping object to *this.
   //!After the call, "moved" does not represent any file mapping object.
   //!Does not throw
   file_mapping(BOOST_RV_REF(file_mapping) moved) BOOST_NOEXCEPT
      :  m_handle(file_handle_t(ipcdetail::invalid_file()))
      ,  m_mode(read_only)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s file mapping to *this.
   //!After the call, "moved" does not represent any file mapping.
   //!Does not throw
   file_mapping &operator=(BOOST_RV_REF(file_mapping) moved) BOOST_NOEXCEPT
   {
      file_mapping tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps to file_mappings.
   //!Does not throw.
   void swap(file_mapping &other) BOOST_NOEXCEPT;

   //!Returns access mode
   //!used in the constructor
   mode_t get_mode() const BOOST_NOEXCEPT;

   //!Obtains the mapping handle
   //!to be used with mapped_region
   mapping_handle_t get_mapping_handle() const BOOST_NOEXCEPT;

   //!Destroys the file mapping. All mapped regions created from this are still
   //!valid. Does not throw
   ~file_mapping();

   //!Returns the name of the file
   //!used in the constructor.
   const char *get_name() const BOOST_NOEXCEPT;

   //!Removes the file named "filename" even if it's been memory mapped.
   //!Returns true on success.
   //!The function might fail in some operating systems if the file is
   //!being used other processes and no deletion permission was shared.
   static bool remove(const char *filename);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!Removes the file named "filename" even if it's been memory mapped.
   //!Returns true on success.
   //!The function might fail in some operating systems if the file is
   //!being used other processes and no deletion permission was shared.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool remove(const wchar_t *filename);
   #endif

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   //!Closes a previously opened file mapping. Never throws.
   void priv_close();
   file_handle_t  m_handle;
   mode_t         m_mode;
   char_wchar_holder    m_filename;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

inline file_mapping::file_mapping() BOOST_NOEXCEPT
   :  m_handle(file_handle_t(ipcdetail::invalid_file()))
   ,  m_mode(read_only)
{}

inline file_mapping::~file_mapping()
{  this->priv_close(); }

inline const char *file_mapping::get_name() const BOOST_NOEXCEPT
{  return m_filename.getn(); }

inline void file_mapping::swap(file_mapping &other) BOOST_NOEXCEPT
{
   (simple_swap)(m_handle, other.m_handle);
   (simple_swap)(m_mode, other.m_mode);
   m_filename.swap(other.m_filename);
}

inline mapping_handle_t file_mapping::get_mapping_handle() const BOOST_NOEXCEPT
{  return ipcdetail::mapping_handle_from_file_handle(m_handle);  }

inline mode_t file_mapping::get_mode() const BOOST_NOEXCEPT
{  return m_mode; }

inline file_mapping::file_mapping
   (const char *filename, mode_t mode)
   :  m_filename(filename)
{
   //Check accesses
   if (mode != read_write && mode != read_only){
      error_info err = other_error;
      throw interprocess_exception(err);
   }

   //Open file
   m_handle = ipcdetail::open_existing_file(filename, mode);

   //Check for error
   if(m_handle == ipcdetail::invalid_file()){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }
   m_mode = mode;
}

inline bool file_mapping::remove(const char *filename)
{  return ipcdetail::delete_file(filename);  }

#ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES
inline file_mapping::file_mapping
   (const wchar_t *filename, mode_t mode)
   :  m_filename(filename)
{
   //Check accesses
   if (mode != read_write && mode != read_only){
      error_info err = other_error;
      throw interprocess_exception(err);
   }

   //Open file
   m_handle = ipcdetail::open_existing_file(filename, mode);

   //Check for error
   if(m_handle == ipcdetail::invalid_file()){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }
   m_mode = mode;
}

inline bool file_mapping::remove(const wchar_t *filename)
{  return ipcdetail::delete_file(filename);  }

#endif


#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline void file_mapping::priv_close()
{
   if(m_handle != ipcdetail::invalid_file()){
      ipcdetail::close_file(m_handle);
      m_handle = ipcdetail::invalid_file();
   }
}

//!A class that stores the name of a file
//!and tries to remove it in its destructor
//!Useful to remove temporary files in the presence
//!of exceptions
class remove_file_on_destroy
{
   const char * m_name;
   public:
   remove_file_on_destroy(const char *name)
      :  m_name(name)
   {}

   ~remove_file_on_destroy()
   {  ipcdetail::delete_file(m_name);  }
};

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_FILE_MAPPING_HPP
