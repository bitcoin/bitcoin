//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_PORTABLE_INTERMODULE_SINGLETON_HPP
#define BOOST_INTERPROCESS_PORTABLE_INTERMODULE_SINGLETON_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/managed_global_memory.hpp>
#include <boost/interprocess/detail/intermodule_singleton_common.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/file_locking_helpers.hpp>
#include <boost/assert.hpp>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

namespace boost{
namespace interprocess{
namespace ipcdetail{

typedef basic_managed_global_memory<shared_memory_object, true>    managed_global_memory;

namespace intermodule_singleton_helpers {

static void create_tmp_subdir_and_get_pid_based_filepath
   (const char *subdir_name, const char *file_prefix, OS_process_id_t pid, std::string &s, bool creation_time = false)
{
   //Let's create a lock file for each process gmem that will mark if
   //the process is alive or not
   create_shared_dir_and_clean_old(s);
   s += "/";
   s += subdir_name;
   if(!open_or_create_directory(s.c_str())){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
   s += "/";
   s += file_prefix;
   if(creation_time){
      std::string sstamp;
      get_pid_creation_time_str(sstamp);
      s += sstamp;
   }
   else{
      pid_str_t pid_str;
      get_pid_str(pid_str, pid);
      s += pid_str;
   }
}

static bool check_if_filename_complies_with_pid
   (const char *filename, const char *prefix, OS_process_id_t pid, std::string &file_suffix, bool creation_time = false)
{
   //Check if filename complies with lock file name pattern
   std::string fname(filename);
   std::string fprefix(prefix);
   if(fname.size() <= fprefix.size()){
      return false;
   }
   fname.resize(fprefix.size());
   if(fname != fprefix){
      return false;
   }

   //If not our lock file, delete it if we can lock it
   fname = filename;
   fname.erase(0, fprefix.size());
   pid_str_t pid_str;
   get_pid_str(pid_str, pid);
   file_suffix = pid_str;
   if(creation_time){
      std::size_t p = fname.find('_');
      if (p == std::string::npos){
         return false;
      }
      std::string save_suffix(fname);
      fname.erase(p);
      fname.swap(file_suffix);
      bool ret = (file_suffix == fname);
      file_suffix.swap(save_suffix);
      return ret;
   }
   else{
      fname.swap(file_suffix);
      return (file_suffix == fname);
   }
}

template<>
struct thread_safe_global_map_dependant<managed_global_memory>
{
   private:
   static const int GMemMarkToBeRemoved = -1;
   static const int GMemNotPresent      = -2;

   static const char *get_lock_file_subdir_name()
   {  return "gmem";  }

   static const char *get_lock_file_base_name()
   {  return "lck";  }

   static void create_and_get_singleton_lock_file_path(std::string &s)
   {
      create_tmp_subdir_and_get_pid_based_filepath
         (get_lock_file_subdir_name(), get_lock_file_base_name(), get_current_process_id(), s, true);
   }

   struct gmem_erase_func
   {
      gmem_erase_func(const char *shm_name, const char *singleton_lock_file_path, managed_global_memory & shm)
         :shm_name_(shm_name), singleton_lock_file_path_(singleton_lock_file_path), shm_(shm)
      {}

      void operator()()
      {
         locking_file_serial_id *pserial_id = shm_.find<locking_file_serial_id>("lock_file_fd").first;
         if(pserial_id){
            pserial_id->fd = GMemMarkToBeRemoved;
         }
         delete_file(singleton_lock_file_path_);
         shared_memory_object::remove(shm_name_);
      }

      const char * const shm_name_;
      const char * const singleton_lock_file_path_;
      managed_global_memory & shm_;
   };

   //This function applies shared memory erasure logic based on the passed lock file.
   static void apply_gmem_erase_logic(const char *filepath, const char *filename)
   {
      int fd = GMemMarkToBeRemoved;
      try{
         std::string str;
         //If the filename is current process lock file, then avoid it
         if(check_if_filename_complies_with_pid
            (filename, get_lock_file_base_name(), get_current_process_id(), str, true)){
            return;
         }
         //Open and lock the other process' lock file
         fd = try_open_and_lock_file(filepath);
         if(fd < 0){
            return;
         }
         //If done, then the process is dead so take global shared memory name
         //(the name is based on the lock file name) and try to apply erasure logic
         str.insert(0, get_map_base_name());
         try{
            managed_global_memory shm(open_only, str.c_str());
            gmem_erase_func func(str.c_str(), filepath, shm);
            shm.try_atomic_func(func);
         }
         catch(interprocess_exception &e){
            //If shared memory is not found erase the lock file
            if(e.get_error_code() == not_found_error){
               delete_file(filepath);
            }
         }
      }
      catch(...){

      }
      if(fd >= 0){
         close_lock_file(fd);
      }
   }

   public:

   static bool remove_old_gmem()
   {
      std::string refcstrRootDirectory;
      get_shared_dir(refcstrRootDirectory);
      refcstrRootDirectory += "/";
      refcstrRootDirectory += get_lock_file_subdir_name();
      return for_each_file_in_dir(refcstrRootDirectory.c_str(), apply_gmem_erase_logic);
   }

   struct lock_file_logic
   {
      lock_file_logic(managed_global_memory &shm)
         : mshm(shm)
      {  shm.atomic_func(*this); }

      void operator()(void)
      {
         retry_with_new_map = false;

         //First find the file locking descriptor id
         locking_file_serial_id *pserial_id =
            mshm.find<locking_file_serial_id>("lock_file_fd").first;

         int fd;
         //If not found schedule a creation
         if(!pserial_id){
            fd = GMemNotPresent;
         }
         //Else get it
         else{
            fd = pserial_id->fd;
         }
         //If we need to create a new one, do it
         if(fd == GMemNotPresent){
            std::string lck_str;
            //Create a unique current pid based lock file path
            create_and_get_singleton_lock_file_path(lck_str);
            //Open or create and lock file
            int fd_lockfile = open_or_create_and_lock_file(lck_str.c_str());
            //If failed, write a bad file descriptor to notify other modules that
            //something was wrong and unlink shared memory. Mark the function object
            //to tell caller to retry with another shared memory
            if(fd_lockfile < 0){
               this->register_lock_file(GMemMarkToBeRemoved);
               std::string s;
               get_map_name(s);
               shared_memory_object::remove(s.c_str());
               retry_with_new_map = true;
            }
            //If successful, register the file descriptor
            else{
               this->register_lock_file(fd_lockfile);
            }
         }
         //If the fd was invalid (maybe a previous try failed) notify caller that
         //should retry creation logic, since this shm might have been already
         //unlinked since the shm was removed
         else if (fd == GMemMarkToBeRemoved){
            retry_with_new_map = true;
         }
         //If the stored fd is not valid (a open fd, a normal file with the
         //expected size, or does not have the same file id number,
         //then it's an old shm from an old process with the same pid.
         //If that's the case, mark it as invalid
         else if(!is_valid_fd(fd) ||
               !is_normal_file(fd) ||
               0 != get_size(fd) ||
               !compare_file_serial(fd, *pserial_id)){
            pserial_id->fd = GMemMarkToBeRemoved;
            std::string s;
            get_map_name(s);
            shared_memory_object::remove(s.c_str());
            retry_with_new_map = true;
         }
         else{
            //If the lock file is ok, increment reference count of
            //attached modules to shared memory
            atomic_inc32(&pserial_id->modules_attached_to_gmem_count);
         }
      }

      bool retry() const { return retry_with_new_map; }

      private:
      locking_file_serial_id * register_lock_file(int fd)
      {
         locking_file_serial_id *pinfo = mshm.construct<locking_file_serial_id>("lock_file_fd")();
         fill_file_serial_id(fd, *pinfo);
         return pinfo;
      }

      managed_global_memory &mshm;
      bool retry_with_new_map;
   };

   static void construct_map(void *addr)
   {
      std::string s;
      intermodule_singleton_helpers::get_map_name(s);
      const char *MapName = s.c_str();
      const std::size_t MapSize = intermodule_singleton_helpers::get_map_size();;
      ::new (addr)managed_global_memory(open_or_create, MapName, MapSize);
   }

   struct unlink_map_logic
   {
      unlink_map_logic(managed_global_memory &mshm)
         : mshm_(mshm)
      {  mshm.atomic_func(*this);  }

      void operator()()
      {
         locking_file_serial_id *pserial_id =
            mshm_.find<locking_file_serial_id>
               ("lock_file_fd").first;
         BOOST_ASSERT(0 != pserial_id);
         if(1 == atomic_dec32(&pserial_id->modules_attached_to_gmem_count)){
            int fd = pserial_id->fd;
            if(fd > 0){
               pserial_id->fd = GMemMarkToBeRemoved;
               std::string s;
               create_and_get_singleton_lock_file_path(s);
               delete_file(s.c_str());
               close_lock_file(fd);
               intermodule_singleton_helpers::get_map_name(s);
               shared_memory_object::remove(s.c_str());
            }
         }
      }

      private:
      managed_global_memory &mshm_;
   };

   static ref_count_ptr *find(managed_global_memory &map, const char *name)
   {
      return map.find<ref_count_ptr>(name).first;
   }

   static ref_count_ptr *insert(managed_global_memory &map, const char *name, const ref_count_ptr &ref)
   {
      return map.construct<ref_count_ptr>(name)(ref);
   }

   static bool erase(managed_global_memory &map, const char *name)
   {
      return map.destroy<ref_count_ptr>(name);
   }

   template<class F>
   static void atomic_func(managed_global_memory &map, F &f)
   {
      map.atomic_func(f);
   }
};

}  //namespace intermodule_singleton_helpers {

template<typename C, bool LazyInit = true, bool Phoenix = false>
class portable_intermodule_singleton
   : public intermodule_singleton_impl<C, LazyInit, Phoenix, managed_global_memory>
{};

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_PORTABLE_INTERMODULE_SINGLETON_HPP
