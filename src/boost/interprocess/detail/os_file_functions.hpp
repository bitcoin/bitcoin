//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP
#define BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/permissions.hpp>

#include <climits>
#include <limits>
#include <string>
#include <boost/move/detail/type_traits.hpp> //make_unsigned

#if defined (BOOST_INTERPROCESS_WINDOWS)
#  include <boost/interprocess/detail/win32_api.hpp>
#  include <wchar.h> //wcsxxx()
#else
#  ifdef BOOST_HAS_UNISTD_H
#     include <fcntl.h>
#     include <unistd.h>
#     include <sys/types.h>
#     include <sys/stat.h>
#     include <dirent.h>
#     include <cerrno>
#     include <cstdio>
#     if 0
#        include <sys/file.h>
#     endif
#  else
#    error Unknown platform
#  endif
#endif

#include <cstring>
#include <cstdlib>

namespace boost {
namespace interprocess {

#if defined (BOOST_INTERPROCESS_WINDOWS)

typedef void *                   file_handle_t;
typedef __int64  offset_t;
typedef struct mapping_handle_impl_t{
   void *   handle;
   bool     is_shm;
}  mapping_handle_t;

typedef enum { read_only      = winapi::generic_read
             , read_write     = winapi::generic_read | winapi::generic_write
             , copy_on_write
             , read_private
             , invalid_mode   = 0xffff
             } mode_t;

typedef enum { file_begin     = winapi::file_begin
             , file_end       = winapi::file_end
             , file_current   = winapi::file_current
             } file_pos_t;

typedef unsigned long      map_options_t;
static const map_options_t default_map_options = map_options_t(-1);

namespace ipcdetail{

inline mapping_handle_t mapping_handle_from_file_handle(file_handle_t hnd)
{
   mapping_handle_t ret;
   ret.handle = hnd;
   ret.is_shm = false;
   return ret;
}

inline mapping_handle_t mapping_handle_from_shm_handle(file_handle_t hnd)
{
   mapping_handle_t ret;
   ret.handle = hnd;
   ret.is_shm = true;
   return ret;
}

inline file_handle_t file_handle_from_mapping_handle(mapping_handle_t hnd)
{  return hnd.handle; }

template<class CharT>
inline bool create_directory(const CharT *path)
{  return winapi::create_directory(path); }

inline bool remove_directory(const char *path)
{  return winapi::remove_directory(path); }

inline bool get_temporary_path(char *buffer, std::size_t buf_len, std::size_t &required_len)
{
   required_len = 0;
   //std::size_t is always bigger or equal than unsigned long in Windows systems
   //In case std::size_t is bigger than unsigned long
   unsigned long buf = buf_len;
   if(buf_len != buf){   //maybe overflowed
      return false;
   }
   required_len = winapi::get_temp_path(buf_len, buffer);
   const bool ret = !(buf_len < required_len);
   if(ret && buffer[required_len-1] == '\\'){
      buffer[required_len-1] = '\0';
   }
   return ret;
}

inline bool get_temporary_path(wchar_t *buffer, std::size_t buf_len, std::size_t &required_len)
{
   required_len = 0;
   //std::size_t is always bigger or equal than unsigned long in Windows systems
   //In case std::size_t is bigger than unsigned long
   unsigned long buf = buf_len;
   if(buf_len != buf){   //maybe overflowed
      return false;
   }
   required_len = winapi::get_temp_path(buf_len, buffer);
   const bool ret = !(buf_len < required_len);
   if(ret && buffer[required_len-1] == L'\\'){
      buffer[required_len-1] = L'\0';
   }
   return ret;
}

template<class CharT>
inline file_handle_t create_new_file
   (const CharT *name, mode_t mode, const permissions & perm = permissions(), bool temporary = false)
{
   unsigned long attr = temporary ? winapi::file_attribute_temporary : 0;
   return winapi::create_file
      ( name, (unsigned int)mode, winapi::create_new, attr
      , (winapi::interprocess_security_attributes*)perm.get_permissions());
}

template <class CharT>
inline file_handle_t create_or_open_file
   (const CharT *name, mode_t mode, const permissions & perm = permissions(), bool temporary = false)
{
   unsigned long attr = temporary ? winapi::file_attribute_temporary : 0;
   return winapi::create_file
      ( name, (unsigned int)mode, winapi::open_always, attr
      , (winapi::interprocess_security_attributes*)perm.get_permissions());
}

template<class CharT>
inline file_handle_t open_existing_file
   (const CharT *name, mode_t mode, bool temporary = false)
{
   unsigned long attr = temporary ? winapi::file_attribute_temporary : 0;
   return winapi::create_file
      (name, (unsigned int)mode, winapi::open_existing, attr, 0);
}

inline bool delete_file(const char *name)
{  return winapi::unlink_file(name);   }

inline bool delete_file(const wchar_t *name)
{  return winapi::unlink_file(name);   }

inline bool truncate_file (file_handle_t hnd, std::size_t size)
{
   offset_t filesize;
   if(!winapi::get_file_size(hnd, filesize))
      return false;

   typedef ::boost::move_detail::make_unsigned<offset_t>::type uoffset_t;
   const uoffset_t max_filesize = uoffset_t((std::numeric_limits<offset_t>::max)());
   const uoffset_t uoff_size = uoffset_t(size);
   //Avoid unused variable warnings in 32 bit systems
   if(uoff_size > max_filesize){
      winapi::set_last_error(winapi::error_file_too_large);
      return false;
   }

   if(offset_t(size) > filesize){
      if(!winapi::set_file_pointer(hnd, filesize, 0, winapi::file_begin)){
         return false;
      }
      //We will write zeros in the end of the file
      //since set_end_of_file does not guarantee this
      for(std::size_t remaining = size - filesize, write_size = 0
         ;remaining > 0
         ;remaining -= write_size){
         const std::size_t DataSize = 512;
         static char data [DataSize];
         write_size = DataSize < remaining ? DataSize : remaining;
         unsigned long written;
         winapi::write_file(hnd, data, (unsigned long)write_size, &written, 0);
         if(written != write_size){
            return false;
         }
      }
   }
   else{
      if(!winapi::set_file_pointer(hnd, size, 0, winapi::file_begin)){
         return false;
      }
      if(!winapi::set_end_of_file(hnd)){
         return false;
      }
   }
   return true;
}

inline bool get_file_size(file_handle_t hnd, offset_t &size)
{  return winapi::get_file_size(hnd, size);  }

inline bool set_file_pointer(file_handle_t hnd, offset_t off, file_pos_t pos)
{  return winapi::set_file_pointer(hnd, off, 0, (unsigned long) pos); }

inline bool get_file_pointer(file_handle_t hnd, offset_t &off)
{  return winapi::set_file_pointer(hnd, 0, &off, winapi::file_current); }

inline bool write_file(file_handle_t hnd, const void *data, std::size_t numdata)
{
   unsigned long written;
   return 0 != winapi::write_file(hnd, data, (unsigned long)numdata, &written, 0);
}

inline file_handle_t invalid_file()
{  return winapi::invalid_handle_value;  }

inline bool close_file(file_handle_t hnd)
{  return 0 != winapi::close_handle(hnd);   }

inline bool acquire_file_lock(file_handle_t hnd)
{
   static winapi::interprocess_overlapped overlapped;
   const unsigned long len = ((unsigned long)-1);
//   winapi::interprocess_overlapped overlapped;
//   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::lock_file_ex
      (hnd, winapi::lockfile_exclusive_lock, 0, len, len, &overlapped);
}

inline bool try_acquire_file_lock(file_handle_t hnd, bool &acquired)
{
   const unsigned long len = ((unsigned long)-1);
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   if(!winapi::lock_file_ex
      (hnd, winapi::lockfile_exclusive_lock | winapi::lockfile_fail_immediately,
       0, len, len, &overlapped)){
      return winapi::get_last_error() == winapi::error_lock_violation ?
               acquired = false, true : false;

   }
   acquired = true;
   return true;
}

inline bool release_file_lock(file_handle_t hnd)
{
   const unsigned long len = ((unsigned long)-1);
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::unlock_file_ex(hnd, 0, len, len, &overlapped);
}

inline bool acquire_file_lock_sharable(file_handle_t hnd)
{
   const unsigned long len = ((unsigned long)-1);
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::lock_file_ex(hnd, 0, 0, len, len, &overlapped);
}

inline bool try_acquire_file_lock_sharable(file_handle_t hnd, bool &acquired)
{
   const unsigned long len = ((unsigned long)-1);
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   if(!winapi::lock_file_ex
      (hnd, winapi::lockfile_fail_immediately, 0, len, len, &overlapped)){
      return winapi::get_last_error() == winapi::error_lock_violation ?
               acquired = false, true : false;
   }
   
   acquired = true;
   return true;
}

inline bool release_file_lock_sharable(file_handle_t hnd)
{  return release_file_lock(hnd);   }

template<class CharT>
struct os_file_traits;

template<>
struct os_file_traits<char>
{
   static const char *any_file()
   {  return "\\*.*";   }

   static const char *backslash()
   {  return "\\";   }

   static char dot()
   {  return '.';   }

   typedef winapi::win32_find_data_a win32_find_data_t;

   static int cmp(const char *a, const char *b)
   {  return std::strcmp(a, b); }
};

template<>
struct os_file_traits<wchar_t>
{
   static const wchar_t *any_file()
   {  return L"\\*.*";   }

   static const wchar_t *backslash()
   {  return L"\\";   }

   static wchar_t dot()
   {  return L'.';   }

   typedef winapi::win32_find_data_w win32_find_data_t;

   static int cmp(const wchar_t *a, const wchar_t *b)
   {  return std::wcscmp(a, b); }
};

template<class CharT>
inline bool delete_subdirectories_recursive
   (const std::basic_string<CharT> &refcstrRootDirectory, const CharT *dont_delete_this, unsigned int count)
{
   bool               bSubdirectory = false;       // Flag, indicating whether
                                                   // subdirectories have been found
   void *             hFile;                       // Handle to directory
   std::basic_string<CharT>        strFilePath;                 // Filepath
   std::basic_string<CharT>        strPattern;                  // Pattern
   typedef os_file_traits<CharT> traits_t;
   typename traits_t::win32_find_data_t  FileInformation;     // File information

   //Find all files and directories
   strPattern = refcstrRootDirectory + traits_t::any_file();
   hFile = winapi::find_first_file(strPattern.c_str(), &FileInformation);
   if(hFile != winapi::invalid_handle_value){
      do{
         //If it's not "." or ".." or the pointed root_level dont_delete_this erase it
         if(FileInformation.cFileName[0] != traits_t::dot() &&
            !(dont_delete_this && count == 0 && traits_t::cmp(dont_delete_this, FileInformation.cFileName) == 0)){
            strFilePath.erase();
            strFilePath = refcstrRootDirectory + traits_t::backslash() + FileInformation.cFileName;

            //If it's a directory, go recursive
            if(FileInformation.dwFileAttributes & winapi::file_attribute_directory){
               // Delete subdirectory
               if(!delete_subdirectories_recursive(strFilePath, dont_delete_this, count+1)){
                  winapi::find_close(hFile);
                  return false;
               }
            }
            //If it's a file, just delete it
            else{
               // Set file attributes
               //if(::SetFileAttributes(strFilePath.c_str(), winapi::file_attribute_normal) == 0)
               //return winapi::get_last_error();
               // Delete file
               winapi::unlink_file(strFilePath.c_str());
            }
         }
      //Go to the next file
      } while(winapi::find_next_file(hFile, &FileInformation) == 1);

      // Close handle
      winapi::find_close(hFile);

      //See if the loop has ended with an error or just because we've traversed all the files
      if(winapi::get_last_error() != winapi::error_no_more_files){
         return false;
      }
      else
      {
         //Erase empty subdirectories or original refcstrRootDirectory
         if(!bSubdirectory && count)
         {
            // Set directory attributes
            //if(::SetFileAttributes(refcstrRootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == 0)
               //return ::GetLastError();
            // Delete directory
            if(winapi::remove_directory(refcstrRootDirectory.c_str()) == 0)
               return false;
         }
      }
   }
   return true;
}

//This function erases all the subdirectories of a directory except the one pointed by "dont_delete_this"
template <class CharT>
inline bool delete_subdirectories(const std::basic_string<CharT> &refcstrRootDirectory, const CharT *dont_delete_this)
{
   return delete_subdirectories_recursive(refcstrRootDirectory, dont_delete_this, 0u);
}


template<class Function>
inline bool for_each_file_in_dir(const char *dir, Function f)
{
   void *             hFile;                       // Handle to directory
   winapi::win32_find_data_a  FileInformation;     // File information

   //Get base directory
   std::string str(dir);
   const std::size_t base_root_dir_len = str.size();

   //Find all files and directories
   str  +=  "\\*.*";
   hFile = winapi::find_first_file(str.c_str(), &FileInformation);
   if(hFile != winapi::invalid_handle_value){
      do{   //Now loop every file
         str.erase(base_root_dir_len);
         //If it's not "." or ".." skip it
         if(FileInformation.cFileName[0] != '.'){
            str += "\\";   str += FileInformation.cFileName;
            //If it's a file, apply erase logic
            if(!(FileInformation.dwFileAttributes & winapi::file_attribute_directory)){
               f(str.c_str(), FileInformation.cFileName);
            }
         }
      //Go to the next file
      } while(winapi::find_next_file(hFile, &FileInformation) == 1);

      // Close handle and see if the loop has ended with an error
      winapi::find_close(hFile);
      if(winapi::get_last_error() != winapi::error_no_more_files){
         return false;
      }
   }
   return true;
}


#else    //#if defined (BOOST_INTERPROCESS_WINDOWS)

typedef int       file_handle_t;
typedef off_t     offset_t;

typedef struct mapping_handle_impl_t
{
   file_handle_t  handle;
   bool           is_xsi;
}  mapping_handle_t;

typedef enum { read_only      = O_RDONLY
             , read_write     = O_RDWR
             , copy_on_write
             , read_private
             , invalid_mode   = 0xffff
             } mode_t;

typedef enum { file_begin     = SEEK_SET
             , file_end       = SEEK_END
             , file_current   = SEEK_CUR
             } file_pos_t;

typedef int map_options_t;
static const map_options_t default_map_options = map_options_t(-1);

namespace ipcdetail{

inline mapping_handle_t mapping_handle_from_file_handle(file_handle_t hnd)
{
   mapping_handle_t ret;
   ret.handle = hnd;
   ret.is_xsi = false;
   return ret;
}

inline file_handle_t file_handle_from_mapping_handle(mapping_handle_t hnd)
{  return hnd.handle; }

inline bool create_directory(const char *path)
{  return ::mkdir(path, 0777) == 0 && ::chmod(path, 0777) == 0; }

inline bool remove_directory(const char *path)
{  return ::rmdir(path) == 0; }

inline bool get_temporary_path(char *buffer, std::size_t buf_len, std::size_t &required_len)
{
   required_len = 5u;
   if(buf_len < required_len)
      return false;
   else{
      std::strcpy(buffer, "/tmp");
   }
   return true;
}

inline file_handle_t create_new_file
   (const char *name, mode_t mode, const permissions & perm = permissions(), bool temporary = false)
{
   (void)temporary;
   int ret = ::open(name, ((int)mode) | O_EXCL | O_CREAT, perm.get_permissions());
   if(ret >= 0){
      ::fchmod(ret, perm.get_permissions());
   }
   return ret;
}

inline file_handle_t create_or_open_file
   (const char *name, mode_t mode, const permissions & perm = permissions(), bool temporary = false)
{
   (void)temporary;
   int ret = -1;
   //We need a loop to change permissions correctly using fchmod, since
   //with "O_CREAT only" ::open we don't know if we've created or opened the file.
   while(true){
      ret = ::open(name, ((int)mode) | O_EXCL | O_CREAT, perm.get_permissions());
      if(ret >= 0){
         ::fchmod(ret, perm.get_permissions());
         break;
      }
      else if(errno == EEXIST){
         if((ret = ::open(name, (int)mode)) >= 0 || errno != ENOENT){
            break;
         }
      }
      else{
         break;
      }
   }
   return ret;
}

inline file_handle_t open_existing_file
   (const char *name, mode_t mode, bool temporary = false)
{
   (void)temporary;
   return ::open(name, (int)mode);
}

inline bool delete_file(const char *name)
{  return ::unlink(name) == 0;   }

inline bool truncate_file (file_handle_t hnd, std::size_t size)
{
   typedef boost::move_detail::make_unsigned<off_t>::type uoff_t;
   if(uoff_t((std::numeric_limits<off_t>::max)()) < size){
      errno = EINVAL;
      return false;
   }
   return 0 == ::ftruncate(hnd, off_t(size));
}

inline bool get_file_size(file_handle_t hnd, offset_t &size)
{
   struct stat data;
   bool ret = 0 == ::fstat(hnd, &data);
   if(ret){
      size = data.st_size;
   }
   return ret;
}

inline bool set_file_pointer(file_handle_t hnd, offset_t off, file_pos_t pos)
{  return ((off_t)(-1)) != ::lseek(hnd, off, (int)pos); }

inline bool get_file_pointer(file_handle_t hnd, offset_t &off)
{
   off = ::lseek(hnd, 0, SEEK_CUR);
   return off != ((off_t)-1);
}

inline bool write_file(file_handle_t hnd, const void *data, std::size_t numdata)
{  return (ssize_t(numdata)) == ::write(hnd, data, numdata);  }

inline file_handle_t invalid_file()
{  return -1;  }

inline bool close_file(file_handle_t hnd)
{  return ::close(hnd) == 0;   }

inline bool acquire_file_lock(file_handle_t hnd)
{
   struct ::flock lock;
   lock.l_type    = F_WRLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLKW, &lock);
}

inline bool try_acquire_file_lock(file_handle_t hnd, bool &acquired)
{
   struct ::flock lock;
   lock.l_type    = F_WRLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   int ret = ::fcntl(hnd, F_SETLK, &lock);
   if(ret == -1){
      return (errno == EAGAIN || errno == EACCES) ?
               (acquired = false, true) : false;
   }
   return (acquired = true);
}

inline bool release_file_lock(file_handle_t hnd)
{
   struct ::flock lock;
   lock.l_type    = F_UNLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLK, &lock);
}

inline bool acquire_file_lock_sharable(file_handle_t hnd)
{
   struct ::flock lock;
   lock.l_type    = F_RDLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLKW, &lock);
}

inline bool try_acquire_file_lock_sharable(file_handle_t hnd, bool &acquired)
{
   struct flock lock;
   lock.l_type    = F_RDLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   int ret = ::fcntl(hnd, F_SETLK, &lock);
   if(ret == -1){
      return (errno == EAGAIN || errno == EACCES) ?
               (acquired = false, true) : false;
   }
   return (acquired = true);
}

inline bool release_file_lock_sharable(file_handle_t hnd)
{  return release_file_lock(hnd);   }

#if 0
inline bool acquire_file_lock(file_handle_t hnd)
{  return 0 == ::flock(hnd, LOCK_EX); }

inline bool try_acquire_file_lock(file_handle_t hnd, bool &acquired)
{
   int ret = ::flock(hnd, LOCK_EX | LOCK_NB);
   acquired = ret == 0;
   return (acquired || errno == EWOULDBLOCK);
}

inline bool release_file_lock(file_handle_t hnd)
{  return 0 == ::flock(hnd, LOCK_UN); }

inline bool acquire_file_lock_sharable(file_handle_t hnd)
{  return 0 == ::flock(hnd, LOCK_SH); }

inline bool try_acquire_file_lock_sharable(file_handle_t hnd, bool &acquired)
{
   int ret = ::flock(hnd, LOCK_SH | LOCK_NB);
   acquired = ret == 0;
   return (acquired || errno == EWOULDBLOCK);
}

inline bool release_file_lock_sharable(file_handle_t hnd)
{  return 0 == ::flock(hnd, LOCK_UN); }
#endif

inline bool delete_subdirectories_recursive
   (const std::string &refcstrRootDirectory, const char *dont_delete_this)
{
   DIR *d = opendir(refcstrRootDirectory.c_str());
   if(!d) {
      return false;
   }

   struct dir_close
   {
      DIR *d_;
      dir_close(DIR *d) : d_(d) {}
      ~dir_close() { ::closedir(d_); }
   } dc(d); (void)dc;

   struct ::dirent *de;
   struct ::stat st;
   std::string fn;

   while((de=::readdir(d))) {
      if( de->d_name[0] == '.' && ( de->d_name[1] == '\0'
            || (de->d_name[1] == '.' && de->d_name[2] == '\0' )) ){
         continue;
      }
      if(dont_delete_this && std::strcmp(dont_delete_this, de->d_name) == 0){
         continue;
      }
      fn = refcstrRootDirectory;
      fn += '/';
      fn += de->d_name;

      if(std::remove(fn.c_str())) {
         if(::stat(fn.c_str(), & st)) {
            return false;
         }
         if(S_ISDIR(st.st_mode)) {
            if(!delete_subdirectories_recursive(fn, 0) ){
               return false;
            }
         } else {
            return false;
         }
      }
   }
   return std::remove(refcstrRootDirectory.c_str()) ? false : true;
}

template<class Function>
inline bool for_each_file_in_dir(const char *dir, Function f)
{
   std::string refcstrRootDirectory(dir);

   DIR *d = opendir(refcstrRootDirectory.c_str());
   if(!d) {
      return false;
   }

   struct dir_close
   {
      DIR *d_;
      dir_close(DIR *d) : d_(d) {}
      ~dir_close() { ::closedir(d_); }
   } dc(d); (void)dc;

   struct ::dirent *de;
   struct ::stat st;
   std::string fn;

   while((de=::readdir(d))) {
      if( de->d_name[0] == '.' && ( de->d_name[1] == '\0'
            || (de->d_name[1] == '.' && de->d_name[2] == '\0' )) ){
         continue;
      }
      fn = refcstrRootDirectory;
      fn += '/';
      fn += de->d_name;

      if(::stat(fn.c_str(), & st)) {
         return false;
      }
      //If it's a file, apply erase logic
      if(!S_ISDIR(st.st_mode)) {
         f(fn.c_str(), de->d_name);
      }
   }
   return true;
}


//This function erases all the subdirectories of a directory except the one pointed by "dont_delete_this"
inline bool delete_subdirectories(const std::string &refcstrRootDirectory, const char *dont_delete_this)
{
   return delete_subdirectories_recursive(refcstrRootDirectory, dont_delete_this );
}

#endif   //#if defined (BOOST_INTERPROCESS_WINDOWS)

inline bool open_or_create_directory(const char *dir_name)
{
   //If fails, check that it's because it already exists
   if(!create_directory(dir_name)){
      error_info info(system_error_code());
      if(info.get_error_code() != already_exists_error){
         return false;
      }
   }
   return true;
}

inline std::string get_temporary_path()
{
   std::size_t required_len = 0;
   get_temporary_path((char*)0, 0, required_len);
   std::string ret_str(required_len, char(0));
   get_temporary_path(&ret_str[0], ret_str.size(), required_len);
   while(!ret_str.empty() && !ret_str[ret_str.size()-1]){
      ret_str.erase(ret_str.size()-1);
   }

   return ret_str;
}

#ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES

inline std::wstring get_temporary_wpath()
{
   std::size_t required_len = 0;
   get_temporary_path((wchar_t*)0, 0, required_len);
   std::wstring ret_str(required_len, char(0));
   get_temporary_path(&ret_str[0], ret_str.size(), required_len);
   while(!ret_str.empty() && !ret_str[ret_str.size()-1]){
      ret_str.erase(ret_str.size()-1);
   }

   return ret_str;
}

#endif

}  //namespace ipcdetail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP
