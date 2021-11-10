//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP
#define BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/core/no_exceptions_support.hpp>
#include <boost/interprocess/detail/type_traits.hpp>

#include <boost/interprocess/detail/transform_iterator.hpp>

#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/nothrow.hpp>
#include <boost/interprocess/detail/segment_manager_helper.hpp>
#include <boost/interprocess/detail/named_proxy.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
// container/detail
#include <boost/container/detail/minimal_char_traits_header.hpp>
#include <boost/container/detail/placement_new.hpp>
// std
#include <cstddef>   //std::size_t
#include <boost/intrusive/detail/minimal_pair_header.hpp>
#include <boost/assert.hpp>
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

//!\file
//!Describes the object placed in a memory segment that provides
//!named object allocation capabilities for single-segment and
//!multi-segment allocations.

namespace boost{
namespace interprocess{

//!This object is the public base class of segment manager.
//!This class only depends on the memory allocation algorithm
//!and implements all the allocation features not related
//!to named or unique objects.
//!
//!Storing a reference to segment_manager forces
//!the holder class to be dependent on index types and character types.
//!When such dependence is not desirable and only anonymous and raw
//!allocations are needed, segment_manager_base is the correct answer.
template<class MemoryAlgorithm>
class segment_manager_base
   :  private MemoryAlgorithm
{
   public:
   typedef segment_manager_base<MemoryAlgorithm> segment_manager_base_type;
   typedef typename MemoryAlgorithm::void_pointer  void_pointer;
   typedef typename MemoryAlgorithm::mutex_family  mutex_family;
   typedef MemoryAlgorithm memory_algorithm;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Experimental. Don't use
   typedef typename MemoryAlgorithm::multiallocation_chain    multiallocation_chain;
   typedef typename MemoryAlgorithm::difference_type  difference_type;
   typedef typename MemoryAlgorithm::size_type        size_type;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!This constant indicates the payload size
   //!associated with each allocation of the memory algorithm
   static const size_type PayloadPerAllocation = MemoryAlgorithm::PayloadPerAllocation;

   //!Constructor of the segment_manager_base
   //!
   //!"size" is the size of the memory segment where
   //!the basic segment manager is being constructed.
   //!
   //!"reserved_bytes" is the number of bytes
   //!after the end of the memory algorithm object itself
   //!that the memory algorithm will exclude from
   //!dynamic allocation
   //!
   //!Can throw
   segment_manager_base(size_type sz, size_type reserved_bytes)
      :  MemoryAlgorithm(sz, reserved_bytes)
   {
      BOOST_ASSERT((sizeof(segment_manager_base<MemoryAlgorithm>) == sizeof(MemoryAlgorithm)));
   }

   //!Returns the size of the memory
   //!segment
   size_type get_size() const
   {  return MemoryAlgorithm::get_size();  }

   //!Returns the number of free bytes of the memory
   //!segment
   size_type get_free_memory() const
   {  return MemoryAlgorithm::get_free_memory();  }

   //!Obtains the minimum size needed by
   //!the segment manager
   static size_type get_min_size (size_type size)
   {  return MemoryAlgorithm::get_min_size(size);  }

   //!Allocates nbytes bytes. This function is only used in
   //!single-segment management. Never throws
   void * allocate (size_type nbytes, const std::nothrow_t &)
   {  return MemoryAlgorithm::allocate(nbytes);   }

   //!Returns a reference to the internal memory algorithm.
   //!This function is useful for custom memory algorithms that
   //!need additional configuration options after construction. Never throws.
   //!This function should be only used by advanced users.
   MemoryAlgorithm &get_memory_algorithm()
   {  return static_cast<MemoryAlgorithm&>(*this);   }

   //!Returns a const reference to the internal memory algorithm.
   //!This function is useful for custom memory algorithms that
   //!need additional configuration options after construction. Never throws.
   //!This function should be only used by advanced users.
   const MemoryAlgorithm &get_memory_algorithm() const
   {  return static_cast<const MemoryAlgorithm&>(*this);   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Experimental. Dont' use.
   //!Allocates n_elements of elem_bytes bytes.
   //!Throws bad_alloc on failure. chain.size() is not increased on failure.
   void allocate_many(size_type elem_bytes, size_type n_elements, multiallocation_chain &chain)
   {
      size_type prev_size = chain.size();
      MemoryAlgorithm::allocate_many(elem_bytes, n_elements, chain);
      if(!elem_bytes || chain.size() == prev_size){
         throw bad_alloc();
      }
   }

   //!Allocates n_elements, each one of element_lengths[i]*sizeof_element bytes.
   //!Throws bad_alloc on failure. chain.size() is not increased on failure.
   void allocate_many(const size_type *element_lengths, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {
      size_type prev_size = chain.size();
      MemoryAlgorithm::allocate_many(element_lengths, n_elements, sizeof_element, chain);
      if(!sizeof_element || chain.size() == prev_size){
         throw bad_alloc();
      }
   }

   //!Allocates n_elements of elem_bytes bytes.
   //!Non-throwing version. chain.size() is not increased on failure.
   void allocate_many(const std::nothrow_t &, size_type elem_bytes, size_type n_elements, multiallocation_chain &chain)
   {  MemoryAlgorithm::allocate_many(elem_bytes, n_elements, chain); }

   //!Allocates n_elements, each one of
   //!element_lengths[i]*sizeof_element bytes.
   //!Non-throwing version. chain.size() is not increased on failure.
   void allocate_many(const std::nothrow_t &, const size_type *elem_sizes, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {  MemoryAlgorithm::allocate_many(elem_sizes, n_elements, sizeof_element, chain); }

   //!Deallocates all elements contained in chain.
   //!Never throws.
   void deallocate_many(multiallocation_chain &chain)
   {  MemoryAlgorithm::deallocate_many(chain); }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Allocates nbytes bytes. Throws boost::interprocess::bad_alloc
   //!on failure
   void * allocate(size_type nbytes)
   {
      void * ret = MemoryAlgorithm::allocate(nbytes);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   //!Allocates nbytes bytes. This function is only used in
   //!single-segment management. Never throws
   void * allocate_aligned (size_type nbytes, size_type alignment, const std::nothrow_t &)
   {  return MemoryAlgorithm::allocate_aligned(nbytes, alignment);   }

   //!Allocates nbytes bytes. This function is only used in
   //!single-segment management. Throws bad_alloc when fails
   void * allocate_aligned(size_type nbytes, size_type alignment)
   {
      void * ret = MemoryAlgorithm::allocate_aligned(nbytes, alignment);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   template<class T>
   T *allocation_command  (boost::interprocess::allocation_type command, size_type limit_size,
                           size_type &prefer_in_recvd_out_size, T *&reuse)
   {
      T *ret = MemoryAlgorithm::allocation_command
         (command | boost::interprocess::nothrow_allocation, limit_size, prefer_in_recvd_out_size, reuse);
      if(!(command & boost::interprocess::nothrow_allocation) && !ret)
         throw bad_alloc();
      return ret;
   }

   void *raw_allocation_command  (boost::interprocess::allocation_type command,   size_type limit_objects,
                           size_type &prefer_in_recvd_out_size, void *&reuse, size_type sizeof_object = 1)
   {
      void *ret = MemoryAlgorithm::raw_allocation_command
         ( command | boost::interprocess::nothrow_allocation, limit_objects,
           prefer_in_recvd_out_size, reuse, sizeof_object);
      if(!(command & boost::interprocess::nothrow_allocation) && !ret)
         throw bad_alloc();
      return ret;
   }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Deallocates the bytes allocated with allocate/allocate_many()
   //!pointed by addr
   void   deallocate          (void *addr)
   {  MemoryAlgorithm::deallocate(addr);   }

   //!Increases managed memory in extra_size bytes more. This only works
   //!with single-segment management.
   void grow(size_type extra_size)
   {  MemoryAlgorithm::grow(extra_size);   }

   //!Decreases managed memory to the minimum. This only works
   //!with single-segment management.
   void shrink_to_fit()
   {  MemoryAlgorithm::shrink_to_fit();   }

   //!Returns the result of "all_memory_deallocated()" function
   //!of the used memory algorithm
   bool all_memory_deallocated()
   {   return MemoryAlgorithm::all_memory_deallocated(); }

   //!Returns the result of "check_sanity()" function
   //!of the used memory algorithm
   bool check_sanity()
   {   return MemoryAlgorithm::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated)
   //!of the memory algorithm
   void zero_free_memory()
   {   MemoryAlgorithm::zero_free_memory(); }

   //!Returns the size of the buffer previously allocated pointed by ptr
   size_type size(const void *ptr) const
   {   return MemoryAlgorithm::size(ptr); }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   protected:
   void * prot_anonymous_construct
      (size_type num, bool dothrow, ipcdetail::in_place_interface &table)
   {
      typedef ipcdetail::block_header<size_type> block_header_t;
      block_header_t block_info (  size_type(table.size*num)
                                 , size_type(table.alignment)
                                 , anonymous_type
                                 , 1
                                 , 0);

      //Allocate memory
      void *ptr_struct = this->allocate(block_info.total_size(), nothrow<>::get());

      //Check if there is enough memory
      if(!ptr_struct){
         if(dothrow){
            throw bad_alloc();
         }
         else{
            return 0;
         }
      }

      //Build scoped ptr to avoid leaks with constructor exception
      ipcdetail::mem_algo_deallocator<MemoryAlgorithm> mem(ptr_struct, *this);

      //Now construct the header
      block_header_t * hdr = ::new(ptr_struct, boost_container_new_t()) block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Now call constructors
      ipcdetail::array_construct(ptr, num, table);

      //All constructors successful, we don't want erase memory
      mem.release();
      return ptr;
   }

   //!Calls the destructor and makes an anonymous deallocate
   void prot_anonymous_destroy(const void *object, ipcdetail::in_place_interface &table)
   {

      //Get control data from associated with this object
      typedef ipcdetail::block_header<size_type> block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(object, table.size, table.alignment);

      //-------------------------------
      //scoped_lock<rmutex> guard(m_header);
      //-------------------------------

      if(ctrl_data->alloc_type() != anonymous_type){
         //This is not an anonymous object, the pointer is wrong!
         BOOST_ASSERT(0);
      }

      //Call destructors and free memory
      //Build scoped ptr to avoid leaks with destructor exception
      std::size_t destroyed = 0;
     table.destroy_n(const_cast<void*>(object), ctrl_data->m_value_bytes/table.size, destroyed);
      this->deallocate(ctrl_data);
   }
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

//!This object is placed in the beginning of memory segment and
//!implements the allocation (named or anonymous) of portions
//!of the segment. This object contains two indexes that
//!maintain an association between a name and a portion of the segment.
//!
//!The first index contains the mappings for normal named objects using the
//!char type specified in the template parameter.
//!
//!The second index contains the association for unique instances. The key will
//!be the const char * returned from type_info.name() function for the unique
//!type to be constructed.
//!
//!segment_manager<CharType, MemoryAlgorithm, IndexType> inherits publicly
//!from segment_manager_base<MemoryAlgorithm> and inherits from it
//!many public functions related to anonymous object and raw memory allocation.
//!See segment_manager_base reference to know about those functions.
template<class CharType
        ,class MemoryAlgorithm
        ,template<class IndexConfig> class IndexType>
class segment_manager
   :  public segment_manager_base<MemoryAlgorithm>
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   segment_manager();
   segment_manager(const segment_manager &);
   segment_manager &operator=(const segment_manager &);
   typedef segment_manager_base<MemoryAlgorithm> segment_manager_base_t;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   typedef MemoryAlgorithm                                  memory_algorithm;
   typedef typename segment_manager_base_t::void_pointer    void_pointer;
   typedef typename segment_manager_base_t::size_type       size_type;
   typedef typename segment_manager_base_t::difference_type difference_type;
   typedef CharType                                         char_type;

   typedef segment_manager_base<MemoryAlgorithm>   segment_manager_base_type;

   static const size_type PayloadPerAllocation = segment_manager_base_t::PayloadPerAllocation;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef ipcdetail::block_header<size_type> block_header_t;
   typedef ipcdetail::index_config<CharType, MemoryAlgorithm>  index_config_named;
   typedef ipcdetail::index_config<char, MemoryAlgorithm>      index_config_unique;
   typedef IndexType<index_config_named>                    index_type;
   typedef ipcdetail::bool_<is_intrusive_index<index_type>::value >    is_intrusive_t;
   typedef ipcdetail::bool_<is_node_index<index_type>::value>          is_node_index_t;

   public:
   typedef IndexType<index_config_named>                    named_index_t;
   typedef IndexType<index_config_unique>                   unique_index_t;
   typedef ipcdetail::char_ptr_holder<CharType>                char_ptr_holder_t;
   typedef ipcdetail::segment_manager_iterator_transform
      <typename named_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   named_transform;

   typedef ipcdetail::segment_manager_iterator_transform
      <typename unique_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   unique_transform;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   typedef typename segment_manager_base_t::mutex_family       mutex_family;

   typedef transform_iterator
      <typename named_index_t::const_iterator, named_transform> const_named_iterator;
   typedef transform_iterator
      <typename unique_index_t::const_iterator, unique_transform> const_unique_iterator;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_proxy
   {
      typedef ipcdetail::named_proxy<segment_manager, T, false>   type;
   };

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_iter_proxy
   {
      typedef ipcdetail::named_proxy<segment_manager, T, true>   type;
   };

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Constructor of the segment manager
   //!"size" is the size of the memory segment where
   //!the segment manager is being constructed.
   //!Can throw
   explicit segment_manager(size_type segment_size)
      :  segment_manager_base_t(segment_size, priv_get_reserved_bytes())
      ,  m_header(static_cast<segment_manager_base_t*>(get_this_pointer()))
   {
      (void) anonymous_instance;   (void) unique_instance;
      //Check EBO is applied, it's required
      const void * const this_addr = this;
      const void *const segm_addr  = static_cast<segment_manager_base_t*>(this);
      (void)this_addr;  (void)segm_addr;
      BOOST_ASSERT( this_addr == segm_addr);
      const std::size_t void_ptr_alignment = boost::move_detail::alignment_of<void_pointer>::value; (void)void_ptr_alignment;
      BOOST_ASSERT((0 == (std::size_t)this_addr % boost::move_detail::alignment_of<segment_manager>::value));
   }

   //!Tries to find a previous named/unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, size_type> find  (char_ptr_holder_t name)
   {  return this->priv_find_impl<T>(name, true);  }

   //!Tries to find a previous named/unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0. This search is not mutex-protected!
   //!Use it only inside atomic_func() calls, where the internal mutex
   //!is guaranteed to be locked.
   template <class T>
   std::pair<T*, size_type> find_no_lock  (char_ptr_holder_t name)
   {  return this->priv_find_impl<T>(name, false);  }

   //!Returns throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type
      construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type find_or_construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type
      construct(char_ptr_holder_t name, const std::nothrow_t &)
   {  return typename construct_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct"
   //!proxy object
   template <class T>
   typename construct_proxy<T>::type
      find_or_construct(char_ptr_holder_t name, const std::nothrow_t &)
   {  return typename construct_proxy<T>::type (this, name, true, false);  }

   //!Returns throwing "construct from iterators" proxy object
   template <class T>
   typename construct_iter_proxy<T>::type
      construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type
      find_or_construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type
      construct_it(char_ptr_holder_t name, const std::nothrow_t &)
   {  return typename construct_iter_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type
      find_or_construct_it(char_ptr_holder_t name, const std::nothrow_t &)
   {  return typename construct_iter_proxy<T>::type (this, name, true, false);  }

   //!Calls object function blocking recursive interprocess_mutex and guarantees that
   //!no new named_alloc or destroy will be executed by any process while
   //!executing the object function call
   template <class Func>
   void atomic_func(Func &f)
   {  scoped_lock<rmutex> guard(m_header);  f();  }

   //!Tries to calls a functor guaranteeing that no new construction, search or
   //!destruction will be executed by any process while executing the object
   //!function call. If the atomic function can't be immediatelly executed
   //!because the internal mutex is already locked, returns false.
   //!If the functor throws, this function throws.
   template <class Func>
   bool try_atomic_func(Func &f)
   {
      scoped_lock<rmutex> guard(m_header, try_to_lock);
      if(guard){
         f();
         return true;
      }
      else{
         return false;
      }
   }

   //!Destroys a previously created named/unique instance.
   //!Returns false if the object was not present.
   template <class T>
   bool destroy(char_ptr_holder_t name)
   {
      BOOST_ASSERT(!name.is_anonymous());
      ipcdetail::placement_destroy<T> dtor;

      if(name.is_unique()){
         return this->priv_generic_named_destroy<char>
            ( typeid(T).name(), m_header.m_unique_index , dtor, is_intrusive_t());
      }
      else{
         return this->priv_generic_named_destroy<CharType>
            ( name.get(), m_header.m_named_index, dtor, is_intrusive_t());
      }
   }

   //!Destroys an anonymous, unique or named object
   //!using its address
   template <class T>
   void destroy_ptr(const T *p)
   {
      //If T is void transform it to char
      typedef typename ipcdetail::char_if_void<T>::type data_t;
      ipcdetail::placement_destroy<data_t> dtor;
      priv_destroy_ptr(p, dtor);
   }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   static const CharType *get_instance_name(const T *ptr)
   { return priv_get_instance_name(block_header_t::block_header_from_value(ptr));  }

   //!Returns the length of an object created with construct/find_or_construct
   //!functions. Does not throw.
   template<class T>
   static size_type get_instance_length(const T *ptr)
   {  return priv_get_instance_length(block_header_t::block_header_from_value(ptr), sizeof(T));  }

   //!Returns is the the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   static instance_type get_instance_type(const T *ptr)
   {  return priv_get_instance_type(block_header_t::block_header_from_value(ptr));  }

   //!Preallocates needed index resources to optimize the
   //!creation of "num" named objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_named_objects(size_type num)
   {
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.reserve(num);
   }

   //!Preallocates needed index resources to optimize the
   //!creation of "num" unique objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_unique_objects(size_type num)
   {
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_unique_index.reserve(num);
   }

   //!Calls shrink_to_fit in both named and unique object indexes
   //!to try to free unused memory from those indexes.
   void shrink_to_fit_indexes()
   {
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.shrink_to_fit();
      m_header.m_unique_index.shrink_to_fit();
   }

   //!Returns the number of named objects stored in
   //!the segment.
   size_type get_num_named_objects()
   {
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_named_index.size();
   }

   //!Returns the number of unique objects stored in
   //!the segment.
   size_type get_num_unique_objects()
   {
      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_unique_index.size();
   }

   //!Obtains the minimum size needed by the
   //!segment manager
   static size_type get_min_size()
   {  return segment_manager_base_t::get_min_size(priv_get_reserved_bytes());  }

   //!Returns a constant iterator to the beginning of the information about
   //!the named allocations performed in this segment manager
   const_named_iterator named_begin() const
   {
      return (make_transform_iterator)
         (m_header.m_named_index.begin(), named_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //!the named allocations performed in this segment manager
   const_named_iterator named_end() const
   {
      return (make_transform_iterator)
         (m_header.m_named_index.end(), named_transform());
   }

   //!Returns a constant iterator to the beginning of the information about
   //!the unique allocations performed in this segment manager
   const_unique_iterator unique_begin() const
   {
      return (make_transform_iterator)
         (m_header.m_unique_index.begin(), unique_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //!the unique allocations performed in this segment manager
   const_unique_iterator unique_end() const
   {
      return (make_transform_iterator)
         (m_header.m_unique_index.end(), unique_transform());
   }

   //!This is the default allocator to allocate types T
   //!from this managed segment
   template<class T>
   struct allocator
   {
      typedef boost::interprocess::allocator<T, segment_manager> type;
   };

   //!Returns an instance of the default allocator for type T
   //!initialized that allocates memory from this segment manager.
   template<class T>
   typename allocator<T>::type
      get_allocator()
   {   return typename allocator<T>::type(this); }

   //!This is the default deleter to delete types T
   //!from this managed segment.
   template<class T>
   struct deleter
   {
      typedef boost::interprocess::deleter<T, segment_manager> type;
   };

   //!Returns an instance of the default deleter for type T
   //!that will delete an object constructed in this segment manager.
   template<class T>
   typename deleter<T>::type
      get_deleter()
   {   return typename deleter<T>::type(this); }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Generic named/anonymous new function. Offers all the possibilities,
   //!such as throwing, search before creating, and the constructor is
   //!encapsulated in an object function.
   template<class T>
   T *generic_construct(const CharType *name,
                        size_type num,
                         bool try2find,
                         bool dothrow,
                         ipcdetail::in_place_interface &table)
   {
      return static_cast<T*>
         (priv_generic_construct(name, num, try2find, dothrow, table));
   }

   private:
   //!Tries to find a previous named allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, size_type> priv_find_impl (const CharType* name, bool lock)
   {
      //The name can't be null, no anonymous object can be found by name
      BOOST_ASSERT(name != 0);
      ipcdetail::placement_destroy<T> table;
      size_type sz;
      void *ret;

      if(name == reinterpret_cast<const CharType*>(-1)){
         ret = priv_generic_find<char> (typeid(T).name(), m_header.m_unique_index, table, sz, is_intrusive_t(), lock);
      }
      else{
         ret = priv_generic_find<CharType> (name, m_header.m_named_index, table, sz, is_intrusive_t(), lock);
      }
      return std::pair<T*, size_type>(static_cast<T*>(ret), sz);
   }

   //!Tries to find a previous unique allocation. Returns the address
   //!and the object count. On failure the first member of the
   //!returned pair is 0.
   template <class T>
   std::pair<T*, size_type> priv_find_impl (const ipcdetail::unique_instance_t* name, bool lock)
   {
      ipcdetail::placement_destroy<T> table;
      size_type size;
      void *ret = priv_generic_find<char>(name, m_header.m_unique_index, table, size, is_intrusive_t(), lock);
      return std::pair<T*, size_type>(static_cast<T*>(ret), size);
   }

   void *priv_generic_construct
      (const CharType *name, size_type num, bool try2find, bool dothrow, ipcdetail::in_place_interface &table)
   {
      void *ret;
      //Security overflow check
      if(num > ((std::size_t)-1)/table.size){
         if(dothrow)
            throw bad_alloc();
         else
            return 0;
      }
      if(name == 0){
         ret = this->prot_anonymous_construct(num, dothrow, table);
      }
      else if(name == reinterpret_cast<const CharType*>(-1)){
         ret = this->priv_generic_named_construct<char>
            (unique_type, table.type_name, num, try2find, dothrow, table, m_header.m_unique_index, is_intrusive_t());
      }
      else{
         ret = this->priv_generic_named_construct<CharType>
            (named_type, name, num, try2find, dothrow, table, m_header.m_named_index, is_intrusive_t());
      }
      return ret;
   }

   void priv_destroy_ptr(const void *ptr, ipcdetail::in_place_interface &dtor)
   {
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr, dtor.size, dtor.alignment);
      switch(ctrl_data->alloc_type()){
         case anonymous_type:
            this->prot_anonymous_destroy(ptr, dtor);
         break;

         case named_type:
            this->priv_generic_named_destroy<CharType>
               (ctrl_data, m_header.m_named_index, dtor, is_node_index_t());
         break;

         case unique_type:
            this->priv_generic_named_destroy<char>
               (ctrl_data, m_header.m_unique_index, dtor, is_node_index_t());
         break;

         default:
            //This type is unknown, bad pointer passed to this function!
            BOOST_ASSERT(0);
         break;
      }
   }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   static const CharType *priv_get_instance_name(block_header_t *ctrl_data)
   {
      boost::interprocess::allocation_type type = ctrl_data->alloc_type();
      if(type == anonymous_type){
         BOOST_ASSERT((type == anonymous_type && ctrl_data->m_num_char == 0) ||
                (type == unique_type    && ctrl_data->m_num_char != 0) );
         return 0;
      }
      CharType *name = static_cast<CharType*>(ctrl_data->template name<CharType>());

      //Sanity checks
      BOOST_ASSERT(ctrl_data->sizeof_char() == sizeof(CharType));
      BOOST_ASSERT(ctrl_data->m_num_char == std::char_traits<CharType>::length(name));
      return name;
   }

   static size_type priv_get_instance_length(block_header_t *ctrl_data, size_type sizeofvalue)
   {
      //Get header
      BOOST_ASSERT((ctrl_data->value_bytes() %sizeofvalue) == 0);
      return ctrl_data->value_bytes()/sizeofvalue;
   }

   //!Returns is the the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   static instance_type priv_get_instance_type(block_header_t *ctrl_data)
   {
      //Get header
      BOOST_ASSERT((instance_type)ctrl_data->alloc_type() < max_allocation_type);
      return (instance_type)ctrl_data->alloc_type();
   }

   static size_type priv_get_reserved_bytes()
   {
      //Get the number of bytes until the end of (*this)
      //beginning in the end of the segment_manager_base_t base.
      return sizeof(segment_manager) - sizeof(segment_manager_base_t);
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name,
       IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
       ipcdetail::in_place_interface &table,
       size_type &length, ipcdetail::true_ is_intrusive, bool use_lock)
   {
      (void)is_intrusive;
      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >         index_type_t;
      typedef typename index_type_t::iterator           index_it;

      //-------------------------------
      scoped_lock<rmutex> guard(priv_get_lock(use_lock));
      //-------------------------------
      //Find name in index
      ipcdetail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = it->get_block_header();

         //Sanity check
         BOOST_ASSERT((ctrl_data->m_value_bytes % table.size) == 0);
         BOOST_ASSERT(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name,
       IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
       ipcdetail::in_place_interface &table,
       size_type &length, ipcdetail::false_ is_intrusive, bool use_lock)
   {
      (void)is_intrusive;
      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::key_type        key_type;
      typedef typename index_type::iterator        index_it;

      //-------------------------------
      scoped_lock<rmutex> guard(priv_get_lock(use_lock));
      //-------------------------------
      //Find name in index
      index_it it = index.find(key_type(name, std::char_traits<CharT>::length(name)));

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                    (ipcdetail::to_raw_pointer(it->second.m_ptr));

         //Sanity check
         BOOST_ASSERT((ctrl_data->m_value_bytes % table.size) == 0);
         BOOST_ASSERT(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
      ipcdetail::in_place_interface &table, ipcdetail::true_ is_node_index)
   {
      (void)is_node_index;
      typedef typename IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >::iterator index_it;

      index_it *ihdr = block_header_t::template to_first_header<index_it>(block_header);
      return this->priv_generic_named_destroy_impl<CharT>(*ihdr, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
      ipcdetail::in_place_interface &table,
      ipcdetail::false_ is_node_index)
   {
      (void)is_node_index;
      CharT *name = static_cast<CharT*>(block_header->template name<CharT>());
      return this->priv_generic_named_destroy<CharT>(name, index, table, is_intrusive_t());
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name,
                                   IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
                                   ipcdetail::in_place_interface &table, ipcdetail::true_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >         index_type_t;
      typedef typename index_type_t::iterator           index_it;
      typedef typename index_type_t::value_type         intrusive_value_type;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      ipcdetail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //BOOST_ASSERT(0);
         return false;
      }

      block_header_t *ctrl_data = it->get_block_header();
      intrusive_value_type *iv = intrusive_value_type::get_intrusive_value_type(ctrl_data);
      void *memory = iv;
      void *values = ctrl_data->value();
      std::size_t num = ctrl_data->m_value_bytes/table.size;

      //Sanity check
      BOOST_ASSERT((ctrl_data->m_value_bytes % table.size) == 0);
      BOOST_ASSERT(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the headers
      ctrl_data->~block_header_t();
      iv->~intrusive_value_type();

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name,
                                   IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
                                   ipcdetail::in_place_interface &table,
                                   ipcdetail::false_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >            index_type;
      typedef typename index_type::iterator              index_it;
      typedef typename index_type::key_type              key_type;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Try to find the name in the index
      index_it it = index.find(key_type (name,
                                     std::char_traits<CharT>::length(name)));

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //BOOST_ASSERT(0);
         return false;
      }
      return this->priv_generic_named_destroy_impl<CharT>(it, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy_impl
      (const typename IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >::iterator &it,
      IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index,
      ipcdetail::in_place_interface &table)
   {
      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::iterator        index_it;

      //Get allocation parameters
      block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                 (ipcdetail::to_raw_pointer(it->second.m_ptr));
      char *stored_name       = static_cast<char*>(static_cast<void*>(const_cast<CharT*>(it->first.name())));
      (void)stored_name;

      //Check if the distance between the name pointer and the memory pointer
      //is correct (this can detect incorrect type in destruction)
      std::size_t num = ctrl_data->m_value_bytes/table.size;
      void *values = ctrl_data->value();

      //Sanity check
      BOOST_ASSERT((ctrl_data->m_value_bytes % table.size) == 0);
      BOOST_ASSERT(static_cast<void*>(stored_name) == static_cast<void*>(ctrl_data->template name<CharT>()));
      BOOST_ASSERT(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the header
      ctrl_data->~block_header_t();

      void *memory;
      if(is_node_index_t::value){
         index_it *ihdr = block_header_t::template
            to_first_header<index_it>(ctrl_data);
         ihdr->~index_it();
         memory = ihdr;
      }
      else{
         memory = ctrl_data;
      }

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template<class CharT>
   void * priv_generic_named_construct
      (unsigned char type, const CharT *name, size_type num, bool try2find,
      bool dothrow, ipcdetail::in_place_interface &table, 
      IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index, ipcdetail::true_ is_intrusive)
   {
      (void)is_intrusive;
     std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( size_type(table.size*num)
                                 , size_type(table.alignment)
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >            index_type_t;
      typedef typename index_type_t::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;

      typename index_type_t::insert_commit_data   commit_data;
      typedef typename index_type_t::value_type   intrusive_value_type;

      BOOST_TRY{
         ipcdetail::intrusive_compare_key<CharT> key(name, namelen);
         insert_ret = index.insert_check(key, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            return it->get_block_header()->value();
         }
         if(dothrow){
            throw interprocess_exception(already_exists_error);
         }
         else{
            return 0;
         }
      }

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr;

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate
            (block_info.template total_size_with_header<intrusive_value_type>());
      }
      else{
         buffer_ptr = this->allocate
            (block_info.template total_size_with_header<intrusive_value_type>(), nothrow<>::get());
         if(!buffer_ptr)
            return 0;
      }

      //Now construct the intrusive hook plus the header
      intrusive_value_type * intrusive_hdr = ::new(buffer_ptr, boost_container_new_t()) intrusive_value_type();
      block_header_t * hdr = ::new(intrusive_hdr->get_block_header(), boost_container_new_t())block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      BOOST_TRY{
         //Now commit the insertion using previous context data
         it = index.insert_commit(*intrusive_hdr, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW
         return 0;
      }
      BOOST_CATCH_END

      //Avoid constructions if constructor is trivial
      //Build scoped ptr to avoid leaks with constructor exception
      ipcdetail::mem_algo_deallocator<segment_manager_base_type> mem
         (buffer_ptr, *static_cast<segment_manager_base_type*>(this));

      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong. This will be executed *before*
      //the memory allocation as the intrusive value is built in that
      //memory
      value_eraser<index_type_t> v_eraser(index, it);

      //Construct array, this can throw
      ipcdetail::array_construct(ptr, num, table);

      //Release rollbacks since construction was successful
      v_eraser.release();
      mem.release();
      return ptr;
   }

   //!Generic named new function for
   //!named functions
   template<class CharT>
   void * priv_generic_named_construct
      (unsigned char type, const CharT *name, size_type num, bool try2find, bool dothrow,
      ipcdetail::in_place_interface &table, 
      IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> > &index, ipcdetail::false_ is_intrusive)
   {
      (void)is_intrusive;
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( size_type(table.size*num)
                                 , size_type(table.alignment)
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<ipcdetail::index_config<CharT, MemoryAlgorithm> >            index_type_t;
      typedef typename index_type_t::key_type              key_type;
      typedef typename index_type_t::mapped_type           mapped_type;
      typedef typename index_type_t::value_type            value_type;
      typedef typename index_type_t::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;
      BOOST_TRY{
         insert_ret = index.insert(value_type(key_type (name, namelen), mapped_type(0)));
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW;
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            block_header_t *hdr = static_cast<block_header_t*>
               (ipcdetail::to_raw_pointer(it->second.m_ptr));
            return hdr->value();
         }
         return 0;
      }
      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      value_eraser<index_type_t> v_eraser(index, it);

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr;
      block_header_t * hdr;

      //Allocate and construct the headers
      if(is_node_index_t::value){
         size_type total_size = block_info.template total_size_with_header<index_it>();
         if(dothrow){
            buffer_ptr = this->allocate(total_size);
         }
         else{
            buffer_ptr = this->allocate(total_size, nothrow<>::get());
            if(!buffer_ptr)
               return 0;
         }
         index_it *idr = ::new(buffer_ptr, boost_container_new_t()) index_it(it);
         hdr = block_header_t::template from_first_header<index_it>(idr);
      }
      else{
         if(dothrow){
            buffer_ptr = this->allocate(block_info.total_size());
         }
         else{
            buffer_ptr = this->allocate(block_info.total_size(), nothrow<>::get());
            if(!buffer_ptr)
               return 0;
         }
         hdr = static_cast<block_header_t*>(buffer_ptr);
      }

      hdr = ::new(hdr, boost_container_new_t())block_header_t(block_info);
      void *ptr = 0; //avoid gcc warning
      ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      //Do the ugly cast, please mama, forgive me!
      //This new key points to an identical string, so it must have the
      //same position than the overwritten key according to the predicate
      const_cast<key_type &>(it->first).name(name_ptr);
      it->second.m_ptr  = hdr;

      //Build scoped ptr to avoid leaks with constructor exception
      ipcdetail::mem_algo_deallocator<segment_manager_base_type> mem
         (buffer_ptr, *static_cast<segment_manager_base_type*>(this));

      //Construct array, this can throw
      ipcdetail::array_construct(ptr, num, table);

      //All constructors successful, we don't want to release memory
      mem.release();

      //Release node v_eraser since construction was successful
      v_eraser.release();
      return ptr;
   }

   private:
   //!Returns the this pointer
   segment_manager *get_this_pointer()
   {  return this;  }

   typedef typename MemoryAlgorithm::mutex_family::recursive_mutex_type   rmutex;

   scoped_lock<rmutex> priv_get_lock(bool use_lock)
   {
      scoped_lock<rmutex> local(m_header, defer_lock);
      if(use_lock){
         local.lock();
      }
      return scoped_lock<rmutex>(boost::move(local));
   }

   //!This struct includes needed data and derives from
   //!rmutex to allow EBO when using null interprocess_mutex
   struct header_t
      :  public rmutex
   {
      named_index_t           m_named_index;
      unique_index_t          m_unique_index;

      header_t(segment_manager_base_t *segment_mngr_base)
         :  m_named_index (segment_mngr_base)
         ,  m_unique_index(segment_mngr_base)
      {}
   }  m_header;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};


}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

