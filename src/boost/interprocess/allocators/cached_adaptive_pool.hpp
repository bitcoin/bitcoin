//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CACHED_ADAPTIVE_POOL_HPP
#define BOOST_INTERPROCESS_CACHED_ADAPTIVE_POOL_HPP

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
#include <boost/interprocess/allocators/detail/adaptive_node_pool.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/interprocess/allocators/detail/node_tools.hpp>
#include <cstddef>

//!\file
//!Describes cached_adaptive_pool pooled shared memory STL compatible allocator

namespace boost {
namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace ipcdetail {

template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock = 64
         , std::size_t MaxFreeBlocks = 2
         , unsigned char OverheadPercent = 5
         >
class cached_adaptive_pool_v1
   :  public ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_adaptive_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , MaxFreeBlocks
            , OverheadPercent
            >
         , 1>
{
   public:
   typedef ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_adaptive_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , MaxFreeBlocks
            , OverheadPercent
            >
         , 1> base_t;

   template<class T2>
   struct rebind
   {
      typedef cached_adaptive_pool_v1
         <T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>  other;
   };

   typedef typename base_t::size_type size_type;

   cached_adaptive_pool_v1(SegmentManager *segment_mngr,
                           size_type max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(segment_mngr, max_cached_nodes)
   {}

   template<class T2>
   cached_adaptive_pool_v1
      (const cached_adaptive_pool_v1
         <T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other)
      : base_t(other)
   {}
};

}  //namespace ipcdetail{

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!An STL node allocator that uses a segment manager as memory
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!
//!This node allocator shares a segregated storage between all instances of
//!cached_adaptive_pool with equal sizeof(T) placed in the same
//!memory segment. But also caches some nodes privately to
//!avoid some synchronization overhead.
//!
//!NodesPerBlock is the minimum number of nodes of nodes allocated at once when
//!the allocator needs runs out of nodes. MaxFreeBlocks is the maximum number of totally free blocks
//!that the adaptive node pool will hold. The rest of the totally free blocks will be
//!deallocated with the segment manager.
//!
//!OverheadPercent is the (approximated) maximum size overhead (1-20%) of the allocator:
//!(memory usable for nodes / total memory allocated from the segment manager)
template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         , std::size_t MaxFreeBlocks
         , unsigned char OverheadPercent
         >
class cached_adaptive_pool
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   :  public ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_adaptive_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , MaxFreeBlocks
            , OverheadPercent
            >
         , 2>
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
{

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   typedef ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_adaptive_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , MaxFreeBlocks
            , OverheadPercent
            >
         , 2> base_t;

   public:
   typedef boost::interprocess::version_type<cached_adaptive_pool, 2>   version;

   template<class T2>
   struct rebind
   {
      typedef cached_adaptive_pool
         <T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>  other;
   };

   cached_adaptive_pool(SegmentManager *segment_mngr,
                         std::size_t max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(segment_mngr, max_cached_nodes)
   {}

   template<class T2>
   cached_adaptive_pool
      (const cached_adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other)
      : base_t(other)
   {}

   #else
   public:
   typedef implementation_defined::segment_manager       segment_manager;
   typedef segment_manager::void_pointer                 void_pointer;
   typedef implementation_defined::pointer               pointer;
   typedef implementation_defined::const_pointer         const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;

   //!Obtains cached_adaptive_pool from
   //!cached_adaptive_pool
   template<class T2>
   struct rebind
   {
      typedef cached_adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> other;
   };

   private:
   //!Not assignable from
   //!related cached_adaptive_pool
   template<class T2, class SegmentManager2, std::size_t N2, std::size_t F2, unsigned char OP2>
   cached_adaptive_pool& operator=
      (const cached_adaptive_pool<T2, SegmentManager2, N2, F2, OP2>&);

   //!Not assignable from
   //!other cached_adaptive_pool
   cached_adaptive_pool& operator=(const cached_adaptive_pool&);

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   cached_adaptive_pool(segment_manager *segment_mngr);

   //!Copy constructor from other cached_adaptive_pool. Increments the reference
   //!count of the associated node pool. Never throws
   cached_adaptive_pool(const cached_adaptive_pool &other);

   //!Copy constructor from related cached_adaptive_pool. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   cached_adaptive_pool
      (const cached_adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other);

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~cached_adaptive_pool();

   //!Returns a pointer to the node pool.
   //!Never throws
   node_pool_t* get_node_pool() const;

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const;

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const;

   //!Allocate memory for an array of count elements.
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_pointer hint = 0);

   //!Deallocate allocated memory.
   //!Never throws
   void deallocate(const pointer &ptr, size_type count);

   //!Deallocates all free blocks
   //!of the pool
   void deallocate_free_blocks();

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2);

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const;

   //!Returns address of non mutable object.
   //!Never throws
   const_pointer address(const_reference value) const;

   //!Copy construct an object.
   //!Throws if T's copy constructor throws
   void construct(const pointer &ptr, const_reference v);

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr);

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold. This size only works for memory allocated with
   //!allocate, allocation_command and allocate_many.
   size_type size(const pointer &p) const;

   pointer allocation_command(boost::interprocess::allocation_type command,
                         size_type limit_size, size_type &prefer_in_recvd_out_size, pointer &reuse);

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void allocate_many(size_type elem_size, size_type num_elements, multiallocation_chain &chain);

   //!Allocates n_elements elements, each one of size elem_sizes[i]in a
   //!contiguous block
   //!of memory. The elements must be deallocated
   void allocate_many(const size_type *elem_sizes, size_type n_elements, multiallocation_chain &chain);

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void deallocate_many(multiallocation_chain &chain);

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one();

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   multiallocation_chain allocate_individual(size_type num_elements);

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p);

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void deallocate_individual(multiallocation_chain &chain);
   //!Sets the new max cached nodes value. This can provoke deallocations
   //!if "newmax" is less than current cached nodes. Never throws
   void set_max_cached_nodes(size_type newmax);

   //!Returns the max cached nodes parameter.
   //!Never throws
   size_type get_max_cached_nodes() const;
   #endif
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Equality test for same type
//!of cached_adaptive_pool
template<class T, class S, std::size_t NodesPerBlock, std::size_t F, std::size_t OP> inline
bool operator==(const cached_adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc1,
                const cached_adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc2);

//!Inequality test for same type
//!of cached_adaptive_pool
template<class T, class S, std::size_t NodesPerBlock, std::size_t F, std::size_t OP> inline
bool operator!=(const cached_adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc1,
                const cached_adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc2);

#endif

}  //namespace interprocess {
}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CACHED_ADAPTIVE_POOL_HPP

