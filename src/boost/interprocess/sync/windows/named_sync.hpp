//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WINDOWS_NAMED_SYNC_HPP
#define BOOST_INTERPROCESS_WINDOWS_NAMED_SYNC_HPP

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
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/sync/windows/sync_utils.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <string>
#include <boost/assert.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class windows_named_sync_interface
{
   public:
   virtual std::size_t get_data_size() const = 0;
   virtual const void *buffer_with_final_data_to_file() = 0;
   virtual const void *buffer_with_init_data_to_file() = 0;
   virtual void *buffer_to_store_init_data_from_file() = 0;
   virtual bool open(create_enum_t creation_type, const char *id_name) = 0;
   virtual bool open(create_enum_t creation_type, const wchar_t *id_name) = 0;
   virtual void close() = 0;
   virtual ~windows_named_sync_interface() = 0;
};

inline windows_named_sync_interface::~windows_named_sync_interface()
{}

class windows_named_sync
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Non-copyable
   windows_named_sync(const windows_named_sync &);
   windows_named_sync &operator=(const windows_named_sync &);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   windows_named_sync();
   template <class CharT>
   void open_or_create(create_enum_t creation_type, const CharT *name, const permissions &perm, windows_named_sync_interface &sync_interface);
   void close(windows_named_sync_interface &sync_interface);

   static bool remove(const char *name);
   static bool remove(const wchar_t *name);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   void *m_file_hnd;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

inline windows_named_sync::windows_named_sync()
   : m_file_hnd(winapi::invalid_handle_value)
{}

inline void windows_named_sync::close(windows_named_sync_interface &sync_interface)
{
   const std::size_t buflen = sync_interface.get_data_size();
   winapi::interprocess_overlapped overlapped;
   if(winapi::lock_file_ex
      (m_file_hnd, winapi::lockfile_exclusive_lock, 0, 1, 0, &overlapped)){
      if(winapi::set_file_pointer(m_file_hnd, sizeof(sync_id::internal_type), 0, winapi::file_begin)){
         const void *buf = sync_interface.buffer_with_final_data_to_file();

         unsigned long written_or_read = 0;
         if(winapi::write_file(m_file_hnd, buf, buflen, &written_or_read, 0)){
            //...
         }
      }
   }
   sync_interface.close();
   //close_handle unlocks the lock
   if(m_file_hnd != winapi::invalid_handle_value){
      winapi::close_handle(m_file_hnd);
      m_file_hnd = winapi::invalid_handle_value;
   }
}

template <class CharT>
inline void windows_named_sync::open_or_create
   ( create_enum_t creation_type
   , const CharT *name
   , const permissions &perm
   , windows_named_sync_interface &sync_interface)
{
   std::basic_string<CharT> aux_str(name);
   m_file_hnd  = winapi::invalid_handle_value;
   //Use a file to emulate POSIX lifetime semantics. After this logic
   //we'll obtain the ID of the native handle to open in aux_str
   {
      create_shared_dir_cleaning_old_and_get_filepath(name, aux_str);
      //Create a file with required permissions.
      m_file_hnd = winapi::create_file
         ( aux_str.c_str()
         , winapi::generic_read | winapi::generic_write
         , creation_type == DoOpen ? winapi::open_existing :
               (creation_type == DoCreate ? winapi::create_new : winapi::open_always)
         , 0
         , (winapi::interprocess_security_attributes*)perm.get_permissions());

      //Obtain OS error in case something has failed
      error_info err;
      bool success = false;
      if(m_file_hnd != winapi::invalid_handle_value){
         //Now lock the file
         const std::size_t buflen = sync_interface.get_data_size();
         typedef __int64 unique_id_type;
         const std::size_t sizeof_file_info = sizeof(unique_id_type) + buflen;
         winapi::interprocess_overlapped overlapped;
         if(winapi::lock_file_ex
            (m_file_hnd, winapi::lockfile_exclusive_lock, 0, 1, 0, &overlapped)){
            __int64 filesize = 0;
            //Obtain the unique id to open the native semaphore.
            //If file size was created
            if(winapi::get_file_size(m_file_hnd, filesize)){
               unsigned long written_or_read = 0;
               unique_id_type unique_id_val;
               if(static_cast<std::size_t>(filesize) != sizeof_file_info){
                  winapi::set_end_of_file(m_file_hnd);
                  winapi::query_performance_counter(&unique_id_val);
                  const void *buf = sync_interface.buffer_with_init_data_to_file();
                  //Write unique ID in file. This ID will be used to calculate the semaphore name
                  if(winapi::write_file(m_file_hnd, &unique_id_val, sizeof(unique_id_val), &written_or_read, 0)  &&
                     written_or_read == sizeof(unique_id_val) &&
                     winapi::write_file(m_file_hnd, buf, buflen, &written_or_read, 0) &&
                     written_or_read == buflen ){
                     success = true;
                  }
                  winapi::get_file_size(m_file_hnd, filesize);
                  BOOST_ASSERT(std::size_t(filesize) == sizeof_file_info);
               }
               else{
                  void *buf = sync_interface.buffer_to_store_init_data_from_file();
                  if(winapi::read_file(m_file_hnd, &unique_id_val, sizeof(unique_id_val), &written_or_read, 0)  &&
                     written_or_read == sizeof(unique_id_val) &&
                     winapi::read_file(m_file_hnd, buf, buflen, &written_or_read, 0)  &&
                     written_or_read == buflen   ){
                     success = true;
                  }
               }
               if(success){
                  //Now create a global semaphore name based on the unique id
                  CharT unique_id_name[sizeof(unique_id_val)*2+1];
                  std::size_t name_suffix_length = sizeof(unique_id_name);
                  bytes_to_str(&unique_id_val, sizeof(unique_id_val), &unique_id_name[0], name_suffix_length);
                  success = sync_interface.open(creation_type, unique_id_name);
               }
            }

            //Obtain OS error in case something has failed
            if(!success)
               err = system_error_code();

            //If this fails we have no possible rollback so don't check the return
            if(!winapi::unlock_file_ex(m_file_hnd, 0, 1, 0, &overlapped)){
               err = system_error_code();
            }
         }
         else{
            //Obtain OS error in case something has failed
            err = system_error_code();
         }
      }
      else{
         err = system_error_code();
      }

      if(!success){
         if(m_file_hnd != winapi::invalid_handle_value){
            winapi::close_handle(m_file_hnd);
            m_file_hnd = winapi::invalid_handle_value;
         }
         //Throw as something went wrong
         throw interprocess_exception(err);
      }
   }
}

inline bool windows_named_sync::remove(const char *name)
{
   try{
      //Make sure a temporary path is created for shared memory
      std::string semfile;
      ipcdetail::shared_filepath(name, semfile);
      return winapi::unlink_file(semfile.c_str());
   }
   catch(...){
      return false;
   }
}

inline bool windows_named_sync::remove(const wchar_t *name)
{
   try{
      //Make sure a temporary path is created for shared memory
      std::wstring semfile;
      ipcdetail::shared_filepath(name, semfile);
      return winapi::unlink_file(semfile.c_str());
   }
   catch(...){
      return false;
   }
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_NAMED_SYNC_HPP
