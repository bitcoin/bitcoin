//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_INTERMODULE_SINGLETON_COMMON_HPP
#define BOOST_INTERPROCESS_INTERMODULE_SINGLETON_COMMON_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/container/detail/type_traits.hpp>  //alignment_of, aligned_storage
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/assert.hpp>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <typeinfo>
#include <sstream>

namespace boost{
namespace interprocess{
namespace ipcdetail{

namespace intermodule_singleton_helpers {

inline void get_pid_creation_time_str(std::string &s)
{
   std::stringstream stream;
   stream << get_current_process_id() << '_';
   stream.precision(6);
   stream << std::fixed << get_current_process_creation_time();
   s = stream.str();
}

inline const char *get_map_base_name()
{  return "bip.gmem.map.";  }

inline void get_map_name(std::string &map_name)
{
   get_pid_creation_time_str(map_name);
   map_name.insert(0, get_map_base_name());
}

inline std::size_t get_map_size()
{  return 65536;  }

template<class ThreadSafeGlobalMap>
struct thread_safe_global_map_dependant;

}  //namespace intermodule_singleton_helpers {

//This class contains common code for all singleton types, so that we instantiate this
//code just once per module. This class also holds a thread soafe global map
//to be used by all instances protected with a reference count
template<class ThreadSafeGlobalMap>
class intermodule_singleton_common
{
   public:
   typedef void*(singleton_constructor_t)(ThreadSafeGlobalMap &);
   typedef void (singleton_destructor_t)(void *, ThreadSafeGlobalMap &);

   static const ::boost::uint32_t Uninitialized       = 0u;
   static const ::boost::uint32_t Initializing        = 1u;
   static const ::boost::uint32_t Initialized         = 2u;
   static const ::boost::uint32_t Broken              = 3u;
   static const ::boost::uint32_t Destroyed           = 4u;

   //Initialize this_module_singleton_ptr, creates the global map if needed and also creates an unique
   //opaque type in global map through a singleton_constructor_t function call,
   //initializing the passed pointer to that unique instance.
   //
   //We have two concurrency types here. a)the global map/singleton creation must
   //be safe between threads of this process but in different modules/dlls. b)
   //the pointer to the singleton is per-module, so we have to protect this
   //initization between threads of the same module.
   //
   //All static variables declared here are shared between inside a module
   //so atomic operations will synchronize only threads of the same module.
   static void initialize_singleton_logic
      (void *&ptr, volatile boost::uint32_t &this_module_singleton_initialized, singleton_constructor_t constructor, bool phoenix)
   {
      //If current module is not initialized enter to lock free logic
      if(atomic_read32(&this_module_singleton_initialized) != Initialized){
         //Now a single thread of the module will succeed in this CAS.
         //trying to pass from Uninitialized to Initializing
         ::boost::uint32_t previous_module_singleton_initialized = atomic_cas32
            (&this_module_singleton_initialized, Initializing, Uninitialized);
         //If the thread succeeded the CAS (winner) it will compete with other
         //winner threads from other modules to create the global map
         if(previous_module_singleton_initialized == Destroyed){
            //Trying to resurrect a dead Phoenix singleton. Just try to
            //mark it as uninitialized and start again
            if(phoenix){
               atomic_cas32(&this_module_singleton_initialized, Uninitialized, Destroyed);
               previous_module_singleton_initialized = atomic_cas32
                  (&this_module_singleton_initialized, Initializing, Uninitialized);
            }
            //Trying to resurrect a non-Phoenix dead singleton is an error
            else{
               throw interprocess_exception("Boost.Interprocess: Dead reference on non-Phoenix singleton of type");
            }
         }
         if(previous_module_singleton_initialized == Uninitialized){
            try{
               //Now initialize the global map, this function must solve concurrency
               //issues between threads of several modules
               initialize_global_map_handle();
               //Now try to create the singleton in global map.
               //This function solves concurrency issues
               //between threads of several modules
               ThreadSafeGlobalMap *const pmap = get_map_ptr();
               void *tmp = constructor(*pmap);
               //Increment the module reference count that reflects how many
               //singletons this module holds, so that we can safely destroy
               //module global map object when no singleton is left
               atomic_inc32(&this_module_singleton_count);
               //Insert a barrier before assigning the pointer to
               //make sure this assignment comes after the initialization
               atomic_write32(&this_module_singleton_initialized, Initializing);
               //Assign the singleton address to the module-local pointer
               ptr = tmp;
               //Memory barrier inserted, all previous operations should complete
               //before this one. Now marked as initialized
               atomic_write32(&this_module_singleton_initialized, Initialized);
            }
            catch(...){
               //Mark singleton failed to initialize
               atomic_write32(&this_module_singleton_initialized, Broken);
               throw;
            }
         }
         //If previous state was initializing, this means that another winner thread is
         //trying to initialize the singleton. Just wait until completes its work.
         else if(previous_module_singleton_initialized == Initializing){
            spin_wait swait;
            while(1){
               previous_module_singleton_initialized = atomic_read32(&this_module_singleton_initialized);
               if(previous_module_singleton_initialized >= Initialized){
                  //Already initialized, or exception thrown by initializer thread
                  break;
               }
               else if(previous_module_singleton_initialized == Initializing){
                  swait.yield();
               }
               else{
                  //This can't be happening!
                  BOOST_ASSERT(0);
               }
            }
         }
         else if(previous_module_singleton_initialized == Initialized){
            //Nothing to do here, the singleton is ready
         }
         //If previous state was greater than initialized, then memory is broken
         //trying to initialize the singleton.
         else{//(previous_module_singleton_initialized > Initialized)
            throw interprocess_exception("boost::interprocess::intermodule_singleton initialization failed");
         }
      }
      BOOST_ASSERT(ptr != 0);
   }

   static void finalize_singleton_logic(void *&ptr, volatile boost::uint32_t &this_module_singleton_initialized, singleton_destructor_t destructor)
   {
      //Protect destruction against lazy singletons not initialized in this execution
      if(ptr){
         //Note: this destructor might provoke a Phoenix singleton
         //resurrection. This means that this_module_singleton_count
         //might change after this call.
         ThreadSafeGlobalMap * const pmap = get_map_ptr();
         destructor(ptr, *pmap);
         ptr = 0;

         //Memory barrier to make sure pointer is nulled.
         //Mark this singleton as destroyed.
         atomic_write32(&this_module_singleton_initialized, Destroyed);

         //If this is the last singleton of this module
         //apply map destruction.
         //Note: singletons are destroyed when the module is unloaded
         //so no threads should be executing or holding references
         //to this module
         if(1 == atomic_dec32(&this_module_singleton_count)){
            destroy_global_map_handle();
         }
      }
   }

   private:
   static ThreadSafeGlobalMap *get_map_ptr()
   {
      return static_cast<ThreadSafeGlobalMap *>(static_cast<void*>(mem_holder.map_mem));
   }

   static void initialize_global_map_handle()
   {
      //Obtain unique map name and size
      spin_wait swait;
      while(1){
         //Try to pass map state to initializing
         ::boost::uint32_t tmp = atomic_cas32(&this_module_map_initialized, Initializing, Uninitialized);
         if(tmp == Initialized || tmp == Broken){
            break;
         }
         else if(tmp == Destroyed){
            tmp = atomic_cas32(&this_module_map_initialized, Uninitialized, Destroyed);
            continue;
         }
         //If some other thread is doing the work wait
         else if(tmp == Initializing){
            swait.yield();
         }
         else{ //(tmp == Uninitialized)
            //If not initialized try it again?
            try{
               //Remove old global map from the system
               intermodule_singleton_helpers::thread_safe_global_map_dependant<ThreadSafeGlobalMap>::remove_old_gmem();
               //in-place construction of the global map class
               ThreadSafeGlobalMap * const pmap = get_map_ptr();
               intermodule_singleton_helpers::thread_safe_global_map_dependant
                  <ThreadSafeGlobalMap>::construct_map(static_cast<void*>(pmap));
               //Use global map's internal lock to initialize the lock file
               //that will mark this gmem as "in use".
               typename intermodule_singleton_helpers::thread_safe_global_map_dependant<ThreadSafeGlobalMap>::
                  lock_file_logic f(*pmap);
               //If function failed (maybe a competing process has erased the shared
               //memory between creation and file locking), retry with a new instance.
               if(f.retry()){
                  pmap->~ThreadSafeGlobalMap();
                  atomic_write32(&this_module_map_initialized, Destroyed);
               }
               else{
                  //Locking succeeded, so this global map module-instance is ready
                  atomic_write32(&this_module_map_initialized, Initialized);
                  break;
               }
            }
            catch(...){
               //
               throw;
            }
         }
      }
   }

   static void destroy_global_map_handle()
   {
      if(!atomic_read32(&this_module_singleton_count)){
         //This module is being unloaded, so destroy
         //the global map object of this module
         //and unlink the global map if it's the last
         ThreadSafeGlobalMap * const pmap = get_map_ptr();
         typename intermodule_singleton_helpers::thread_safe_global_map_dependant<ThreadSafeGlobalMap>::
            unlink_map_logic f(*pmap);
         pmap->~ThreadSafeGlobalMap();
         atomic_write32(&this_module_map_initialized, Destroyed);
         //Do some cleanup for other processes old gmem instances
         intermodule_singleton_helpers::thread_safe_global_map_dependant<ThreadSafeGlobalMap>::remove_old_gmem();
      }
   }

   //Static data, zero-initalized without any dependencies
   //this_module_singleton_count is the number of singletons used by this module
   static volatile boost::uint32_t this_module_singleton_count;

   //this_module_map_initialized is the state of this module's map class object.
   //Values: Uninitialized, Initializing, Initialized, Broken
   static volatile boost::uint32_t this_module_map_initialized;

   //Raw memory to construct the global map manager
   static union mem_holder_t
   {
      unsigned char map_mem [sizeof(ThreadSafeGlobalMap)];
      ::boost::container::dtl::max_align_t aligner;
   } mem_holder;
};

template<class ThreadSafeGlobalMap>
volatile boost::uint32_t intermodule_singleton_common<ThreadSafeGlobalMap>::this_module_singleton_count;

template<class ThreadSafeGlobalMap>
volatile boost::uint32_t intermodule_singleton_common<ThreadSafeGlobalMap>::this_module_map_initialized;

template<class ThreadSafeGlobalMap>
typename intermodule_singleton_common<ThreadSafeGlobalMap>::mem_holder_t
   intermodule_singleton_common<ThreadSafeGlobalMap>::mem_holder;

//A reference count to be stored in global map holding the number
//of singletons (one per module) attached to the instance pointed by
//the internal ptr.
struct ref_count_ptr
{
   ref_count_ptr(void *p, boost::uint32_t count)
      : ptr(p), singleton_ref_count(count)
   {}
   void *ptr;
   //This reference count serves to count the number of attached
   //modules to this singleton
   volatile boost::uint32_t singleton_ref_count;
};


//Now this class is a singleton, initializing the singleton in
//the first get() function call if LazyInit is true. If false
//then the singleton will be initialized when loading the module.
template<typename C, bool LazyInit, bool Phoenix, class ThreadSafeGlobalMap>
class intermodule_singleton_impl
{
   public:

   static C& get()   //Let's make inlining easy
   {
      if(!this_module_singleton_ptr){
         if(lifetime.dummy_function()){  //This forces lifetime instantiation, for reference counted destruction
            atentry_work();
         }
      }
      return *static_cast<C*>(this_module_singleton_ptr);
   }

   private:

   static void atentry_work()
   {
      intermodule_singleton_common<ThreadSafeGlobalMap>::initialize_singleton_logic
         (this_module_singleton_ptr, this_module_singleton_initialized, singleton_constructor, Phoenix);
   }

   static void atexit_work()
   {
      intermodule_singleton_common<ThreadSafeGlobalMap>::finalize_singleton_logic
         (this_module_singleton_ptr, this_module_singleton_initialized, singleton_destructor);
   }

   //These statics will be zero-initialized without any constructor call dependency
   //this_module_singleton_ptr will be a module-local pointer to the singleton
   static void*                      this_module_singleton_ptr;

   //this_module_singleton_count will be used to synchronize threads of the same module
   //for access to a singleton instance, and to flag the state of the
   //singleton.
   static volatile boost::uint32_t   this_module_singleton_initialized;

   //This class destructor will trigger singleton destruction
   struct lifetime_type_lazy
   {
      bool dummy_function()
      {  return m_dummy == 0; }

      ~lifetime_type_lazy()
      {
         //if(!Phoenix){
            //atexit_work();
         //}
      }

      //Dummy volatile so that the compiler can't resolve its value at compile-time
      //and can't avoid lifetime_type instantiation if dummy_function() is called.
      static volatile int m_dummy;
   };

   struct lifetime_type_static
      : public lifetime_type_lazy
   {
      lifetime_type_static()
      {  atentry_work();  }
   };

   typedef typename if_c
      <LazyInit, lifetime_type_lazy, lifetime_type_static>::type lifetime_type;

   static lifetime_type lifetime;

   //A functor to be executed inside global map lock that just
   //searches for the singleton in map and if not present creates a new one.
   //If singleton constructor throws, the exception is propagated
   struct init_atomic_func
   {
      init_atomic_func(ThreadSafeGlobalMap &m)
         : m_map(m), ret_ptr()
      {}

      void operator()()
      {
         ref_count_ptr *rcount = intermodule_singleton_helpers::thread_safe_global_map_dependant
            <ThreadSafeGlobalMap>::find(m_map, typeid(C).name());
         if(!rcount){
            C *p = new C;
            try{
               ref_count_ptr val(p, 0u);
               rcount = intermodule_singleton_helpers::thread_safe_global_map_dependant
                           <ThreadSafeGlobalMap>::insert(m_map, typeid(C).name(), val);
            }
            catch(...){
               intermodule_singleton_helpers::thread_safe_global_map_dependant
                           <ThreadSafeGlobalMap>::erase(m_map, typeid(C).name());
               delete p;
               throw;
            }
         }
         //if(Phoenix){
            std::atexit(&atexit_work);
         //}
         atomic_inc32(&rcount->singleton_ref_count);
         ret_ptr = rcount->ptr;
      }
      void *data() const
         { return ret_ptr;  }

      private:
      ThreadSafeGlobalMap &m_map;
      void *ret_ptr;
   };

   //A functor to be executed inside global map lock that just
   //deletes the singleton in map if the attached count reaches to zero
   struct fini_atomic_func
   {
      fini_atomic_func(ThreadSafeGlobalMap &m)
         : m_map(m)
      {}

      void operator()()
      {
         ref_count_ptr *rcount = intermodule_singleton_helpers::thread_safe_global_map_dependant
            <ThreadSafeGlobalMap>::find(m_map, typeid(C).name());
            //The object must exist
         BOOST_ASSERT(rcount);
         BOOST_ASSERT(rcount->singleton_ref_count > 0);
         //Check if last reference
         if(atomic_dec32(&rcount->singleton_ref_count) == 1){
            //If last, destroy the object
            BOOST_ASSERT(rcount->ptr != 0);
            C *pc = static_cast<C*>(rcount->ptr);
            //Now destroy map entry
            bool destroyed = intermodule_singleton_helpers::thread_safe_global_map_dependant
                        <ThreadSafeGlobalMap>::erase(m_map, typeid(C).name());
            (void)destroyed;  BOOST_ASSERT(destroyed == true);
            delete pc;
         }
      }

      private:
      ThreadSafeGlobalMap &m_map;
   };

   //A wrapper to execute init_atomic_func
   static void *singleton_constructor(ThreadSafeGlobalMap &map)
   {
      init_atomic_func f(map);
      intermodule_singleton_helpers::thread_safe_global_map_dependant
                  <ThreadSafeGlobalMap>::atomic_func(map, f);
      return f.data();
   }

   //A wrapper to execute fini_atomic_func
   static void singleton_destructor(void *p, ThreadSafeGlobalMap &map)
   {  (void)p;
      fini_atomic_func f(map);
      intermodule_singleton_helpers::thread_safe_global_map_dependant
                  <ThreadSafeGlobalMap>::atomic_func(map, f);
   }
};

template <typename C, bool L, bool P, class ThreadSafeGlobalMap>
volatile int intermodule_singleton_impl<C, L, P, ThreadSafeGlobalMap>::lifetime_type_lazy::m_dummy = 0;

//These will be zero-initialized by the loader
template <typename C, bool L, bool P, class ThreadSafeGlobalMap>
void *intermodule_singleton_impl<C, L, P, ThreadSafeGlobalMap>::this_module_singleton_ptr = 0;

template <typename C, bool L, bool P, class ThreadSafeGlobalMap>
volatile boost::uint32_t intermodule_singleton_impl<C, L, P, ThreadSafeGlobalMap>::this_module_singleton_initialized = 0;

template <typename C, bool L, bool P, class ThreadSafeGlobalMap>
typename intermodule_singleton_impl<C, L, P, ThreadSafeGlobalMap>::lifetime_type
   intermodule_singleton_impl<C, L, P, ThreadSafeGlobalMap>::lifetime;

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_INTERMODULE_SINGLETON_COMMON_HPP
