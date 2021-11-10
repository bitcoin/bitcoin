//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2010-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ROBUST_EMULATION_HPP
#define BOOST_INTERPROCESS_ROBUST_EMULATION_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/detail/intermodule_singleton.hpp>
#include <boost/interprocess/detail/portable_intermodule_singleton.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#include <string>

namespace boost{
namespace interprocess{
namespace ipcdetail{

namespace robust_emulation_helpers {

template<class T>
class mutex_traits
{
   public:
   static void take_ownership(T &t)
   {  t.take_ownership(); }
};

inline void remove_if_can_lock_file(const char *file_path)
{
   file_handle_t fhnd = open_existing_file(file_path, read_write);

   if(fhnd != invalid_file()){
      bool acquired;
      if(try_acquire_file_lock(fhnd, acquired) && acquired){
         delete_file(file_path);
      }
      close_file(fhnd);
   }
}

inline const char *robust_lock_subdir_path()
{  return "robust"; }

inline const char *robust_lock_prefix()
{  return "lck"; }

inline void robust_lock_path(std::string &s)
{
   get_shared_dir(s);
   s += "/";
   s += robust_lock_subdir_path();
}

inline void create_and_get_robust_lock_file_path(std::string &s, OS_process_id_t pid)
{
   intermodule_singleton_helpers::create_tmp_subdir_and_get_pid_based_filepath
      (robust_lock_subdir_path(), robust_lock_prefix(), pid, s);
}

//This class will be a intermodule_singleton. The constructor will create
//a lock file, the destructor will erase it.
//
//We should take in care that another process might be erasing unlocked
//files while creating this one, so there are some race conditions we must
//take in care to guarantee some robustness.
class robust_mutex_lock_file
{
   file_handle_t fd;
   std::string fname;
   public:
   robust_mutex_lock_file()
   {
      permissions p;
      p.set_unrestricted();
      //Remove old lock files of other processes
      remove_old_robust_lock_files();
      //Create path and obtain lock file path for this process
      create_and_get_robust_lock_file_path(fname, get_current_process_id());

      //Now try to open or create the lock file
      fd = create_or_open_file(fname.c_str(), read_write, p);
      //If we can't open or create it, then something unrecoverable has happened
      if(fd == invalid_file()){
         throw interprocess_exception(other_error, "Robust emulation robust_mutex_lock_file constructor failed: could not open or create file");
      }

      //Now we must take in care a race condition with another process
      //calling "remove_old_robust_lock_files()". No other threads from this
      //process will be creating the lock file because intermodule_singleton
      //guarantees this. So let's loop acquiring the lock and checking if we
      //can't exclusively create the file (if the file is erased by another process
      //then this exclusive open would fail). If the file can't be exclusively created
      //then we have correctly open/create and lock the file. If the file can
      //be exclusively created, then close previous locked file and try again.
      while(1){
         bool acquired;
         if(!try_acquire_file_lock(fd, acquired) || !acquired ){
            throw interprocess_exception(other_error, "Robust emulation robust_mutex_lock_file constructor failed: try_acquire_file_lock");
         }
         //Creating exclusively must fail with already_exists_error
         //to make sure we've locked the file and no one has
         //deleted it between creation and locking
         file_handle_t fd2 = create_new_file(fname.c_str(), read_write, p);
         if(fd2 != invalid_file()){
            close_file(fd);
            fd = fd2;
            continue;
         }
         //If exclusive creation fails with expected error go ahead
         else if(error_info(system_error_code()).get_error_code() == already_exists_error){ //must already exist
            //Leak descriptor to mantain the file locked until the process dies
            break;
         }
         //If exclusive creation fails with unexpected error throw an unrecoverable error
         else{
            close_file(fd);
            throw interprocess_exception(other_error, "Robust emulation robust_mutex_lock_file constructor failed: create_file filed with unexpected error");
         }
      }
   }

   ~robust_mutex_lock_file()
   {
      //The destructor is guaranteed by intermodule_singleton to be
      //executed serialized between all threads from current process,
      //so we just need to close and unlink the file.
      close_file(fd);
      //If some other process deletes the file before us after
      //closing it there should not be any problem.
      delete_file(fname.c_str());
   }

   private:
   //This functor is execute for all files in the lock file directory
   class other_process_lock_remover
   {
      public:
      void operator()(const char *filepath, const char *filename)
      {
         std::string pid_str;
         //If the lock file is not our own lock file, then try to do the cleanup
         if(!intermodule_singleton_helpers::check_if_filename_complies_with_pid
            (filename, robust_lock_prefix(), get_current_process_id(), pid_str)){
            remove_if_can_lock_file(filepath);
         }
      }
   };

   bool remove_old_robust_lock_files()
   {
      std::string refcstrRootDirectory;
      robust_lock_path(refcstrRootDirectory);
      return for_each_file_in_dir(refcstrRootDirectory.c_str(), other_process_lock_remover());
   }
};

}  //namespace robust_emulation_helpers {

//This is the mutex class. Mutex should follow mutex concept
//with an additonal "take_ownership()" function to take ownership of the
//mutex when robust_spin_mutex determines the previous owner was dead.
template<class Mutex>
class robust_spin_mutex
{
   public:
   static const boost::uint32_t correct_state = 0;
   static const boost::uint32_t fixing_state  = 1;
   static const boost::uint32_t broken_state  = 2;

   typedef robust_emulation_helpers::mutex_traits<Mutex> mutex_traits_t;

   robust_spin_mutex();
   void lock();
   bool try_lock();
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);
   void unlock();
   void consistent();
   bool previous_owner_dead();

   private:
   static const unsigned int spin_threshold = 100u;
   bool lock_own_unique_file();
   bool robust_check();
   bool check_if_owner_dead_and_take_ownership_atomically();
   bool is_owner_dead(boost::uint32_t own);
   void owner_to_filename(boost::uint32_t own, std::string &s);
   //The real mutex
   Mutex mtx;
   //The pid of the owner
   volatile boost::uint32_t owner;
   //The state of the mutex (correct, fixing, broken)
   volatile boost::uint32_t state;
};

template<class Mutex>
inline robust_spin_mutex<Mutex>::robust_spin_mutex()
   : mtx(), owner(get_invalid_process_id()), state(correct_state)
{}

template<class Mutex>
inline void robust_spin_mutex<Mutex>::lock()
{  try_based_lock(*this);  }

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::try_lock()
{
   //Same as lock() but without spinning
   if(atomic_read32(&this->state) == broken_state){
      throw interprocess_exception(lock_error, "Broken id");
   }

   if(!this->lock_own_unique_file()){
      throw interprocess_exception(lock_error, "Broken id");
   }

   if (mtx.try_lock()){
      atomic_write32(&this->owner, get_current_process_id());
      return true;
   }
   else{
      if(!this->robust_check()){
         return false;
      }
      else{
         return true;
      }
   }
}

template<class Mutex>
template<class TimePoint>
inline bool robust_spin_mutex<Mutex>::timed_lock
   (const TimePoint &abs_time)
{  return try_based_timed_lock(*this, abs_time);   }

template<class Mutex>
inline void robust_spin_mutex<Mutex>::owner_to_filename(boost::uint32_t own, std::string &s)
{
   robust_emulation_helpers::create_and_get_robust_lock_file_path(s, own);
}

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::robust_check()
{
   //If the old owner was dead, and we've acquired ownership, mark
   //the mutex as 'fixing'. This means that a "consistent()" is needed
   //to avoid marking the mutex as "broken" when the mutex is unlocked.
   if(!this->check_if_owner_dead_and_take_ownership_atomically()){
      return false;
   }
   atomic_write32(&this->state, fixing_state);
   return true;
}

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::check_if_owner_dead_and_take_ownership_atomically()
{
   boost::uint32_t cur_owner = get_current_process_id();
   boost::uint32_t old_owner = atomic_read32(&this->owner), old_owner2;
   //The cas loop guarantees that only one thread from this or another process
   //will succeed taking ownership
   do{
      //Check if owner is dead
      if(!this->is_owner_dead(old_owner)){
         return false;
      }
      //If it's dead, try to mark this process as the owner in the owner field
      old_owner2 = old_owner;
      old_owner = atomic_cas32(&this->owner, cur_owner, old_owner);
   }while(old_owner2 != old_owner);
   //If success, we fix mutex internals to assure our ownership
   mutex_traits_t::take_ownership(mtx);
   return true;
}

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::is_owner_dead(boost::uint32_t own)
{
   //If owner is an invalid id, then it's clear it's dead
   if(own == (boost::uint32_t)get_invalid_process_id()){
      return true;
   }

   //Obtain the lock filename of the owner field
   std::string file;
   this->owner_to_filename(own, file);

   //Now the logic is to open and lock it
   file_handle_t fhnd = open_existing_file(file.c_str(), read_write);

   if(fhnd != invalid_file()){
      //If we can open the file, lock it.
      bool acquired;
      if(try_acquire_file_lock(fhnd, acquired) && acquired){
         //If locked, just delete the file
         delete_file(file.c_str());
         close_file(fhnd);
         return true;
      }
      //If not locked, the owner is suppossed to be still alive
      close_file(fhnd);
   }
   else{
      //If the lock file does not exist then the owner is dead (a previous cleanup)
      //function has deleted the file. If there is another reason, then this is
      //an unrecoverable error
      if(error_info(system_error_code()).get_error_code() == not_found_error){
         return true;
      }
   }
   return false;
}

template<class Mutex>
inline void robust_spin_mutex<Mutex>::consistent()
{
   //This function supposes the previous state was "fixing"
   //and the current process holds the mutex
   if(atomic_read32(&this->state) != fixing_state &&
      atomic_read32(&this->owner) != (boost::uint32_t)get_current_process_id()){
      throw interprocess_exception(lock_error, "Broken id");
   }
   //If that's the case, just update mutex state
   atomic_write32(&this->state, correct_state);
}

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::previous_owner_dead()
{
   //Notifies if a owner recovery has been performed in the last lock()
   return atomic_read32(&this->state) == fixing_state;
}

template<class Mutex>
inline void robust_spin_mutex<Mutex>::unlock()
{
   //If in "fixing" state, unlock and mark the mutex as unrecoverable
   //so next locks will fail and all threads will be notified that the
   //data protected by the mutex was not recoverable.
   if(atomic_read32(&this->state) == fixing_state){
      atomic_write32(&this->state, broken_state);
   }
   //Write an invalid owner to minimize pid reuse possibility
   atomic_write32(&this->owner, get_invalid_process_id());
   mtx.unlock();
}

template<class Mutex>
inline bool robust_spin_mutex<Mutex>::lock_own_unique_file()
{
   //This function forces instantiation of the singleton
   robust_emulation_helpers::robust_mutex_lock_file* dummy =
      &ipcdetail::intermodule_singleton
         <robust_emulation_helpers::robust_mutex_lock_file>::get();
   return dummy != 0;
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
