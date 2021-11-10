///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_ALLOCATOR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/intrusive/pointer_traits.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/containers/allocation_type.hpp>
#include <boost/container/detail/multiallocation_chain.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/container/detail/placement_new.hpp>

#include <cstddef>
#include <stdexcept>

//!\file
//!Describes an allocator that allocates portions of fixed size
//!memory buffer (shared memory, mapped file...)

namespace boost {
namespace interprocess {


//!An STL compatible allocator that uses a segment manager as
//!memory source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
template<class T, class SegmentManager>
class allocator
{
   public:
   //Segment manager
   typedef SegmentManager                                segment_manager;
   typedef typename SegmentManager::void_pointer         void_pointer;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   //Self type
   typedef allocator<T, SegmentManager>   self_t;

   //Pointer to void
   typedef typename segment_manager::void_pointer  aux_pointer_t;

   //Typedef to const void pointer
   typedef typename boost::intrusive::
      pointer_traits<aux_pointer_t>::template
         rebind_pointer<const void>::type          cvoid_ptr;

   //Pointer to the allocator
   typedef typename boost::intrusive::
      pointer_traits<cvoid_ptr>::template
         rebind_pointer<segment_manager>::type          alloc_ptr_t;

   //Not assignable from related allocator
   template<class T2, class SegmentManager2>
   allocator& operator=(const allocator<T2, SegmentManager2>&);

   //Not assignable from other allocator
   allocator& operator=(const allocator&);

   //Pointer to the allocator
   alloc_ptr_t mp_mngr;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   typedef T                                    value_type;
   typedef typename boost::intrusive::
      pointer_traits<cvoid_ptr>::template
         rebind_pointer<T>::type                pointer;
   typedef typename boost::intrusive::
      pointer_traits<pointer>::template
         rebind_pointer<const T>::type          const_pointer;
   typedef typename ipcdetail::add_reference
                     <value_type>::type         reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type   const_reference;
   typedef typename segment_manager::size_type               size_type;
   typedef typename segment_manager::difference_type         difference_type;

   typedef boost::interprocess::version_type<allocator, 2>   version;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Experimental. Don't use.
   typedef boost::container::dtl::transform_multiallocation_chain
      <typename SegmentManager::multiallocation_chain, T>multiallocation_chain;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Obtains an allocator that allocates
   //!objects of type T2
   template<class T2>
   struct rebind
   {
      typedef allocator<T2, SegmentManager>     other;
   };

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const
   {  return ipcdetail::to_raw_pointer(mp_mngr);   }

   //!Constructor from the segment manager.
   //!Never throws
   allocator(segment_manager *segment_mngr)
      : mp_mngr(segment_mngr) { }

   //!Constructor from other allocator.
   //!Never throws
   allocator(const allocator &other)
      : mp_mngr(other.get_segment_manager()){ }

   //!Constructor from related allocator.
   //!Never throws
   template<class T2>
   allocator(const allocator<T2, SegmentManager> &other)
      : mp_mngr(other.get_segment_manager()){}

   //!Allocates memory for an array of count elements.
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_ptr hint = 0)
   {
      (void)hint;
      if(size_overflows<sizeof(T)>(count)){
         throw bad_alloc();
      }
      return pointer(static_cast<value_type*>(mp_mngr->allocate(count*sizeof(T))));
   }

   //!Deallocates memory previously allocated.
   //!Never throws
   void deallocate(const pointer &ptr, size_type)
   {  mp_mngr->deallocate((void*)ipcdetail::to_raw_pointer(ptr));  }

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const
   {  return mp_mngr->get_size()/sizeof(T);   }

   //!Swap segment manager. Does not throw. If each allocator is placed in
   //!different memory segments, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  boost::adl_move_swap(alloc1.mp_mngr, alloc2.mp_mngr);   }

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold. This size only works for memory allocated with
   //!allocate, allocation_command and allocate_many.
   size_type size(const pointer &p) const
   {
      return (size_type)mp_mngr->size(ipcdetail::to_raw_pointer(p))/sizeof(T);
   }

   pointer allocation_command(boost::interprocess::allocation_type command,
                           size_type limit_size, size_type &prefer_in_recvd_out_size, pointer &reuse)
   {
      value_type *reuse_raw = ipcdetail::to_raw_pointer(reuse);
      pointer const p = mp_mngr->allocation_command(command, limit_size, prefer_in_recvd_out_size, reuse_raw);
      reuse = reuse_raw;
      return p;
   }

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void allocate_many(size_type elem_size, size_type num_elements, multiallocation_chain &chain)
   {
      if(size_overflows<sizeof(T)>(elem_size)){
         throw bad_alloc();
      }
      mp_mngr->allocate_many(elem_size*sizeof(T), num_elements, chain);
   }

   //!Allocates n_elements elements, each one of size elem_sizes[i]in a
   //!contiguous block
   //!of memory. The elements must be deallocated
   void allocate_many(const size_type *elem_sizes, size_type n_elements, multiallocation_chain &chain)
   {
      mp_mngr->allocate_many(elem_sizes, n_elements, sizeof(T), chain);
   }

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void deallocate_many(multiallocation_chain &chain)
   {  mp_mngr->deallocate_many(chain); }

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one()
   {  return this->allocate(1);  }

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void allocate_individual(size_type num_elements, multiallocation_chain &chain)
   {  this->allocate_many(1, num_elements, chain); }

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p)
   {  return this->deallocate(p, 1);  }

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void deallocate_individual(multiallocation_chain &chain)
   {  this->deallocate_many(chain); }

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const
   {  return pointer(boost::addressof(value));  }

   //!Returns address of non mutable object.
   //!Never throws
   const_pointer address(const_reference value) const
   {  return const_pointer(boost::addressof(value));  }

   //!Constructs an object
   //!Throws if T's constructor throws
   //!For backwards compatibility with libraries using C++03 allocators
   template<class P>
   void construct(const pointer &ptr, BOOST_FWD_REF(P) p)
   {  ::new((void*)ipcdetail::to_raw_pointer(ptr), boost_container_new_t()) value_type(::boost::forward<P>(p));  }

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr)
   {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }

};

//!Equality test for same type
//!of allocator
template<class T, class SegmentManager> inline
bool operator==(const allocator<T , SegmentManager>  &alloc1,
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

//!Inequality test for same type
//!of allocator
template<class T, class SegmentManager> inline
bool operator!=(const allocator<T, SegmentManager>  &alloc1,
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template<class T>
struct has_trivial_destructor;

template<class T, class SegmentManager>
struct has_trivial_destructor
   <boost::interprocess::allocator <T, SegmentManager> >
{
   static const bool value = true;
};
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_ALLOCATOR_HPP

