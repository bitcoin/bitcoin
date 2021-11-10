//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SHARED_DIR_HELPERS_HPP
#define BOOST_INTERPROCESS_DETAIL_SHARED_DIR_HELPERS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <string>

#if defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME) && defined(BOOST_INTERPROCESS_WINDOWS)
   #include <boost/interprocess/detail/windows_intermodule_singleton.hpp>
#endif

namespace boost {
namespace interprocess {
namespace ipcdetail {

template<class CharT>
struct shared_dir_constants;

template<>
struct shared_dir_constants<char>
{
   static char dir_separator()
   {  return '/';   }

   static const char *dir_interprocess()
   {  return "/boost_interprocess";   }
};

template<>
struct shared_dir_constants<wchar_t>
{
   static wchar_t dir_separator()
   {  return L'/';   }

   static const wchar_t *dir_interprocess()
   {  return L"/boost_interprocess";   }
};

#if defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME)
   #if defined(BOOST_INTERPROCESS_WINDOWS)
      //This type will initialize the stamp
      template<class CharT>
      struct windows_bootstamp
      {
         windows_bootstamp()
         {
            //Throw if bootstamp not available
            if(!winapi::get_last_bootup_time(stamp)){
               error_info err = system_error_code();
               throw interprocess_exception(err);
            }
         }
         //Use std::string. Even if this will be constructed in shared memory, all
         //modules/dlls are from this process so internal raw pointers to heap are always valid
         std::basic_string<CharT> stamp;
      };

      template <class CharT>
      inline void get_bootstamp(std::basic_string<CharT> &s, bool add = false)
      {
         const windows_bootstamp<CharT> &bootstamp = windows_intermodule_singleton<windows_bootstamp<CharT> >::get();
         if(add){
            s += bootstamp.stamp;
         }
         else{
            s = bootstamp.stamp;
         }
      }
   #elif defined(BOOST_INTERPROCESS_HAS_BSD_KERNEL_BOOTTIME)
      inline void get_bootstamp(std::string &s, bool add = false)
      {
         // FreeBSD specific: sysctl "kern.boottime"
         int request[2] = { CTL_KERN, KERN_BOOTTIME };
         struct ::timeval result;
         std::size_t result_len = sizeof result;

         if (::sysctl (request, 2, &result, &result_len, 0, 0) < 0)
            return;

         char bootstamp_str[256];

         const char Characters [] =
            { '0', '1', '2', '3', '4', '5', '6', '7'
            , '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

         std::size_t char_counter = 0;
         //32 bit values to allow 32 and 64 bit process IPC
         boost::uint32_t fields[2] = { boost::uint32_t(result.tv_sec), boost::uint32_t(result.tv_usec) };
         for(std::size_t field = 0; field != 2; ++field){
            for(std::size_t i = 0; i != sizeof(fields[0]); ++i){
               const char *ptr = (const char *)&fields[field];
               bootstamp_str[char_counter++] = Characters[(ptr[i]&0xF0)>>4];
               bootstamp_str[char_counter++] = Characters[(ptr[i]&0x0F)];
            }
         }
         bootstamp_str[char_counter] = 0;
         if(add){
            s += bootstamp_str;
         }
         else{
            s = bootstamp_str;
         }
      }
   #else
      #error "BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME defined with no known implementation"
   #endif
#endif   //#if defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME)

template <class CharT>
inline void get_shared_dir_root(std::basic_string<CharT> &dir_path)
{
   #if defined (BOOST_INTERPROCESS_WINDOWS)
      winapi::get_shared_documents_folder(dir_path);
   #else               
      dir_path = "/tmp";
   #endif

   //We always need this path, so throw on error
   if(dir_path.empty()){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }

   dir_path += shared_dir_constants<CharT>::dir_interprocess();
}

#if defined(BOOST_INTERPROCESS_SHARED_DIR_FUNC) && defined(BOOST_INTERPROCESS_SHARED_DIR_PATH)
#error "Error: Both BOOST_INTERPROCESS_SHARED_DIR_FUNC and BOOST_INTERPROCESS_SHARED_DIR_PATH defined!"
#endif

#ifdef BOOST_INTERPROCESS_SHARED_DIR_FUNC

   // When BOOST_INTERPROCESS_SHARED_DIR_FUNC is defined, users have to implement
   // get_shared_dir
   void get_shared_dir(std::string &shared_dir);

   // When BOOST_INTERPROCESS_SHARED_DIR_FUNC is defined, users have to implement
   // get_shared_dir
   void get_shared_dir(std::wstring &shared_dir);

#else

template<class CharT>
inline void get_shared_dir(std::basic_string<CharT> &shared_dir)
{
   #if defined(BOOST_INTERPROCESS_SHARED_DIR_PATH)
      shared_dir = BOOST_INTERPROCESS_SHARED_DIR_PATH;
   #else 
      get_shared_dir_root(shared_dir);
      #if defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME)
         shared_dir += shared_dir_constants<CharT>::dir_separator();
         get_bootstamp(shared_dir, true);
      #endif
   #endif
}
#endif

template<class CharT>
inline void shared_filepath(const CharT *filename, std::basic_string<CharT> &filepath)
{
   get_shared_dir(filepath);
   filepath += shared_dir_constants<CharT>::dir_separator();
   filepath += filename;
}

template<class CharT>
inline void create_shared_dir_and_clean_old(std::basic_string<CharT> &shared_dir)
{
   #if defined(BOOST_INTERPROCESS_SHARED_DIR_PATH) || defined(BOOST_INTERPROCESS_SHARED_DIR_FUNC)
      get_shared_dir(shared_dir);
   #else
      //First get the temp directory
      std::basic_string<CharT> root_shared_dir;
      get_shared_dir_root(root_shared_dir);

      //If fails, check that it's because already exists
      if(!create_directory(root_shared_dir.c_str())){
         error_info info(system_error_code());
         if(info.get_error_code() != already_exists_error){
            throw interprocess_exception(info);
         }
      }

      #if defined(BOOST_INTERPROCESS_HAS_KERNEL_BOOTTIME)
         get_shared_dir(shared_dir);

         //If fails, check that it's because already exists
         if(!create_directory(shared_dir.c_str())){
            error_info info(system_error_code());
            if(info.get_error_code() != already_exists_error){
               throw interprocess_exception(info);
            }
         }
         //Now erase all old directories created in the previous boot sessions
         std::basic_string<CharT> subdir = shared_dir;
         subdir.erase(0, root_shared_dir.size()+1);
         delete_subdirectories(root_shared_dir, subdir.c_str());
      #else
         shared_dir = root_shared_dir;
      #endif
   #endif
}

template<class CharT>
inline void create_shared_dir_cleaning_old_and_get_filepath(const CharT *filename, std::basic_string<CharT> &shared_dir)
{
   create_shared_dir_and_clean_old(shared_dir);
   shared_dir += shared_dir_constants<CharT>::dir_separator();
   shared_dir += filename;
}

template<class CharT>
inline void add_leading_slash(const CharT *name, std::basic_string<CharT> &new_name)
{
   if(name[0] != shared_dir_constants<CharT>::dir_separator()){
      new_name = shared_dir_constants<CharT>::dir_separator();
   }
   new_name += name;
}

}  //namespace boost{
}  //namespace interprocess {
}  //namespace ipcdetail {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //ifndef BOOST_INTERPROCESS_DETAIL_SHARED_DIR_HELPERS_HPP
