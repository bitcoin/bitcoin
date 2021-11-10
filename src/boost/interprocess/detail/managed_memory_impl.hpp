//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_MANAGED_MEMORY_IMPL_HPP
#define BOOST_INTERPROCESS_DETAIL_MANAGED_MEMORY_IMPL_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/detail/nothrow.hpp>
#include <boost/interprocess/detail/simple_swap.hpp>
//
#include <boost/core/no_exceptions_support.hpp>
//
#include <boost/intrusive/detail/minimal_pair_header.hpp>
#include <boost/assert.hpp>

//!\file
//!Describes a named shared memory allocation user class.
//!

namespace boost {
namespace interprocess {
namespace ipcdetail {

template<class BasicManagedMemoryImpl>
class create_open_func;

template<
         class CharType,
         class MemoryAlgorithm,
         template<class IndexConfig> class IndexType
        >
struct segment_manager_type
{
   typedef segment_manager<CharType, MemoryAlgorithm, IndexType> type;
};

//!This class is designed to be a base class to classes that manage
//!creation of objects in a fixed size memory buffer. Apart
//!from allocating raw memory, the user can construct named objects. To
//!achieve this, this class uses the reserved space provided by the allocation
//!algorithm to place a named_allocator_algo, who takes care of name mappings.
//!The class can be customized with the char type used for object names
//!and the memory allocation algorithm to be used.*/
template <  class CharType
         ,  class MemoryAlgorithm
         ,  template<class IndexConfig> class IndexType
         ,  std::size_t Offset = 0
         >
class basic_managed_memory_impl
{
   //Non-copyable
   basic_managed_memory_impl(const basic_managed_memory_impl &);
   basic_managed_memory_impl &operator=(const basic_managed_memory_impl &);

   template<class BasicManagedMemoryImpl>
   friend class create_open_func;

   public:
   typedef typename segment_manager_type
      <CharType, MemoryAlgorithm, IndexType>::type    segment_manager;
   typedef CharType                                   char_type;
   typedef MemoryAlgorithm                            memory_algorithm;
   typedef typename MemoryAlgorithm::mutex_family     mutex_family;
   typedef CharType                                   char_t;
   typedef typename MemoryAlgorithm::size_type        size_type;
   typedef typename MemoryAlgorithm::difference_type  difference_type;
   typedef difference_type                            handle_t;
   typedef typename segment_manager::
      const_named_iterator                            const_named_iterator;
   typedef typename segment_manager::
      const_unique_iterator                           const_unique_iterator;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   typedef typename
           segment_manager::char_ptr_holder_t         char_ptr_holder_t;
   //Experimental. Don't use.

   typedef typename segment_manager::multiallocation_chain  multiallocation_chain;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   static const size_type PayloadPerAllocation = segment_manager::PayloadPerAllocation;

   private:
   typedef basic_managed_memory_impl
               <CharType, MemoryAlgorithm, IndexType, Offset> self_t;
   protected:
   template<class ManagedMemory, class CharT>
   static bool grow(const CharT *filename, size_type extra_bytes)
   {
      typedef typename ManagedMemory::device_type device_type;
      //Increase file size
      try{
         offset_t old_size;
         {
            device_type f(open_or_create, filename, read_write);
            if(!f.get_size(old_size))
               return false;
            f.truncate(old_size + extra_bytes);
         }
         ManagedMemory managed_memory(open_only, filename);
         //Grow always works
         managed_memory.self_t::grow(extra_bytes);
      }
      catch(...){
         return false;
      }
      return true;
   }

   template<class ManagedMemory, class CharT>
   static bool shrink_to_fit(const CharT *filename)
   {
      typedef typename ManagedMemory::device_type device_type;
      size_type new_size;
      try{
         ManagedMemory managed_memory(open_only, filename);
         managed_memory.get_size();
         managed_memory.self_t::shrink_to_fit();
         new_size = managed_memory.get_size();
      }
      catch(...){
         return false;
      }

      //Decrease file size
      {
         device_type f(open_or_create, filename, read_write);
         f.truncate(new_size);
      }
      return true;
   }

   //!Constructor. Allocates basic resources. Never throws.
   basic_managed_memory_impl()
      : mp_header(0){}

   //!Destructor. Calls close. Never throws.
   ~basic_managed_memory_impl()
   {  this->close_impl(); }

   //!Places segment manager in the reserved space. This can throw.
   bool  create_impl   (void *addr, size_type size)
   {
      if(mp_header)  return false;

      //Check if there is enough space
      if(size < segment_manager::get_min_size())
         return false;

      //This function should not throw. The index construction can
      //throw if constructor allocates memory. So we must catch it.
      BOOST_TRY{
         //Let's construct the allocator in memory
         BOOST_ASSERT((0 == (std::size_t)addr % boost::move_detail::alignment_of<segment_manager>::value));
         mp_header       = ::new(addr, boost_container_new_t()) segment_manager(size);
      }
      BOOST_CATCH(...){
         return false;
      }
      BOOST_CATCH_END
      return true;
   }

   //!Connects to a segment manager in the reserved buffer. Never throws.
   bool  open_impl     (void *addr, size_type)
   {
      if(mp_header)  return false;
      mp_header = static_cast<segment_manager*>(addr);
      return true;
   }

   //!Frees resources. Never throws.
   bool close_impl()
   {
      bool ret = mp_header != 0;
      mp_header = 0;
      return ret;
   }

   //!Frees resources and destroys common resources. Never throws.
   bool destroy_impl()
   {
      if(mp_header == 0)
         return false;
      mp_header->~segment_manager();
      this->close_impl();
         return true;
   }

   //!
   void grow(size_type extra_bytes)
   {  mp_header->grow(extra_bytes); }

   void shrink_to_fit()
   {  mp_header->shrink_to_fit(); }

   public:

   //!Returns segment manager. Never throws.
   segment_manager *get_segment_manager() const
   {   return mp_header; }

   //!Returns the base address of the memory in this process. Never throws.
   void *   get_address   () const
   {   return reinterpret_cast<char*>(mp_header) - Offset; }

   //!Returns the size of memory segment. Never throws.
   size_type   get_size   () const
   {   return mp_header->get_size() + Offset;  }

   //!Returns the number of free bytes of the memory
   //!segment
   size_type get_free_memory() const
   {  return mp_header->get_free_memory();  }

   //!Returns the result of "all_memory_deallocated()" function
   //!of the used memory algorithm
   bool all_memory_deallocated()
   {   return mp_header->all_memory_deallocated(); }

   //!Returns the result of "check_sanity()" function
   //!of the used memory algorithm
   bool check_sanity()
   {   return mp_header->check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated) of
   //!the memory algorithm
   void zero_free_memory()
   {   mp_header->zero_free_memory(); }

   //!Transforms an absolute address into an offset from base address.
   //!The address must belong to the memory segment. Never throws.
   handle_t get_handle_from_address   (const void *ptr) const
   {
      return (handle_t)(reinterpret_cast<const char*>(ptr) -
             reinterpret_cast<const char*>(this->get_address()));
   }

   //!Returns true if the address belongs to the managed memory segment
   bool belongs_to_segment (const void *ptr) const
   {
      return ptr >= this->get_address() &&
             ptr <  (reinterpret_cast<const char*>(this->get_address()) + this->get_size());
   }

   //!Transforms previously obtained offset into an absolute address in the
   //!process space of the current process. Never throws.*/
   void *    get_address_from_handle (handle_t offset) const
   {  return reinterpret_cast<char*>(this->get_address()) + offset; }

   //!Searches for nbytes of free memory in the segment, marks the
   //!memory as used and return the pointer to the memory. If no
   //!memory is available throws a boost::interprocess::bad_alloc exception
   void* allocate             (size_type nbytes)
   {   return mp_header->allocate(nbytes);   }

   //!Searches for nbytes of free memory in the segment, marks the
   //!memory as used and return the pointer to the memory. If no memory
   //!is available returns 0. Never throws.
   void* allocate             (size_type nbytes, const std::nothrow_t &tag)
   {   return mp_header->allocate(nbytes, tag);  }

   //!Allocates nbytes bytes aligned to "alignment" bytes. "alignment"
   //!must be power of two. If no memory
   //!is available returns 0. Never throws.
   void * allocate_aligned (size_type nbytes, size_type alignment, const std::nothrow_t &tag)
   {   return mp_header->allocate_aligned(nbytes, alignment, tag);  }

   template<class T>
   T * allocation_command  (boost::interprocess::allocation_type command,   size_type limit_size,
                           size_type &prefer_in_recvd_out_size, T *&reuse)
   {  return mp_header->allocation_command(command, limit_size, prefer_in_recvd_out_size, reuse);  }

   //!Allocates nbytes bytes aligned to "alignment" bytes. "alignment"
   //!must be power of two. If no
   //!memory is available throws a boost::interprocess::bad_alloc exception
   void * allocate_aligned(size_type nbytes, size_type alignment)
   {   return mp_header->allocate_aligned(nbytes, alignment);  }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Experimental. Don't use.

   //!Allocates n_elements of elem_bytes bytes.
   //!Throws bad_alloc on failure. chain.size() is not increased on failure.
   void allocate_many(size_type elem_bytes, size_type n_elements, multiallocation_chain &chain)
   {  mp_header->allocate_many(elem_bytes, n_elements, chain); }

   //!Allocates n_elements, each one of element_lengths[i]*sizeof_element bytes.
   //!Throws bad_alloc on failure. chain.size() is not increased on failure.
   void allocate_many(const size_type *element_lengths, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {  mp_header->allocate_many(element_lengths, n_elements, sizeof_element, chain); }

   //!Allocates n_elements of elem_bytes bytes.
   //!Non-throwing version. chain.size() is not increased on failure.
   void allocate_many(const std::nothrow_t &tag, size_type elem_bytes, size_type n_elements, multiallocation_chain &chain)
   {  mp_header->allocate_many(tag, elem_bytes, n_elements, chain); }

   //!Allocates n_elements, each one of
   //!element_lengths[i]*sizeof_element bytes.
   //!Non-throwing version. chain.size() is not increased on failure.
   void allocate_many(const std::nothrow_t &tag, const size_type *elem_sizes, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {  mp_header->allocate_many(tag, elem_sizes, n_elements, sizeof_element, chain); }

   //!Deallocates all elements contained in chain.
   //!Never throws.
   void deallocate_many(multiallocation_chain &chain)
   {  mp_header->deallocate_many(chain); }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Marks previously allocated memory as free. Never throws.
   void  deallocate           (void *addr)
   {   if (mp_header) mp_header->deallocate(addr);  }

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, size_type> find  (char_ptr_holder_t name)
   {   return mp_header->template find<T>(name); }

   //!Creates a named object or array in memory
   //!
   //!Allocates and constructs a T object or an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. If an array is being constructed all objects are
   //!created using the same parameters given to this function.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> Throws boost::interprocess::bad_alloc if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and if an
   //!array was being constructed, destructors of created objects are called
   //!before freeing the memory.
   template <class T>
   typename segment_manager::template construct_proxy<T>::type
      construct(char_ptr_holder_t name)
   {   return mp_header->template construct<T>(name);  }

   //!Finds or creates a named object or array in memory
   //!
   //!Tries to find an object with the given name in memory. If
   //!found, returns the pointer to this pointer. If the object is not found,
   //!allocates and constructs a T object or an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. If an array is being constructed all objects are
   //!created using the same parameters given to this function.
   //!
   //!-> Throws boost::interprocess::bad_alloc if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and if an
   //!array was being constructed, destructors of created objects are called
   //!before freeing the memory.
   template <class T>
   typename segment_manager::template construct_proxy<T>::type
      find_or_construct(char_ptr_holder_t name)
   {   return mp_header->template find_or_construct<T>(name);  }

   //!Creates a named object or array in memory
   //!
   //!Allocates and constructs a T object or an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. If an array is being constructed all objects are
   //!created using the same parameters given to this function.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> Returns 0 if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and if an
   //!array was being constructed, destructors of created objects are called
   //!before freeing the memory.
   template <class T>
   typename segment_manager::template construct_proxy<T>::type
      construct(char_ptr_holder_t name, const std::nothrow_t &tag)
   {   return mp_header->template construct<T>(name, tag);  }

   //!Finds or creates a named object or array in memory
   //!
   //!Tries to find an object with the given name in memory. If
   //!found, returns the pointer to this pointer. If the object is not found,
   //!allocates and constructs a T object or an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. If an array is being constructed all objects are
   //!created using the same parameters given to this function.
   //!
   //!-> Returns 0 if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and if an
   //!array was being constructed, destructors of created objects are called
   //!before freeing the memory.
   template <class T>
   typename segment_manager::template construct_proxy<T>::type
      find_or_construct(char_ptr_holder_t name, const std::nothrow_t &tag)
   {   return mp_header->template find_or_construct<T>(name, tag);  }

   //!Creates a named array from iterators in memory
   //!
   //!Allocates and constructs an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. Each element in the array is created using the
   //!objects returned when dereferencing iterators as parameters
   //!and incrementing all iterators for each element.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> Throws boost::interprocess::bad_alloc if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and
   //!destructors of created objects are called before freeing the memory.
   template <class T>
   typename segment_manager::template construct_iter_proxy<T>::type
      construct_it(char_ptr_holder_t name)
   {   return mp_header->template construct_it<T>(name);  }

   //!Finds or creates a named array from iterators in memory
   //!
   //!Tries to find an object with the given name in memory. If
   //!found, returns the pointer to this pointer. If the object is not found,
   //!allocates and constructs an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. Each element in the array is created using the
   //!objects returned when dereferencing iterators as parameters
   //!and incrementing all iterators for each element.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> Throws boost::interprocess::bad_alloc if there is no available memory
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and
   //!destructors of created objects are called before freeing the memory.
   template <class T>
   typename segment_manager::template construct_iter_proxy<T>::type
      find_or_construct_it(char_ptr_holder_t name)
   {   return mp_header->template find_or_construct_it<T>(name);  }

   //!Creates a named array from iterators in memory
   //!
   //!Allocates and constructs an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. Each element in the array is created using the
   //!objects returned when dereferencing iterators as parameters
   //!and incrementing all iterators for each element.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> If there is no available memory, returns 0.
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and
   //!destructors of created objects are called before freeing the memory.*/
   template <class T>
   typename segment_manager::template construct_iter_proxy<T>::type
      construct_it(char_ptr_holder_t name, const std::nothrow_t &tag)
   {   return mp_header->template construct_it<T>(name, tag);  }

   //!Finds or creates a named array from iterators in memory
   //!
   //!Tries to find an object with the given name in memory. If
   //!found, returns the pointer to this pointer. If the object is not found,
   //!allocates and constructs an array of T in memory,
   //!associates this with the given name and returns a pointer to the
   //!created object. Each element in the array is created using the
   //!objects returned when dereferencing iterators as parameters
   //!and incrementing all iterators for each element.
   //!
   //!-> If the name was previously used, returns 0.
   //!
   //!-> If there is no available memory, returns 0.
   //!
   //!-> If T's constructor throws, the function throws that exception.
   //!
   //!Memory is freed automatically if T's constructor throws and
   //!destructors of created objects are called before freeing the memory.*/
   template <class T>
   typename segment_manager::template construct_iter_proxy<T>::type
      find_or_construct_it(char_ptr_holder_t name, const std::nothrow_t &tag)
   {   return mp_header->template find_or_construct_it<T>(name, tag);  }

   //!Calls a functor and guarantees that no new construction, search or
   //!destruction will be executed by any process while executing the object
   //!function call. If the functor throws, this function throws.
   template <class Func>
   void atomic_func(Func &f)
   {   mp_header->atomic_func(f);  }

   //!Tries to call a functor guaranteeing that no new construction, search or
   //!destruction will be executed by any process while executing the object
   //!function call. If the atomic function can't be immediatelly executed
   //!because the internal mutex is already locked, returns false.
   //!If the functor throws, this function throws.
   template <class Func>
   bool try_atomic_func(Func &f)
   {   return mp_header->try_atomic_func(f); }

   //!Destroys a named memory object or array.
   //!
   //!Finds the object with the given name, calls its destructors,
   //!frees used memory and returns true.
   //!
   //!-> If the object is not found, it returns false.
   //!
   //!Exception Handling:
   //!
   //!When deleting a dynamically object or array, the Standard
   //!does not guarantee that dynamically allocated memory, will be released.
   //!Also, when deleting arrays, the Standard doesn't require calling
   //!destructors for the rest of the objects if for one of them the destructor
   //!terminated with an exception.
   //!
   //!Destroying an object:
   //!
   //!If the destructor throws, the memory will be freed and that exception
   //!will be thrown.
   //!
   //!Destroying an array:
   //!
   //!When destroying an array, if a destructor throws, the rest of
   //!destructors are called. If any of these throws, the exceptions are
   //!ignored. The name association will be erased, memory will be freed and
   //!the first exception will be thrown. This guarantees the unlocking of
   //!mutexes and other resources.
   //!
   //!For all theses reasons, classes with throwing destructors are not
   //!recommended.
   template <class T>
   bool destroy(const CharType *name)
   {   return mp_header->template destroy<T>(name); }

   //!Destroys the unique instance of type T
   //!
   //!Calls the destructor, frees used memory and returns true.
   //!
   //!Exception Handling:
   //!
   //!When deleting a dynamically object, the Standard does not
   //!guarantee that dynamically allocated memory will be released.
   //!
   //!Destroying an object:
   //!
   //!If the destructor throws, the memory will be freed and that exception
   //!will be thrown.
   //!
   //!For all theses reasons, classes with throwing destructors are not
   //!recommended for  memory.
   template <class T>
   bool destroy(const unique_instance_t *const )
   {   return mp_header->template destroy<T>(unique_instance);  }

   //!Destroys the object (named, unique, or anonymous)
   //!
   //!Calls the destructor, frees used memory and returns true.
   //!
   //!Exception Handling:
   //!
   //!When deleting a dynamically object, the Standard does not
   //!guarantee that dynamically allocated memory will be released.
   //!
   //!Destroying an object:
   //!
   //!If the destructor throws, the memory will be freed and that exception
   //!will be thrown.
   //!
   //!For all theses reasons, classes with throwing destructors are not
   //!recommended for  memory.
   template <class T>
   void destroy_ptr(const T *ptr)
   {  mp_header->template destroy_ptr<T>(ptr); }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. If ptr points to an unique instance typeid(T).name() is returned.
   template<class T>
   static const char_type *get_instance_name(const T *ptr)
   {  return segment_manager::get_instance_name(ptr);   }

   //!Returns is the type an object created with construct/find_or_construct
   //!functions. Does not throw.
   template<class T>
   static instance_type get_instance_type(const T *ptr)
   {  return segment_manager::get_instance_type(ptr); }

   //!Returns the length of an object created with construct/find_or_construct
   //!functions (1 if is a single element, >=1 if it's an array). Does not throw.
   template<class T>
   static size_type get_instance_length(const T *ptr)
   {  return segment_manager::get_instance_length(ptr); }

   //!Preallocates needed index resources to optimize the
   //!creation of "num" named objects in the  memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_named_objects(size_type num)
   {  mp_header->reserve_named_objects(num);  }

   //!Preallocates needed index resources to optimize the
   //!creation of "num" unique objects in the  memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_unique_objects(size_type num)
   {  mp_header->reserve_unique_objects(num);  }

   //!Calls shrink_to_fit in both named and unique object indexes
   //to try to free unused memory from those indexes.
   void shrink_to_fit_indexes()
   {  mp_header->shrink_to_fit_indexes();  }

   //!Returns the number of named objects stored
   //!in the managed segment.
   size_type get_num_named_objects()
   {  return mp_header->get_num_named_objects();  }

   //!Returns the number of unique objects stored
   //!in the managed segment.
   size_type get_num_unique_objects()
   {  return mp_header->get_num_unique_objects();  }

   //!Returns a constant iterator to the index storing the
   //!named allocations. NOT thread-safe. Never throws.
   const_named_iterator named_begin() const
   {  return mp_header->named_begin(); }

   //!Returns a constant iterator to the end of the index
   //!storing the named allocations. NOT thread-safe. Never throws.
   const_named_iterator named_end() const
   {  return mp_header->named_end(); }

   //!Returns a constant iterator to the index storing the
   //!unique allocations. NOT thread-safe. Never throws.
   const_unique_iterator unique_begin() const
   {  return mp_header->unique_begin(); }

   //!Returns a constant iterator to the end of the index
   //!storing the unique allocations. NOT thread-safe. Never throws.
   const_unique_iterator unique_end() const
   {  return mp_header->unique_end(); }

   //!This is the default allocator to allocate types T
   //!from this managed segment
   template<class T>
   struct allocator
   {
      typedef typename segment_manager::template allocator<T>::type type;
   };

   //!Returns an instance of the default allocator for type T
   //!initialized that allocates memory from this segment manager.
   template<class T>
   typename allocator<T>::type
      get_allocator()
   {   return mp_header->template get_allocator<T>(); }

   //!This is the default deleter to delete types T
   //!from this managed segment.
   template<class T>
   struct deleter
   {
      typedef typename segment_manager::template deleter<T>::type type;
   };

   //!Returns an instance of the default allocator for type T
   //!initialized that allocates memory from this segment manager.
   template<class T>
   typename deleter<T>::type
      get_deleter()
   {   return mp_header->template get_deleter<T>(); }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, size_type> find_no_lock  (char_ptr_holder_t name)
   {   return mp_header->template find_no_lock<T>(name); }
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   protected:
   //!Swaps the segment manager's managed by this managed memory segment.
   //!NOT thread-safe. Never throws.
   void swap(basic_managed_memory_impl &other)
   {  (simple_swap)(mp_header, other.mp_header); }

   private:
   segment_manager *mp_header;
};

template<class BasicManagedMemoryImpl>
class create_open_func
{
   typedef typename BasicManagedMemoryImpl::size_type size_type;

   public:

   create_open_func(BasicManagedMemoryImpl * const frontend, create_enum_t type)
      : m_frontend(frontend), m_type(type){}

   bool operator()(void *addr, std::size_t size, bool created) const
   {
      if( ((m_type == DoOpen)   &&  created) ||
          ((m_type == DoCreate) && !created) ||
          //Check for overflow
          size_type(-1) < size ){
         return false;
      }
      else if(created){
         return m_frontend->create_impl(addr, static_cast<size_type>(size));
      }
      else{
         return m_frontend->open_impl  (addr, static_cast<size_type>(size));
      }
   }

   static std::size_t get_min_size()
   {
      const size_type sz = BasicManagedMemoryImpl::segment_manager::get_min_size();
      if(sz > std::size_t(-1)){
         //The minimum size is not representable by std::size_t
         BOOST_ASSERT(false);
         return std::size_t(-1);
      }
      else{
         return static_cast<std::size_t>(sz);
      }
   }

   private:
   BasicManagedMemoryImpl *m_frontend;
   create_enum_t           m_type;
};

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_MANAGED_MEMORY_IMPL_HPP

