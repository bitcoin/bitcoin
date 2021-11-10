//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP
#define BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

// interprocess
#include <boost/interprocess/containers/allocation_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/mem_algo/detail/mem_algo_common.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
// interprocess/detail
#include <boost/interprocess/detail/min_max.hpp>
#include <boost/interprocess/detail/math_functions.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/utilities.hpp>
// container
#include <boost/container/detail/multiallocation_chain.hpp>
// container/detail
#include <boost/container/detail/placement_new.hpp>
// move/detail
#include <boost/move/detail/type_traits.hpp> //make_unsigned, alignment_of
// intrusive
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/set.hpp>
// other boost
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
// std
#include <climits>
#include <cstring>

//#define BOOST_INTERPROCESS_RBTREE_BEST_FIT_ABI_V1_HPP
//to maintain ABI compatible with the original version
//ABI had to be updated to fix compatibility issues when
//sharing shared memory between 32 adn 64 bit processes.

//!\file
//!Describes a best-fit algorithm based in an intrusive red-black tree used to allocate
//!objects in shared memory. This class is intended as a base class for single segment
//!and multi-segment implementations.

namespace boost {
namespace interprocess {

//!This class implements an algorithm that stores the free nodes in a red-black tree
//!to have logarithmic search/insert times.
template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
class rbtree_best_fit
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable
   rbtree_best_fit();
   rbtree_best_fit(const rbtree_best_fit &);
   rbtree_best_fit &operator=(const rbtree_best_fit &);

   private:
   struct block_ctrl;
   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<block_ctrl>::type                   block_ctrl_ptr;

   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<char>::type                         char_ptr;

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Shared mutex family used for the rest of the Interprocess framework
   typedef MutexFamily        mutex_family;
   //!Pointer type to be used with the rest of the Interprocess framework
   typedef VoidPointer        void_pointer;
   typedef ipcdetail::basic_multiallocation_chain<VoidPointer>  multiallocation_chain;

   typedef typename boost::intrusive::pointer_traits<char_ptr>::difference_type difference_type;
   typedef typename boost::container::dtl::make_unsigned<difference_type>::type     size_type;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   private:

   typedef typename bi::make_set_base_hook
      < bi::void_pointer<VoidPointer>
      , bi::optimize_size<true>
      , bi::link_mode<bi::normal_link> >::type           TreeHook;

   struct SizeHolder
   {
      //!This block's memory size (including block_ctrl
      //!header) in Alignment units
      size_type m_prev_size;
      size_type m_size      :  sizeof(size_type)*CHAR_BIT - 2;
      size_type m_prev_allocated :  1;
      size_type m_allocated :  1;
   };

   //!Block control structure
   struct block_ctrl
      :  public SizeHolder, public TreeHook
   {
      block_ctrl()
      {  this->m_size = 0; this->m_allocated = 0, this->m_prev_allocated = 0;  }

      friend bool operator<(const block_ctrl &a, const block_ctrl &b)
      {  return a.m_size < b.m_size;  }
      friend bool operator==(const block_ctrl &a, const block_ctrl &b)
      {  return a.m_size == b.m_size;  }
   };

   struct size_block_ctrl_compare
   {
      bool operator()(size_type size, const block_ctrl &block) const
      {  return size < block.m_size;  }

      bool operator()(const block_ctrl &block, size_type size) const
      {  return block.m_size < size;  }
   };

   //!Shared mutex to protect memory allocate/deallocate
   typedef typename MutexFamily::mutex_type                       mutex_type;
   typedef typename bi::make_multiset
      <block_ctrl, bi::base_hook<TreeHook> >::type                Imultiset;

   typedef typename Imultiset::iterator                           imultiset_iterator;
   typedef typename Imultiset::const_iterator                     imultiset_const_iterator;

   //!This struct includes needed data and derives from
   //!mutex_type to allow EBO when using null mutex_type
   struct header_t : public mutex_type
   {
      Imultiset            m_imultiset;

      //!The extra size required by the segment
      size_type            m_extra_hdr_bytes;
      //!Allocated bytes for internal checking
      size_type            m_allocated;
      //!The size of the memory segment
      size_type            m_size;
   }  m_header;

   friend class ipcdetail::memory_algorithm_common<rbtree_best_fit>;

   typedef ipcdetail::memory_algorithm_common<rbtree_best_fit> algo_impl_t;

   public:
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Constructor. "size" is the total size of the managed memory segment,
   //!"extra_hdr_bytes" indicates the extra bytes beginning in the sizeof(rbtree_best_fit)
   //!offset that the allocator should not use at all.
   rbtree_best_fit           (size_type size, size_type extra_hdr_bytes);

   //!Destructor.
   ~rbtree_best_fit();

   //!Obtains the minimum size needed by the algorithm
   static size_type get_min_size (size_type extra_hdr_bytes);

   //Functions for single segment management

   //!Allocates bytes, returns 0 if there is not more memory
   void* allocate             (size_type nbytes);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //Experimental. Dont' use

   //!Multiple element allocation, same size
   void allocate_many(size_type elem_bytes, size_type num_elements, multiallocation_chain &chain)
   {
      //-----------------------
      boost::interprocess::scoped_lock<mutex_type> guard(m_header);
      //-----------------------
      algo_impl_t::allocate_many(this, elem_bytes, num_elements, chain);
   }

   //!Multiple element allocation, different size
   void allocate_many(const size_type *elem_sizes, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {
      //-----------------------
      boost::interprocess::scoped_lock<mutex_type> guard(m_header);
      //-----------------------
      algo_impl_t::allocate_many(this, elem_sizes, n_elements, sizeof_element, chain);
   }

   //!Multiple element allocation, different size
   void deallocate_many(multiallocation_chain &chain);

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Deallocates previously allocated bytes
   void   deallocate          (void *addr);

   //!Returns the size of the memory segment
   size_type get_size()  const;

   //!Returns the number of free bytes of the segment
   size_type get_free_memory()  const;

   //!Initializes to zero all the memory that's not in use.
   //!This function is normally used for security reasons.
   void zero_free_memory();

   //!Increases managed memory in
   //!extra_size bytes more
   void grow(size_type extra_size);

   //!Decreases managed memory as much as possible
   void shrink_to_fit();

   //!Returns true if all allocated memory has been deallocated
   bool all_memory_deallocated();

   //!Makes an internal sanity check
   //!and returns true if success
   bool check_sanity();

   template<class T>
   T * allocation_command  (boost::interprocess::allocation_type command, size_type limit_size,
                           size_type &prefer_in_recvd_out_size, T *&reuse);

   void * raw_allocation_command (boost::interprocess::allocation_type command,   size_type limit_object,
                              size_type &prefer_in_recvd_out_size,
                              void *&reuse_ptr, size_type sizeof_object = 1);

   //!Returns the size of the buffer previously allocated pointed by ptr
   size_type size(const void *ptr) const;

   //!Allocates aligned bytes, returns 0 if there is not more memory.
   //!Alignment must be power of 2
   void* allocate_aligned     (size_type nbytes, size_type alignment);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   static size_type priv_first_block_offset_from_this(const void *this_ptr, size_type extra_hdr_bytes);

   block_ctrl *priv_first_block();

   block_ctrl *priv_end_block();

   void* priv_allocation_command(boost::interprocess::allocation_type command,   size_type limit_size,
                        size_type &prefer_in_recvd_out_size, void *&reuse_ptr, size_type sizeof_object);


   //!Real allocation algorithm with min allocation option
   void * priv_allocate( boost::interprocess::allocation_type command
                       , size_type limit_size, size_type &prefer_in_recvd_out_size
                       , void *&reuse_ptr, size_type backwards_multiple = 1);

   //!Obtains the block control structure of the user buffer
   static block_ctrl *priv_get_block(const void *ptr);

   //!Obtains the pointer returned to the user from the block control
   static void *priv_get_user_buffer(const block_ctrl *block);

   //!Returns the number of total units that a user buffer
   //!of "userbytes" bytes really occupies (including header)
   static size_type priv_get_total_units(size_type userbytes);

   //!Real expand function implementation
   bool priv_expand(void *ptr, const size_type min_size, size_type &prefer_in_recvd_out_size);

   //!Real expand to both sides implementation
   void* priv_expand_both_sides(boost::interprocess::allocation_type command
                               ,size_type min_size
                               ,size_type &prefer_in_recvd_out_size
                               ,void *reuse_ptr
                               ,bool only_preferred_backwards
                               ,size_type backwards_multiple);

   //!Returns true if the previous block is allocated
   bool priv_is_prev_allocated(block_ctrl *ptr);

   //!Get a pointer of the "end" block from the first block of the segment
   static block_ctrl * priv_end_block(block_ctrl *first_segment_block);

   //!Get a pointer of the "first" block from the end block of the segment
   static block_ctrl * priv_first_block(block_ctrl *end_segment_block);

   //!Get poitner of the previous block (previous block must be free)
   static block_ctrl * priv_prev_block(block_ctrl *ptr);

   //!Get the size in the tail of the previous block
   static block_ctrl * priv_next_block(block_ctrl *ptr);

   //!Check if this block is free (not allocated)
   bool priv_is_allocated_block(block_ctrl *ptr);

   //!Marks the block as allocated
   void priv_mark_as_allocated_block(block_ctrl *ptr);

   //!Marks the block as allocated
   void priv_mark_new_allocated_block(block_ctrl *ptr)
   {  return priv_mark_as_allocated_block(ptr); }

   //!Marks the block as allocated
   void priv_mark_as_free_block(block_ctrl *ptr);

   //!Checks if block has enough memory and splits/unlinks the block
   //!returning the address to the users
   void* priv_check_and_allocate(size_type units
                                ,block_ctrl* block
                                ,size_type &received_size);
   //!Real deallocation algorithm
   void priv_deallocate(void *addr);

   //!Makes a new memory portion available for allocation
   void priv_add_segment(void *addr, size_type size);

   public:

   static const size_type Alignment = !MemAlignment
      ? size_type(::boost::container::dtl::alignment_of
                  < ::boost::container::dtl::max_align_t>::value)
      : size_type(MemAlignment)
      ;

   private:
   //Due to embedded bits in size, Alignment must be at least 4
   BOOST_STATIC_ASSERT((Alignment >= 4));
   //Due to rbtree size optimizations, Alignment must have at least pointer alignment
   BOOST_STATIC_ASSERT((Alignment >= ::boost::container::dtl::alignment_of<void_pointer>::value));
   static const size_type AlignmentMask = (Alignment - 1);
   static const size_type BlockCtrlBytes = ipcdetail::ct_rounded_size<sizeof(block_ctrl), Alignment>::value;
   static const size_type BlockCtrlUnits = BlockCtrlBytes/Alignment;
   static const size_type AllocatedCtrlBytes  = ipcdetail::ct_rounded_size<sizeof(SizeHolder), Alignment>::value;
   static const size_type AllocatedCtrlUnits  = AllocatedCtrlBytes/Alignment;
   static const size_type EndCtrlBlockBytes   = ipcdetail::ct_rounded_size<sizeof(SizeHolder), Alignment>::value;
   static const size_type EndCtrlBlockUnits   = EndCtrlBlockBytes/Alignment;
   static const size_type MinBlockUnits       = BlockCtrlUnits;
   static const size_type UsableByPreviousChunk   = sizeof(size_type);

   //Make sure the maximum alignment is power of two
   BOOST_STATIC_ASSERT((0 == (Alignment & (Alignment - size_type(1u)))));
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   static const size_type PayloadPerAllocation = AllocatedCtrlBytes - UsableByPreviousChunk;
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
       rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>
   ::priv_first_block_offset_from_this(const void *this_ptr, size_type extra_hdr_bytes)
{
   size_type uint_this      = (std::size_t)this_ptr;
   size_type main_hdr_end   = uint_this + sizeof(rbtree_best_fit) + extra_hdr_bytes;
   size_type aligned_main_hdr_end = ipcdetail::get_rounded_size(main_hdr_end, Alignment);
   size_type block1_off = aligned_main_hdr_end -  uint_this;
   algo_impl_t::assert_alignment(aligned_main_hdr_end);
   algo_impl_t::assert_alignment(uint_this + block1_off);
   return block1_off;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_add_segment(void *addr, size_type segment_size)
{
   //Check alignment
   algo_impl_t::check_alignment(addr);
   //Check size
   BOOST_ASSERT(segment_size >= (BlockCtrlBytes + EndCtrlBlockBytes));

   //Initialize the first big block and the "end" node
   block_ctrl *first_big_block = ::new(addr, boost_container_new_t())block_ctrl;
   first_big_block->m_size = segment_size/Alignment - EndCtrlBlockUnits;
   BOOST_ASSERT(first_big_block->m_size >= BlockCtrlUnits);

   //The "end" node is just a node of size 0 with the "end" bit set
   block_ctrl *end_block = static_cast<block_ctrl*>
      (new (reinterpret_cast<char*>(addr) + first_big_block->m_size*Alignment)SizeHolder);

   //This will overwrite the prev part of the "end" node
   priv_mark_as_free_block (first_big_block);
   #ifdef BOOST_INTERPROCESS_RBTREE_BEST_FIT_ABI_V1_HPP
   first_big_block->m_prev_size = end_block->m_size =
      (reinterpret_cast<char*>(first_big_block) - reinterpret_cast<char*>(end_block))/Alignment;
   #else
   first_big_block->m_prev_size = end_block->m_size =
      (reinterpret_cast<char*>(end_block) - reinterpret_cast<char*>(first_big_block))/Alignment;
   #endif
   end_block->m_allocated = 1;
   first_big_block->m_prev_allocated = 1;

   BOOST_ASSERT(priv_next_block(first_big_block) == end_block);
   BOOST_ASSERT(priv_prev_block(end_block) == first_big_block);
   BOOST_ASSERT(priv_first_block() == first_big_block);
   BOOST_ASSERT(priv_end_block() == end_block);

   //Some check to validate the algorithm, since it makes some assumptions
   //to optimize the space wasted in bookkeeping:

   //Check that the sizes of the header are placed before the rbtree
   BOOST_ASSERT(static_cast<void*>(static_cast<SizeHolder*>(first_big_block))
          < static_cast<void*>(static_cast<TreeHook*>(first_big_block)));
   //Insert it in the intrusive containers
   m_header.m_imultiset.insert(*first_big_block);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
       rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>
   ::priv_first_block()
{
   const size_type block1_off = priv_first_block_offset_from_this(this, m_header.m_extra_hdr_bytes);
   return reinterpret_cast<block_ctrl *>(reinterpret_cast<char*>(this) + block1_off);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
       rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>
   ::priv_end_block()
{
   const size_type block1_off = priv_first_block_offset_from_this(this, m_header.m_extra_hdr_bytes);
   const size_type original_first_block_size = (m_header.m_size - block1_off)/Alignment - EndCtrlBlockUnits;
   block_ctrl *end_block = reinterpret_cast<block_ctrl*>
      (reinterpret_cast<char*>(this) + block1_off + original_first_block_size*Alignment);
   return end_block;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   rbtree_best_fit(size_type segment_size, size_type extra_hdr_bytes)
{
   //Initialize the header
   m_header.m_allocated       = 0;
   m_header.m_size            = segment_size;
   m_header.m_extra_hdr_bytes = extra_hdr_bytes;

   //Now write calculate the offset of the first big block that will
   //cover the whole segment
   BOOST_ASSERT(get_min_size(extra_hdr_bytes) <= segment_size);
   size_type block1_off  = priv_first_block_offset_from_this(this, extra_hdr_bytes);
   priv_add_segment(reinterpret_cast<char*>(this) + block1_off, segment_size - block1_off);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::~rbtree_best_fit()
{
   //There is a memory leak!
//   BOOST_ASSERT(m_header.m_allocated == 0);
//   BOOST_ASSERT(m_header.m_root.m_next->m_next == block_ctrl_ptr(&m_header.m_root));
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::grow(size_type extra_size)
{
   //Get the address of the first block
   block_ctrl *first_block = priv_first_block();
   block_ctrl *old_end_block = priv_end_block();
   size_type old_border_offset = (size_type)(reinterpret_cast<char*>(old_end_block) -
                                    reinterpret_cast<char*>(this)) + EndCtrlBlockBytes;

   //Update managed buffer's size
   m_header.m_size += extra_size;

   //We need at least MinBlockUnits blocks to create a new block
   if((m_header.m_size - old_border_offset) < MinBlockUnits){
      return;
   }

   //Now create a new block between the old end and the new end
   size_type align_offset = (m_header.m_size - old_border_offset)/Alignment;
   block_ctrl *new_end_block = reinterpret_cast<block_ctrl*>
      (reinterpret_cast<char*>(old_end_block) + align_offset*Alignment);

   //the last and first block are special:
   //new_end_block->m_size & first_block->m_prev_size store the absolute value
   //between them
   new_end_block->m_allocated = 1;
   #ifdef BOOST_INTERPROCESS_RBTREE_BEST_FIT_ABI_V1_HPP
   new_end_block->m_size      = (reinterpret_cast<char*>(first_block) -
                                 reinterpret_cast<char*>(new_end_block))/Alignment;
   #else
   new_end_block->m_size      = (reinterpret_cast<char*>(new_end_block) -
                                 reinterpret_cast<char*>(first_block))/Alignment;
   #endif
   first_block->m_prev_size = new_end_block->m_size;
   first_block->m_prev_allocated = 1;
   BOOST_ASSERT(new_end_block == priv_end_block());

   //The old end block is the new block
   block_ctrl *new_block = old_end_block;
   new_block->m_size = (reinterpret_cast<char*>(new_end_block) -
                        reinterpret_cast<char*>(new_block))/Alignment;
   BOOST_ASSERT(new_block->m_size >= BlockCtrlUnits);
   priv_mark_as_allocated_block(new_block);
   BOOST_ASSERT(priv_next_block(new_block) == new_end_block);

   m_header.m_allocated += (size_type)new_block->m_size*Alignment;

   //Now deallocate the newly created block
   this->priv_deallocate(priv_get_user_buffer(new_block));
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::shrink_to_fit()
{
   //Get the address of the first block
   block_ctrl *first_block = priv_first_block();
   algo_impl_t::assert_alignment(first_block);

   //block_ctrl *old_end_block = priv_end_block(first_block);
   block_ctrl *old_end_block = priv_end_block();
   algo_impl_t::assert_alignment(old_end_block);
   size_type old_end_block_size = old_end_block->m_size;

   void *unique_buffer = 0;
   block_ctrl *last_block;
   //Check if no memory is allocated between the first and last block
   if(priv_next_block(first_block) == old_end_block){
      //If so check if we can allocate memory
      size_type ignore_recvd = 0;
      void *ignore_reuse = 0;
      unique_buffer = priv_allocate(boost::interprocess::allocate_new, 0, ignore_recvd, ignore_reuse);
      //If not, return, we can't shrink
      if(!unique_buffer)
         return;
      //If we can, mark the position just after the new allocation as the new end
      algo_impl_t::assert_alignment(unique_buffer);
      block_ctrl *unique_block = priv_get_block(unique_buffer);
      BOOST_ASSERT(priv_is_allocated_block(unique_block));
      algo_impl_t::assert_alignment(unique_block);
      last_block = priv_next_block(unique_block);
      BOOST_ASSERT(!priv_is_allocated_block(last_block));
      algo_impl_t::assert_alignment(last_block);
   }
   else{
      //If memory is allocated, check if the last block is allocated
      if(priv_is_prev_allocated(old_end_block))
         return;
      //If not, mark last block after the free block
      last_block = priv_prev_block(old_end_block);
   }

   size_type last_block_size = last_block->m_size;

   //Erase block from the free tree, since we will erase it
   m_header.m_imultiset.erase(Imultiset::s_iterator_to(*last_block));

   size_type shrunk_border_offset = (size_type)(reinterpret_cast<char*>(last_block) -
                                       reinterpret_cast<char*>(this)) + EndCtrlBlockBytes;

   block_ctrl *new_end_block = last_block;
   algo_impl_t::assert_alignment(new_end_block);

   //Write new end block attributes
   #ifdef BOOST_INTERPROCESS_RBTREE_BEST_FIT_ABI_V1_HPP
   new_end_block->m_size = first_block->m_prev_size =
      (reinterpret_cast<char*>(first_block) - reinterpret_cast<char*>(new_end_block))/Alignment;
   #else
   new_end_block->m_size = first_block->m_prev_size =
      (reinterpret_cast<char*>(new_end_block) - reinterpret_cast<char*>(first_block))/Alignment;
   #endif

   new_end_block->m_allocated = 1;
   (void)last_block_size;
   (void)old_end_block_size;
   BOOST_ASSERT(new_end_block->m_size == (old_end_block_size - last_block_size));

   //Update managed buffer's size
   m_header.m_size = shrunk_border_offset;
   BOOST_ASSERT(priv_end_block() == new_end_block);
   if(unique_buffer)
      priv_deallocate(unique_buffer);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::get_size()  const
{  return m_header.m_size;  }

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::get_free_memory()  const
{
   return m_header.m_size - m_header.m_allocated -
      priv_first_block_offset_from_this(this, m_header.m_extra_hdr_bytes);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   get_min_size (size_type extra_hdr_bytes)
{
   return (algo_impl_t::ceil_units(sizeof(rbtree_best_fit)) +
           algo_impl_t::ceil_units(extra_hdr_bytes) +
           MinBlockUnits + EndCtrlBlockUnits)*Alignment;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline bool rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
    all_memory_deallocated()
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   size_type block1_off  =
      priv_first_block_offset_from_this(this, m_header.m_extra_hdr_bytes);

   return m_header.m_allocated == 0 &&
      m_header.m_imultiset.begin() != m_header.m_imultiset.end() &&
       (++m_header.m_imultiset.begin()) == m_header.m_imultiset.end()
       && m_header.m_imultiset.begin()->m_size ==
         (m_header.m_size - block1_off - EndCtrlBlockBytes)/Alignment;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
bool rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
    check_sanity()
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   imultiset_iterator ib(m_header.m_imultiset.begin()), ie(m_header.m_imultiset.end());

   size_type free_memory = 0;

   //Iterate through all blocks obtaining their size
   for(; ib != ie; ++ib){
      free_memory += (size_type)ib->m_size*Alignment;
      algo_impl_t::assert_alignment(&*ib);
      if(!algo_impl_t::check_alignment(&*ib))
         return false;
   }

   //Check allocated bytes are less than size
   if(m_header.m_allocated > m_header.m_size){
      return false;
   }

   size_type block1_off  =
      priv_first_block_offset_from_this(this, m_header.m_extra_hdr_bytes);

   //Check free bytes are less than size
   if(free_memory > (m_header.m_size - block1_off)){
      return false;
   }
   return true;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   allocate(size_type nbytes)
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   size_type ignore_recvd = nbytes;
   void *ignore_reuse = 0;
   return priv_allocate(boost::interprocess::allocate_new, nbytes, ignore_recvd, ignore_reuse);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   allocate_aligned(size_type nbytes, size_type alignment)
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   return algo_impl_t::allocate_aligned(this, nbytes, alignment);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
template<class T>
inline T* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   allocation_command  (boost::interprocess::allocation_type command,   size_type limit_size,
                        size_type &prefer_in_recvd_out_size, T *&reuse)
{
   void* raw_reuse = reuse;
   void* const ret = priv_allocation_command(command, limit_size, prefer_in_recvd_out_size, raw_reuse, sizeof(T));
   reuse = static_cast<T*>(raw_reuse);
   BOOST_ASSERT(0 == ((std::size_t)ret % ::boost::container::dtl::alignment_of<T>::value));
   return static_cast<T*>(ret);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   raw_allocation_command  (boost::interprocess::allocation_type command,   size_type limit_objects,
                        size_type &prefer_in_recvd_out_objects, void *&reuse_ptr, size_type sizeof_object)
{
   size_type const preferred_objects = prefer_in_recvd_out_objects;
   if(!sizeof_object)
      return reuse_ptr = 0, static_cast<void*>(0);
   if(command & boost::interprocess::try_shrink_in_place){
      if(!reuse_ptr)  return static_cast<void*>(0);
      const bool success = algo_impl_t::try_shrink
         ( this, reuse_ptr, limit_objects*sizeof_object
         , prefer_in_recvd_out_objects = preferred_objects*sizeof_object);
      prefer_in_recvd_out_objects /= sizeof_object;
      return success ? reuse_ptr : 0;
   }
   else{
      return priv_allocation_command
         (command, limit_objects, prefer_in_recvd_out_objects, reuse_ptr, sizeof_object);
   }
}


template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_allocation_command (boost::interprocess::allocation_type command,   size_type limit_size,
                       size_type &prefer_in_recvd_out_size,
                       void *&reuse_ptr, size_type sizeof_object)
{
   void* ret;
   size_type const preferred_size = prefer_in_recvd_out_size;
   size_type const max_count = m_header.m_size/sizeof_object;
   if(limit_size > max_count || preferred_size > max_count){
      return reuse_ptr = 0, static_cast<void*>(0);
   }
   size_type l_size = limit_size*sizeof_object;
   size_type p_size = preferred_size*sizeof_object;
   size_type r_size;
   {
      //-----------------------
      boost::interprocess::scoped_lock<mutex_type> guard(m_header);
      //-----------------------
      ret = priv_allocate(command, l_size, r_size = p_size, reuse_ptr, sizeof_object);
   }
   prefer_in_recvd_out_size = r_size/sizeof_object;
   return ret;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   size(const void *ptr) const
{
   //We need no synchronization since this block's size is not going
   //to be modified by anyone else
   //Obtain the real size of the block
   return ((size_type)priv_get_block(ptr)->m_size - AllocatedCtrlUnits)*Alignment + UsableByPreviousChunk;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::zero_free_memory()
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   imultiset_iterator ib(m_header.m_imultiset.begin()), ie(m_header.m_imultiset.end());

   //Iterate through all blocks obtaining their size
   while(ib != ie){
      //Just clear user the memory part reserved for the user
      volatile char *ptr = reinterpret_cast<char*>(&*ib) + BlockCtrlBytes;
      size_type s = (size_type)ib->m_size*Alignment - BlockCtrlBytes;
      while(s--){
         *ptr++ = 0;
      }

      //This surprisingly is optimized out by Visual C++ 7.1 in release mode!
      //std::memset( reinterpret_cast<char*>(&*ib) + BlockCtrlBytes
      //           , 0
      //           , ib->m_size*Alignment - BlockCtrlBytes);
      ++ib;
   }
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_expand_both_sides(boost::interprocess::allocation_type command
                         ,size_type min_size
                         ,size_type &prefer_in_recvd_out_size
                         ,void *reuse_ptr
                         ,bool only_preferred_backwards
                         ,size_type backwards_multiple)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   algo_impl_t::assert_alignment(reuse_ptr);
   if(command & boost::interprocess::expand_fwd){
      if(priv_expand(reuse_ptr, min_size, prefer_in_recvd_out_size = preferred_size))
         return reuse_ptr;
   }
   else{
      prefer_in_recvd_out_size = this->size(reuse_ptr);
      if(prefer_in_recvd_out_size >= preferred_size || prefer_in_recvd_out_size >= min_size)
         return reuse_ptr;
   }

   if(backwards_multiple){
      BOOST_ASSERT(0 == (min_size       % backwards_multiple));
      BOOST_ASSERT(0 == (preferred_size % backwards_multiple));
   }

   if(command & boost::interprocess::expand_bwd){
      //Obtain the real size of the block
      block_ctrl *reuse = priv_get_block(reuse_ptr);

      //Sanity check
      algo_impl_t::assert_alignment(reuse);

      block_ctrl *prev_block;

      //If the previous block is not free, there is nothing to do
      if(priv_is_prev_allocated(reuse)){
         return 0;
      }

      prev_block = priv_prev_block(reuse);
      BOOST_ASSERT(!priv_is_allocated_block(prev_block));

      //Some sanity checks
      BOOST_ASSERT(prev_block->m_size == reuse->m_prev_size);
      algo_impl_t::assert_alignment(prev_block);

      size_type needs_backwards_aligned;
      size_type lcm;
      if(!algo_impl_t::calculate_lcm_and_needs_backwards_lcmed
         ( backwards_multiple
         , prefer_in_recvd_out_size
         , only_preferred_backwards ? preferred_size : min_size
         , lcm, needs_backwards_aligned)){
         return 0;
      }

      //Check if previous block has enough size
      if(size_type(prev_block->m_size*Alignment) >= needs_backwards_aligned){
         //Now take all next space. This will succeed
         if(command & boost::interprocess::expand_fwd){
            size_type received_size2;
            if(!priv_expand(reuse_ptr, prefer_in_recvd_out_size, received_size2 = prefer_in_recvd_out_size)){
               BOOST_ASSERT(0);
            }
            BOOST_ASSERT(prefer_in_recvd_out_size == received_size2);
         }
         //We need a minimum size to split the previous one
         if(prev_block->m_size >= (needs_backwards_aligned/Alignment + BlockCtrlUnits)){
            block_ctrl *new_block = reinterpret_cast<block_ctrl *>
               (reinterpret_cast<char*>(reuse) - needs_backwards_aligned);

            //Free old previous buffer
            new_block->m_size =
               AllocatedCtrlUnits + (needs_backwards_aligned + (prefer_in_recvd_out_size - UsableByPreviousChunk))/Alignment;
            BOOST_ASSERT(new_block->m_size >= BlockCtrlUnits);
            priv_mark_as_allocated_block(new_block);

            prev_block->m_size = (reinterpret_cast<char*>(new_block) -
                                  reinterpret_cast<char*>(prev_block))/Alignment;
            BOOST_ASSERT(prev_block->m_size >= BlockCtrlUnits);
            priv_mark_as_free_block(prev_block);

            //Update the old previous block in the free blocks tree
            //If the new size fulfills tree invariants do nothing,
            //otherwise erase() + insert()
            {
               imultiset_iterator prev_block_it(Imultiset::s_iterator_to(*prev_block));
               imultiset_iterator was_smaller_it(prev_block_it);
               if(prev_block_it != m_header.m_imultiset.begin() &&
                  (--(was_smaller_it = prev_block_it))->m_size > prev_block->m_size){
                  m_header.m_imultiset.erase(prev_block_it);
                  m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *prev_block);
               }
            }

            prefer_in_recvd_out_size = needs_backwards_aligned + prefer_in_recvd_out_size;
            m_header.m_allocated += needs_backwards_aligned;

            //Check alignment
            algo_impl_t::assert_alignment(new_block);

            //If the backwards expansion has remaining bytes in the
            //first bytes, fill them with a pattern
            void *p = priv_get_user_buffer(new_block);
            void *user_ptr = reinterpret_cast<char*>(p);
            BOOST_ASSERT((static_cast<char*>(reuse_ptr) - static_cast<char*>(user_ptr)) % backwards_multiple == 0);
            algo_impl_t::assert_alignment(user_ptr);
            return user_ptr;
         }
         //Check if there is no place to create a new block and
         //the whole new block is multiple of the backwards expansion multiple
         else if(prev_block->m_size >= needs_backwards_aligned/Alignment &&
                 0 == ((prev_block->m_size*Alignment) % lcm)) {
            //Erase old previous block, since we will change it
            m_header.m_imultiset.erase(Imultiset::s_iterator_to(*prev_block));

            //Just merge the whole previous block
            //prev_block->m_size*Alignment is multiple of lcm (and backwards_multiple)
            prefer_in_recvd_out_size = prefer_in_recvd_out_size + (size_type)prev_block->m_size*Alignment;

            m_header.m_allocated += (size_type)prev_block->m_size*Alignment;
            //Now update sizes
            prev_block->m_size = prev_block->m_size + reuse->m_size;
            BOOST_ASSERT(prev_block->m_size >= BlockCtrlUnits);
            priv_mark_as_allocated_block(prev_block);

            //If the backwards expansion has remaining bytes in the
            //first bytes, fill them with a pattern
            void *user_ptr = priv_get_user_buffer(prev_block);
            BOOST_ASSERT((static_cast<char*>(reuse_ptr) - static_cast<char*>(user_ptr)) % backwards_multiple == 0);
            algo_impl_t::assert_alignment(user_ptr);
            return user_ptr;
         }
         else{
            //Alignment issues
         }
      }
   }
   return 0;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   deallocate_many(typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::multiallocation_chain &chain)
{
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   algo_impl_t::deallocate_many(this, chain);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void * rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_allocate(boost::interprocess::allocation_type command
                ,size_type limit_size
                ,size_type &prefer_in_recvd_out_size
                ,void *&reuse_ptr
               ,size_type backwards_multiple)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   if(command & boost::interprocess::shrink_in_place){
      if(!reuse_ptr)  return static_cast<void*>(0);
      bool success =
         algo_impl_t::shrink(this, reuse_ptr, limit_size, prefer_in_recvd_out_size = preferred_size);
      return success ? reuse_ptr : 0;
   }

   prefer_in_recvd_out_size = 0;

   if(limit_size > preferred_size)
      return reuse_ptr = 0, static_cast<void*>(0);

   //Number of units to request (including block_ctrl header)
   size_type preferred_units = priv_get_total_units(preferred_size);

   //Number of units to request (including block_ctrl header)
   size_type limit_units = priv_get_total_units(limit_size);

   //Expand in place
   prefer_in_recvd_out_size = preferred_size;
   if(reuse_ptr && (command & (boost::interprocess::expand_fwd | boost::interprocess::expand_bwd))){
      void *ret = priv_expand_both_sides
         (command, limit_size, prefer_in_recvd_out_size, reuse_ptr, true, backwards_multiple);
      if(ret)
         return ret;
   }

   if(command & boost::interprocess::allocate_new){
      size_block_ctrl_compare comp;
      imultiset_iterator it(m_header.m_imultiset.lower_bound(preferred_units, comp));

      if(it != m_header.m_imultiset.end()){
         return reuse_ptr = 0, this->priv_check_and_allocate
            (preferred_units, ipcdetail::to_raw_pointer(&*it), prefer_in_recvd_out_size);
      }

      if(it != m_header.m_imultiset.begin()&&
              (--it)->m_size >= limit_units){
         return reuse_ptr = 0, this->priv_check_and_allocate
            (it->m_size, ipcdetail::to_raw_pointer(&*it), prefer_in_recvd_out_size);
      }
   }


   //Now try to expand both sides with min size
   if(reuse_ptr && (command & (boost::interprocess::expand_fwd | boost::interprocess::expand_bwd))){
      return priv_expand_both_sides
         (command, limit_size, prefer_in_recvd_out_size = preferred_size, reuse_ptr, false, backwards_multiple);
   }
   return reuse_ptr = 0, static_cast<void*>(0);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_get_block(const void *ptr)
{
   return const_cast<block_ctrl*>
      (reinterpret_cast<const block_ctrl*>
         (reinterpret_cast<const char*>(ptr) - AllocatedCtrlBytes));
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline
void *rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
      priv_get_user_buffer(const typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *block)
{  return const_cast<char*>(reinterpret_cast<const char*>(block) + AllocatedCtrlBytes);   }

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
inline typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::size_type
rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_get_total_units(size_type userbytes)
{
   if(userbytes < UsableByPreviousChunk)
      userbytes = UsableByPreviousChunk;
   size_type units = ipcdetail::get_rounded_size(userbytes - UsableByPreviousChunk, Alignment)/Alignment + AllocatedCtrlUnits;
   if(units < BlockCtrlUnits) units = BlockCtrlUnits;
   return units;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
bool rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::
   priv_expand (void *ptr, const size_type min_size, size_type &prefer_in_recvd_out_size)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   //Obtain the real size of the block
   block_ctrl *block = priv_get_block(ptr);
   size_type old_block_units = block->m_size;

   //The block must be marked as allocated and the sizes must be equal
   BOOST_ASSERT(priv_is_allocated_block(block));

   //Put this to a safe value
   prefer_in_recvd_out_size = (old_block_units - AllocatedCtrlUnits)*Alignment + UsableByPreviousChunk;
   if(prefer_in_recvd_out_size >= preferred_size || prefer_in_recvd_out_size >= min_size)
      return true;

   //Now translate it to Alignment units
   const size_type min_user_units = algo_impl_t::ceil_units(min_size - UsableByPreviousChunk);
   const size_type preferred_user_units = algo_impl_t::ceil_units(preferred_size - UsableByPreviousChunk);

   //Some parameter checks
   BOOST_ASSERT(min_user_units <= preferred_user_units);

   block_ctrl *next_block;

   if(priv_is_allocated_block(next_block = priv_next_block(block))){
      return prefer_in_recvd_out_size >= min_size;
   }
   algo_impl_t::assert_alignment(next_block);

   //Is "block" + "next_block" big enough?
   const size_type merged_units = old_block_units + (size_type)next_block->m_size;

   //Now get the expansion size
   const size_type merged_user_units = merged_units - AllocatedCtrlUnits;

   if(merged_user_units < min_user_units){
      prefer_in_recvd_out_size = merged_units*Alignment - UsableByPreviousChunk;
      return false;
   }

   //Now get the maximum size the user can allocate
   size_type intended_user_units = (merged_user_units < preferred_user_units) ?
      merged_user_units : preferred_user_units;

   //These are total units of the merged block (supposing the next block can be split)
   const size_type intended_units = AllocatedCtrlUnits + intended_user_units;

   //Check if we can split the next one in two parts
   if((merged_units - intended_units) >=  BlockCtrlUnits){
      //This block is bigger than needed, split it in
      //two blocks, the first one will be merged and
      //the second's size will be the remaining space
      BOOST_ASSERT(next_block->m_size == priv_next_block(next_block)->m_prev_size);
      const size_type rem_units = merged_units - intended_units;

      //Check if we we need to update the old next block in the free blocks tree
      //If the new size fulfills tree invariants, we just need to replace the node
      //(the block start has been displaced), otherwise erase() + insert().
      //
      //This fixup must be done in two parts, because the new next block might
      //overwrite the tree hook of the old next block. So we first erase the
      //old if needed and we'll insert the new one after creating the new next
      imultiset_iterator old_next_block_it(Imultiset::s_iterator_to(*next_block));
      const bool size_invariants_broken =
            (next_block->m_size - rem_units ) < BlockCtrlUnits ||
            (old_next_block_it != m_header.m_imultiset.begin() &&
            (--imultiset_iterator(old_next_block_it))->m_size > rem_units);
      if(size_invariants_broken){
         m_header.m_imultiset.erase(old_next_block_it);
      }
      //This is the remaining block
      block_ctrl *rem_block = ::new(reinterpret_cast<block_ctrl*>
                     (reinterpret_cast<char*>(block) + intended_units*Alignment), boost_container_new_t())block_ctrl;
      rem_block->m_size  = rem_units;
      algo_impl_t::assert_alignment(rem_block);
      BOOST_ASSERT(rem_block->m_size >= BlockCtrlUnits);
      priv_mark_as_free_block(rem_block);

      //Now the second part of the fixup
      if(size_invariants_broken)
         m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *rem_block);
      else
         m_header.m_imultiset.replace_node(old_next_block_it, *rem_block);

      //Write the new length
      block->m_size = intended_user_units + AllocatedCtrlUnits;
      BOOST_ASSERT(block->m_size >= BlockCtrlUnits);
      m_header.m_allocated += (intended_units - old_block_units)*Alignment;
   }
   //There is no free space to create a new node: just merge both blocks
   else{
      //Now we have to update the data in the tree
      m_header.m_imultiset.erase(Imultiset::s_iterator_to(*next_block));

      //Write the new length
      block->m_size = merged_units;
      BOOST_ASSERT(block->m_size >= BlockCtrlUnits);
      m_header.m_allocated += (merged_units - old_block_units)*Alignment;
   }
   priv_mark_as_allocated_block(block);
   prefer_in_recvd_out_size = ((size_type)block->m_size - AllocatedCtrlUnits)*Alignment + UsableByPreviousChunk;
   return true;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_prev_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *ptr)
{
   BOOST_ASSERT(!ptr->m_prev_allocated);
   return reinterpret_cast<block_ctrl *>
      (reinterpret_cast<char*>(ptr) - ptr->m_prev_size*Alignment);
}



template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_end_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *first_segment_block)
{
   //The first block's logic is different from the rest of blocks: stores in m_prev_size the absolute
   //distance with the end block
   BOOST_ASSERT(first_segment_block->m_prev_allocated);
   block_ctrl *end_block = reinterpret_cast<block_ctrl *>
      (reinterpret_cast<char*>(first_segment_block) + first_segment_block->m_prev_size*Alignment);
   (void)end_block;
   BOOST_ASSERT(end_block->m_allocated == 1);
   BOOST_ASSERT(end_block->m_size == first_segment_block->m_prev_size);
   BOOST_ASSERT(end_block > first_segment_block);
   return end_block;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_first_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *end_segment_block)
{
   //The first block's logic is different from the rest of blocks: stores in m_prev_size the absolute
   //distance with the end block
   BOOST_ASSERT(end_segment_block->m_allocated);
   block_ctrl *first_block = reinterpret_cast<block_ctrl *>
      (reinterpret_cast<char*>(end_segment_block) - end_segment_block->m_size*Alignment);
   (void)first_block;
   BOOST_ASSERT(first_block->m_prev_allocated == 1);
   BOOST_ASSERT(first_block->m_prev_size == end_segment_block->m_size);
   BOOST_ASSERT(end_segment_block > first_block);
   return first_block;
}


template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_next_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *ptr)
{
   return reinterpret_cast<block_ctrl *>
      (reinterpret_cast<char*>(ptr) + ptr->m_size*Alignment);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
bool rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_is_allocated_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *block)
{
   bool allocated = block->m_allocated != 0;
   #ifndef NDEBUG
   if(block != priv_end_block()){
      block_ctrl *next_block = reinterpret_cast<block_ctrl *>
         (reinterpret_cast<char*>(block) + block->m_size*Alignment);
      bool next_block_prev_allocated = next_block->m_prev_allocated != 0;
      (void)next_block_prev_allocated;
      BOOST_ASSERT(allocated == next_block_prev_allocated);
   }
   #endif
   return allocated;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
bool rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_is_prev_allocated
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *block)
{
   if(block->m_prev_allocated){
      return true;
   }
   else{
      #ifndef NDEBUG
      if(block != priv_first_block()){
         block_ctrl *prev = priv_prev_block(block);
         (void)prev;
         BOOST_ASSERT(!prev->m_allocated);
         BOOST_ASSERT(prev->m_size == block->m_prev_size);
      }
      #endif
      return false;
   }
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_mark_as_allocated_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *block)
{
   block->m_allocated = 1;
   reinterpret_cast<block_ctrl *>
      (reinterpret_cast<char*>(block)+ block->m_size*Alignment)->m_prev_allocated = 1;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_mark_as_free_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl *block)
{
   block->m_allocated = 0;
   block_ctrl *next_block = priv_next_block(block);
   next_block->m_prev_allocated = 0;
   next_block->m_prev_size = block->m_size;
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment> inline
void* rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_check_and_allocate
   (size_type nunits
   ,typename rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::block_ctrl* block
   ,size_type &received_size)
{
   size_type upper_nunits = nunits + BlockCtrlUnits;
   imultiset_iterator it_old = Imultiset::s_iterator_to(*block);
   algo_impl_t::assert_alignment(block);

   if (block->m_size >= upper_nunits){
      //This block is bigger than needed, split it in
      //two blocks, the first's size will be "units" and
      //the second's size "block->m_size-units"
      size_type block_old_size = block->m_size;
      block->m_size = nunits;
      BOOST_ASSERT(block->m_size >= BlockCtrlUnits);

      //This is the remaining block
      block_ctrl *rem_block = ::new(reinterpret_cast<block_ctrl*>
                     (reinterpret_cast<char*>(block) + Alignment*nunits), boost_container_new_t())block_ctrl;
      algo_impl_t::assert_alignment(rem_block);
      rem_block->m_size  = block_old_size - nunits;
      BOOST_ASSERT(rem_block->m_size >= BlockCtrlUnits);
      priv_mark_as_free_block(rem_block);

      imultiset_iterator it_hint;
      if(it_old == m_header.m_imultiset.begin()
         || (--imultiset_iterator(it_old))->m_size <= rem_block->m_size){
         //option a: slow but secure
         //m_header.m_imultiset.insert(m_header.m_imultiset.erase(it_old), *rem_block);
         //option b: Construct an empty node and swap
         //Imultiset::init_node(*rem_block);
         //block->swap_nodes(*rem_block);
         //option c: replace the node directly
         m_header.m_imultiset.replace_node(Imultiset::s_iterator_to(*it_old), *rem_block);
      }
      else{
         //Now we have to update the data in the tree
         m_header.m_imultiset.erase(it_old);
         m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *rem_block);
      }

   }
   else if (block->m_size >= nunits){
      m_header.m_imultiset.erase(it_old);
   }
   else{
      BOOST_ASSERT(0);
      return 0;
   }
   //We need block_ctrl for deallocation stuff, so
   //return memory user can overwrite
   m_header.m_allocated += (size_type)block->m_size*Alignment;
   received_size =  ((size_type)block->m_size - AllocatedCtrlUnits)*Alignment + UsableByPreviousChunk;

   //Mark the block as allocated
   priv_mark_as_allocated_block(block);

   //Clear the memory occupied by the tree hook, since this won't be
   //cleared with zero_free_memory
   TreeHook *t = static_cast<TreeHook*>(block);
   //Just clear the memory part reserved for the user
   std::size_t tree_hook_offset_in_block = (char*)t - (char*)block;
   //volatile char *ptr =
   char *ptr = reinterpret_cast<char*>(block)+tree_hook_offset_in_block;
   const std::size_t s = BlockCtrlBytes - tree_hook_offset_in_block;
   std::memset(ptr, 0, s);
   this->priv_next_block(block)->m_prev_size = 0;
   return priv_get_user_buffer(block);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::deallocate(void* addr)
{
   if(!addr)   return;
   //-----------------------
   boost::interprocess::scoped_lock<mutex_type> guard(m_header);
   //-----------------------
   return this->priv_deallocate(addr);
}

template<class MutexFamily, class VoidPointer, std::size_t MemAlignment>
void rbtree_best_fit<MutexFamily, VoidPointer, MemAlignment>::priv_deallocate(void* addr)
{
   if(!addr)   return;

   block_ctrl *block = priv_get_block(addr);

   //The blocks must be marked as allocated and the sizes must be equal
   BOOST_ASSERT(priv_is_allocated_block(block));

   //Check if alignment and block size are right
   algo_impl_t::assert_alignment(addr);

   size_type block_old_size = Alignment*(size_type)block->m_size;
   BOOST_ASSERT(m_header.m_allocated >= block_old_size);

   //Update used memory count
   m_header.m_allocated -= block_old_size;

   //The block to insert in the tree
   block_ctrl *block_to_insert = block;

   //Get the next block
   block_ctrl *const next_block  = priv_next_block(block);
   const bool merge_with_prev    = !priv_is_prev_allocated(block);
   const bool merge_with_next    = !priv_is_allocated_block(next_block);

   //Merge logic. First just update block sizes, then fix free blocks tree
   if(merge_with_prev || merge_with_next){
      //Merge if the previous is free
      if(merge_with_prev){
         //Get the previous block
         block_to_insert = priv_prev_block(block);
         block_to_insert->m_size += block->m_size;
         BOOST_ASSERT(block_to_insert->m_size >= BlockCtrlUnits);
      }
      //Merge if the next is free
      if(merge_with_next){
         block_to_insert->m_size += next_block->m_size;
         BOOST_ASSERT(block_to_insert->m_size >= BlockCtrlUnits);
         const imultiset_iterator next_it = Imultiset::s_iterator_to(*next_block);
         if(merge_with_prev){
            m_header.m_imultiset.erase(next_it);
         }
         else{
            m_header.m_imultiset.replace_node(next_it, *block_to_insert);
         }
      }

      //Now try to shortcut erasure + insertion (O(log(N))) with
      //a O(1) operation if merging does not alter tree positions
      const imultiset_iterator block_to_check_it = Imultiset::s_iterator_to(*block_to_insert);
      imultiset_const_iterator next_to_check_it(block_to_check_it), end_it(m_header.m_imultiset.end());

      if(++next_to_check_it != end_it && block_to_insert->m_size > next_to_check_it->m_size){
         //Block is bigger than next, so move it
         m_header.m_imultiset.erase(block_to_check_it);
         m_header.m_imultiset.insert(end_it, *block_to_insert);
      }
      else{
         //Block size increment didn't violate tree invariants so there is nothing to fix
      }
   }
   else{
      m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *block_to_insert);
   }
   priv_mark_as_free_block(block_to_insert);
}

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP
