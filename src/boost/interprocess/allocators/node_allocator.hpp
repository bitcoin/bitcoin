//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NODE_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_NODE_ALLOCATOR_HPP

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
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/allocators/detail/node_pool.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/container/detail/multiallocation_chain.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <cstddef>

//!\file
//!Describes node_allocator pooled shared memory STL compatible allocator

namespace boost {
namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace ipcdetail{

template < unsigned int Version
         , class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         >
class node_allocator_base
   : public node_pool_allocation_impl
   < node_allocator_base
      < Version, T, SegmentManager, NodesPerBlock>
   , Version
   , T
   , SegmentManager
   >
{
   public:
   typedef typename SegmentManager::void_pointer         void_pointer;
   typedef SegmentManager                                segment_manager;
   typedef node_allocator_base
      <Version, T, SegmentManager, NodesPerBlock>   self_t;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   template <int dummy>
   struct node_pool
   {
      typedef ipcdetail::shared_node_pool
      < SegmentManager, sizeof_value<T>::value, NodesPerBlock> type;

      static type *get(void *p)
      {  return static_cast<type*>(p);  }
   };
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   BOOST_STATIC_ASSERT((Version <=2));

   public:
   //-------
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<T>::type                         pointer;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<const T>::type                   const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;

   typedef boost::interprocess::version_type<node_allocator_base, Version>   version;
   typedef boost::container::dtl::transform_multiallocation_chain
      <typename SegmentManager::multiallocation_chain, T>multiallocation_chain;

   //!Obtains node_allocator_base from
   //!node_allocator_base
   template<class T2>
   struct rebind
   {
      typedef node_allocator_base<Version, T2, SegmentManager, NodesPerBlock>       other;
   };

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   //!Not assignable from related node_allocator_base
   template<unsigned int Version2, class T2, class SegmentManager2, std::size_t N2>
   node_allocator_base& operator=
      (const node_allocator_base<Version2, T2, SegmentManager2, N2>&);

   //!Not assignable from other node_allocator_base
   //node_allocator_base& operator=(const node_allocator_base&);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   node_allocator_base(segment_manager *segment_mngr)
      : mp_node_pool(ipcdetail::get_or_create_node_pool<typename node_pool<0>::type>(segment_mngr)) { }

   //!Copy constructor from other node_allocator_base. Increments the reference
   //!count of the associated node pool. Never throws
   node_allocator_base(const node_allocator_base &other)
      : mp_node_pool(other.get_node_pool())
   {
      node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool))->inc_ref_count();
   }

   //!Copy constructor from related node_allocator_base. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   node_allocator_base
      (const node_allocator_base<Version, T2, SegmentManager, NodesPerBlock> &other)
      : mp_node_pool(ipcdetail::get_or_create_node_pool<typename node_pool<0>::type>(other.get_segment_manager())) { }

   //!Assignment from other node_allocator_base
   node_allocator_base& operator=(const node_allocator_base &other)
   {
      node_allocator_base c(other);
      boost::adl_move_swap(*this, c);
      return *this;
   }

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~node_allocator_base()
   {  ipcdetail::destroy_node_pool_if_last_link(node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool)));   }

   //!Returns a pointer to the node pool.
   //!Never throws
   void* get_node_pool() const
   {  return ipcdetail::to_raw_pointer(mp_node_pool);   }

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const
   {  return node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool))->get_segment_manager();  }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  boost::adl_move_swap(alloc1.mp_node_pool, alloc2.mp_node_pool);  }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   void_pointer   mp_node_pool;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

//!Equality test for same type
//!of node_allocator_base
template<unsigned int V, class T, class S, std::size_t NPC> inline
bool operator==(const node_allocator_base<V, T, S, NPC> &alloc1,
                const node_allocator_base<V, T, S, NPC> &alloc2)
   {  return alloc1.get_node_pool() == alloc2.get_node_pool(); }

//!Inequality test for same type
//!of node_allocator_base
template<unsigned int V, class T, class S, std::size_t NPC> inline
bool operator!=(const node_allocator_base<V, T, S, NPC> &alloc1,
                const node_allocator_base<V, T, S, NPC> &alloc2)
   {  return alloc1.get_node_pool() != alloc2.get_node_pool(); }

template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock = 64
         >
class node_allocator_v1
   :  public node_allocator_base
         < 1
         , T
         , SegmentManager
         , NodesPerBlock
         >
{
   public:
   typedef ipcdetail::node_allocator_base
         < 1, T, SegmentManager, NodesPerBlock> base_t;

   template<class T2>
   struct rebind
   {
      typedef node_allocator_v1<T2, SegmentManager, NodesPerBlock>  other;
   };

   node_allocator_v1(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   template<class T2>
   node_allocator_v1
      (const node_allocator_v1<T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}
};

}  //namespace ipcdetail{

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!An STL node allocator that uses a segment manager as memory
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!This node allocator shares a segregated storage between all instances
//!of node_allocator with equal sizeof(T) placed in the same segment
//!group. NodesPerBlock is the number of nodes allocated at once when the allocator
//!runs out of nodes
template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         >
class node_allocator
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   :  public ipcdetail::node_allocator_base
         < 2
         , T
         , SegmentManager
         , NodesPerBlock
         >
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
{

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typedef ipcdetail::node_allocator_base
         < 2, T, SegmentManager, NodesPerBlock> base_t;
   public:
   typedef boost::interprocess::version_type<node_allocator, 2>   version;

   template<class T2>
   struct rebind
   {
      typedef node_allocator<T2, SegmentManager, NodesPerBlock>  other;
   };

   node_allocator(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   template<class T2>
   node_allocator
      (const node_allocator<T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}

   #else //BOOST_INTERPROCESS_DOXYGEN_INVOKED
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

   //!Obtains node_allocator from
   //!node_allocator
   template<class T2>
   struct rebind
   {
      typedef node_allocator<T2, SegmentManager, NodesPerBlock> other;
   };

   private:
   //!Not assignable from
   //!related node_allocator
   template<class T2, class SegmentManager2, std::size_t N2>
   node_allocator& operator=
      (const node_allocator<T2, SegmentManager2, N2>&);

   //!Not assignable from
   //!other node_allocator
   //node_allocator& operator=(const node_allocator&);

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   node_allocator(segment_manager *segment_mngr);

   //!Copy constructor from other node_allocator. Increments the reference
   //!count of the associated node pool. Never throws
   node_allocator(const node_allocator &other);

   //!Copy constructor from related node_allocator. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   node_allocator
      (const node_allocator<T2, SegmentManager, NodesPerBlock> &other);

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~node_allocator();

   //!Returns a pointer to the node pool.
   //!Never throws
   void* get_node_pool() const;

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
   void allocate_individual(size_type num_elements, multiallocation_chain &chain);

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
   #endif
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Equality test for same type
//!of node_allocator
template<class T, class S, std::size_t NPC> inline
bool operator==(const node_allocator<T, S, NPC> &alloc1,
                const node_allocator<T, S, NPC> &alloc2);

//!Inequality test for same type
//!of node_allocator
template<class T, class S, std::size_t NPC> inline
bool operator!=(const node_allocator<T, S, NPC> &alloc1,
                const node_allocator<T, S, NPC> &alloc2);

#endif

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_NODE_ALLOCATOR_HPP
