//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MEM_ALGO_DETAIL_SIMPLE_SEQ_FIT_IMPL_HPP
#define BOOST_INTERPROCESS_MEM_ALGO_DETAIL_SIMPLE_SEQ_FIT_IMPL_HPP

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
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/min_max.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/interprocess/mem_algo/detail/mem_algo_common.hpp>
#include <boost/move/detail/type_traits.hpp> //make_unsigned, alignment_of
#include <boost/intrusive/detail/minimal_pair_header.hpp>
#include <cstring>
#include <boost/assert.hpp>

//!\file
//!Describes sequential fit algorithm used to allocate objects in shared memory.
//!This class is intended as a base class for single segment and multi-segment
//!implementations.

namespace boost {
namespace interprocess {
namespace ipcdetail {

//!This class implements the simple sequential fit algorithm with a simply
//!linked list of free buffers.
//!This class is intended as a base class for single segment and multi-segment
//!implementations.
template<class MutexFamily, class VoidPointer>
class simple_seq_fit_impl
{
   //Non-copyable
   simple_seq_fit_impl();
   simple_seq_fit_impl(const simple_seq_fit_impl &);
   simple_seq_fit_impl &operator=(const simple_seq_fit_impl &);

   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<char>::type                         char_ptr;

   public:

   //!Shared interprocess_mutex family used for the rest of the Interprocess framework
   typedef MutexFamily        mutex_family;
   //!Pointer type to be used with the rest of the Interprocess framework
   typedef VoidPointer        void_pointer;
   typedef boost::container::dtl::
      basic_multiallocation_chain<VoidPointer>     multiallocation_chain;

   typedef typename boost::intrusive::pointer_traits<char_ptr>::difference_type difference_type;
   typedef typename boost::container::dtl::make_unsigned<difference_type>::type size_type;


   private:
   class block_ctrl;
   friend class block_ctrl;

   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<block_ctrl>::type                   block_ctrl_ptr;

   //!Block control structure
   class block_ctrl
   {
      public:
      //!Offset pointer to the next block.
      block_ctrl_ptr m_next;
      //!This block's memory size (including block_ctrl
      //!header) in BasicSize units
      size_type    m_size;

      size_type get_user_bytes() const
      {  return this->m_size*Alignment - BlockCtrlBytes; }

      size_type get_total_bytes() const
      {  return this->m_size*Alignment; }
   };

   //!Shared interprocess_mutex to protect memory allocate/deallocate
   typedef typename MutexFamily::mutex_type        interprocess_mutex;

   //!This struct includes needed data and derives from
   //!interprocess_mutex to allow EBO when using null interprocess_mutex
   struct header_t : public interprocess_mutex
   {
      //!Pointer to the first free block
      block_ctrl        m_root;
      //!Allocated bytes for internal checking
      size_type         m_allocated;
      //!The size of the memory segment
      size_type         m_size;
      //!The extra size required by the segment
      size_type         m_extra_hdr_bytes;
   }  m_header;

   friend class ipcdetail::memory_algorithm_common<simple_seq_fit_impl>;

   typedef ipcdetail::memory_algorithm_common<simple_seq_fit_impl> algo_impl_t;

   public:
   //!Constructor. "size" is the total size of the managed memory segment,
   //!"extra_hdr_bytes" indicates the extra bytes beginning in the sizeof(simple_seq_fit_impl)
   //!offset that the allocator should not use at all.
   simple_seq_fit_impl           (size_type size, size_type extra_hdr_bytes);

   //!Destructor
   ~simple_seq_fit_impl();

   //!Obtains the minimum size needed by the algorithm
   static size_type get_min_size (size_type extra_hdr_bytes);

   //Functions for single segment management

   //!Allocates bytes, returns 0 if there is not more memory
   void* allocate             (size_type nbytes);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Multiple element allocation, same size
   void allocate_many(size_type elem_bytes, size_type num_elements, multiallocation_chain &chain)
   {
      //-----------------------
      boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
      //-----------------------
      algo_impl_t::allocate_many(this, elem_bytes, num_elements, chain);
   }

   //!Multiple element allocation, different size
   void allocate_many(const size_type *elem_sizes, size_type n_elements, size_type sizeof_element, multiallocation_chain &chain)
   {
      //-----------------------
      boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
      //-----------------------
      algo_impl_t::allocate_many(this, elem_sizes, n_elements, sizeof_element, chain);
   }

   //!Multiple element deallocation
   void deallocate_many(multiallocation_chain &chain);

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   //!Deallocates previously allocated bytes
   void   deallocate          (void *addr);

   //!Returns the size of the memory segment
   size_type get_size()  const;

   //!Returns the number of free bytes of the memory segment
   size_type get_free_memory()  const;

   //!Increases managed memory in extra_size bytes more
   void grow(size_type extra_size);

   //!Decreases managed memory as much as possible
   void shrink_to_fit();

   //!Returns true if all allocated memory has been deallocated
   bool all_memory_deallocated();

   //!Makes an internal sanity check and returns true if success
   bool check_sanity();

   //!Initializes to zero all the memory that's not in use.
   //!This function is normally used for security reasons.
   void zero_free_memory();

   template<class T>
   T *allocation_command  (boost::interprocess::allocation_type command,   size_type limit_size,
                           size_type &prefer_in_recvd_out_size, T *&reuse);

   void * raw_allocation_command  (boost::interprocess::allocation_type command,   size_type limit_size,
                               size_type &prefer_in_recvd_out_size, void *&reuse_ptr, size_type sizeof_object = 1);

   //!Returns the size of the buffer previously allocated pointed by ptr
   size_type size(const void *ptr) const;

   //!Allocates aligned bytes, returns 0 if there is not more memory.
   //!Alignment must be power of 2
   void* allocate_aligned     (size_type nbytes, size_type alignment);

   private:

   //!Obtains the pointer returned to the user from the block control
   static void *priv_get_user_buffer(const block_ctrl *block);

   //!Obtains the block control structure of the user buffer
   static block_ctrl *priv_get_block(const void *ptr);

   //!Real allocation algorithm with min allocation option
   void * priv_allocate(boost::interprocess::allocation_type command
                        ,size_type min_size
                        ,size_type &prefer_in_recvd_out_size, void *&reuse_ptr);

   void * priv_allocation_command(boost::interprocess::allocation_type command
                                 ,size_type min_size
                                 ,size_type &prefer_in_recvd_out_size
                                 ,void *&reuse_ptr
                                 ,size_type sizeof_object);

   //!Returns the number of total units that a user buffer
   //!of "userbytes" bytes really occupies (including header)
   static size_type priv_get_total_units(size_type userbytes);

   static size_type priv_first_block_offset(const void *this_ptr, size_type extra_hdr_bytes);
   size_type priv_block_end_offset() const;

   //!Returns next block if it's free.
   //!Returns 0 if next block is not free.
   block_ctrl *priv_next_block_if_free(block_ctrl *ptr);

   //!Check if this block is free (not allocated)
   bool priv_is_allocated_block(block_ctrl *ptr);

   //!Returns previous block's if it's free.
   //!Returns 0 if previous block is not free.
   std::pair<block_ctrl*, block_ctrl*> priv_prev_block_if_free(block_ctrl *ptr);

   //!Real expand function implementation
   bool priv_expand(void *ptr, size_type min_size, size_type &prefer_in_recvd_out_size);

   //!Real expand to both sides implementation
   void* priv_expand_both_sides(boost::interprocess::allocation_type command
                               ,size_type min_size, size_type &prefer_in_recvd_out_size
                               ,void *reuse_ptr
                               ,bool only_preferred_backwards);

   //!Real private aligned allocation function
   //void* priv_allocate_aligned     (size_type nbytes, size_type alignment);

   //!Checks if block has enough memory and splits/unlinks the block
   //!returning the address to the users
   void* priv_check_and_allocate(size_type units
                                ,block_ctrl* prev
                                ,block_ctrl* block
                                ,size_type &received_size);
   //!Real deallocation algorithm
   void priv_deallocate(void *addr);

   //!Makes a new memory portion available for allocation
   void priv_add_segment(void *addr, size_type size);

   void priv_mark_new_allocated_block(block_ctrl *block);

   public:
   static const size_type Alignment      = ::boost::container::dtl::alignment_of
      < ::boost::container::dtl::max_align_t>::value;
   private:
   static const size_type BlockCtrlBytes = ipcdetail::ct_rounded_size<sizeof(block_ctrl), Alignment>::value;
   static const size_type BlockCtrlUnits = BlockCtrlBytes/Alignment;
   static const size_type MinBlockUnits  = BlockCtrlUnits;
   static const size_type MinBlockSize   = MinBlockUnits*Alignment;
   static const size_type AllocatedCtrlBytes = BlockCtrlBytes;
   static const size_type AllocatedCtrlUnits = BlockCtrlUnits;
   static const size_type UsableByPreviousChunk = 0;

   public:
   static const size_type PayloadPerAllocation = BlockCtrlBytes;
};

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>
   ::priv_first_block_offset(const void *this_ptr, size_type extra_hdr_bytes)
{
   //First align "this" pointer
   size_type uint_this         = (std::size_t)this_ptr;
   size_type uint_aligned_this = uint_this/Alignment*Alignment;
   size_type this_disalignment = (uint_this - uint_aligned_this);
   size_type block1_off =
      ipcdetail::get_rounded_size(sizeof(simple_seq_fit_impl) + extra_hdr_bytes + this_disalignment, Alignment)
      - this_disalignment;
   algo_impl_t::assert_alignment(this_disalignment + block1_off);
   return block1_off;
}

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>
   ::priv_block_end_offset() const
{
   //First align "this" pointer
   size_type uint_this         = (std::size_t)this;
   size_type uint_aligned_this = uint_this/Alignment*Alignment;
   size_type this_disalignment = (uint_this - uint_aligned_this);
   size_type old_end =
      ipcdetail::get_truncated_size(m_header.m_size + this_disalignment, Alignment)
      - this_disalignment;
   algo_impl_t::assert_alignment(old_end + this_disalignment);
   return old_end;
}

template<class MutexFamily, class VoidPointer>
inline simple_seq_fit_impl<MutexFamily, VoidPointer>::
   simple_seq_fit_impl(size_type segment_size, size_type extra_hdr_bytes)
{
   //Initialize sizes and counters
   m_header.m_allocated = 0;
   m_header.m_size      = segment_size;
   m_header.m_extra_hdr_bytes = extra_hdr_bytes;

   //Initialize pointers
   size_type block1_off = priv_first_block_offset(this, extra_hdr_bytes);

   m_header.m_root.m_next  = reinterpret_cast<block_ctrl*>
      ((reinterpret_cast<char*>(this) + block1_off));
   algo_impl_t::assert_alignment(ipcdetail::to_raw_pointer(m_header.m_root.m_next));
   m_header.m_root.m_next->m_size  = (segment_size - block1_off)/Alignment;
   m_header.m_root.m_next->m_next  = &m_header.m_root;
}

template<class MutexFamily, class VoidPointer>
inline simple_seq_fit_impl<MutexFamily, VoidPointer>::~simple_seq_fit_impl()
{
   //There is a memory leak!
//   BOOST_ASSERT(m_header.m_allocated == 0);
//   BOOST_ASSERT(m_header.m_root.m_next->m_next == block_ctrl_ptr(&m_header.m_root));
}

template<class MutexFamily, class VoidPointer>
inline void simple_seq_fit_impl<MutexFamily, VoidPointer>::grow(size_type extra_size)
{
   //Old highest address block's end offset
   size_type old_end = this->priv_block_end_offset();

   //Update managed buffer's size
   m_header.m_size += extra_size;

   //We need at least MinBlockSize blocks to create a new block
   if((m_header.m_size - old_end) < MinBlockSize){
      return;
   }

   //We'll create a new free block with extra_size bytes

   block_ctrl *new_block = reinterpret_cast<block_ctrl*>
      (reinterpret_cast<char*>(this) + old_end);

   algo_impl_t::assert_alignment(new_block);
   new_block->m_next = 0;
   new_block->m_size = (m_header.m_size - old_end)/Alignment;
   m_header.m_allocated += new_block->m_size*Alignment;
   this->priv_deallocate(priv_get_user_buffer(new_block));
}

template<class MutexFamily, class VoidPointer>
void simple_seq_fit_impl<MutexFamily, VoidPointer>::shrink_to_fit()
{
   //Get the root and the first memory block
   block_ctrl *prev                 = &m_header.m_root;
   block_ctrl *last                 = &m_header.m_root;
   block_ctrl *block                = ipcdetail::to_raw_pointer(last->m_next);
   block_ctrl *root                 = &m_header.m_root;

   //No free block?
   if(block == root) return;

   //Iterate through the free block list
   while(block != root){
      prev  = last;
      last  = block;
      block = ipcdetail::to_raw_pointer(block->m_next);
   }

   char *last_free_end_address   = reinterpret_cast<char*>(last) + last->m_size*Alignment;
   if(last_free_end_address != (reinterpret_cast<char*>(this) + priv_block_end_offset())){
      //there is an allocated block in the end of this block
      //so no shrinking is possible
      return;
   }

   //Check if have only 1 big free block
   void *unique_block = 0;
   if(!m_header.m_allocated){
      BOOST_ASSERT(prev == root);
      size_type ignore_recvd = 0;
      void *ignore_reuse = 0;
      unique_block = priv_allocate(boost::interprocess::allocate_new, 0, ignore_recvd, ignore_reuse);
      if(!unique_block)
         return;
      last = ipcdetail::to_raw_pointer(m_header.m_root.m_next);
      BOOST_ASSERT(last_free_end_address == (reinterpret_cast<char*>(last) + last->m_size*Alignment));
   }
   size_type last_units = last->m_size;

   size_type received_size;
   void *addr = priv_check_and_allocate(last_units, prev, last, received_size);
   (void)addr;
   BOOST_ASSERT(addr);
   BOOST_ASSERT(received_size == last_units*Alignment - AllocatedCtrlBytes);

   //Shrink it
   m_header.m_size /= Alignment;
   m_header.m_size -= last->m_size;
   m_header.m_size *= Alignment;
   m_header.m_allocated -= last->m_size*Alignment;

   if(unique_block)
      priv_deallocate(unique_block);
}

template<class MutexFamily, class VoidPointer>
inline void simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_mark_new_allocated_block(block_ctrl *new_block)
{
   new_block->m_next = 0;
}

template<class MutexFamily, class VoidPointer>
inline
typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *
   simple_seq_fit_impl<MutexFamily, VoidPointer>::priv_get_block(const void *ptr)
{
   return const_cast<block_ctrl*>(reinterpret_cast<const block_ctrl*>
      (reinterpret_cast<const char*>(ptr) - AllocatedCtrlBytes));
}

template<class MutexFamily, class VoidPointer>
inline
void *simple_seq_fit_impl<MutexFamily, VoidPointer>::
      priv_get_user_buffer(const typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *block)
{
   return const_cast<char*>(reinterpret_cast<const char*>(block) + AllocatedCtrlBytes);
}

template<class MutexFamily, class VoidPointer>
inline void simple_seq_fit_impl<MutexFamily, VoidPointer>::priv_add_segment(void *addr, size_type segment_size)
{
   algo_impl_t::assert_alignment(addr);
   //Check size
   BOOST_ASSERT(!(segment_size < MinBlockSize));
   if(segment_size < MinBlockSize)
      return;
   //Construct big block using the new segment
   block_ctrl *new_block   = static_cast<block_ctrl *>(addr);
   new_block->m_size       = segment_size/Alignment;
   new_block->m_next       = 0;
   //Simulate this block was previously allocated
   m_header.m_allocated   += new_block->m_size*Alignment;
   //Return block and insert it in the free block list
   this->priv_deallocate(priv_get_user_buffer(new_block));
}

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>::get_size()  const
   {  return m_header.m_size;  }

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>::get_free_memory()  const
{
   return m_header.m_size - m_header.m_allocated -
      algo_impl_t::multiple_of_units(sizeof(*this) + m_header.m_extra_hdr_bytes);
}

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>::
   get_min_size (size_type extra_hdr_bytes)
{
   return ipcdetail::get_rounded_size((size_type)sizeof(simple_seq_fit_impl),Alignment) +
          ipcdetail::get_rounded_size(extra_hdr_bytes,Alignment)
          + MinBlockSize;
}

template<class MutexFamily, class VoidPointer>
inline bool simple_seq_fit_impl<MutexFamily, VoidPointer>::
    all_memory_deallocated()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return m_header.m_allocated == 0 &&
          ipcdetail::to_raw_pointer(m_header.m_root.m_next->m_next) == &m_header.m_root;
}

template<class MutexFamily, class VoidPointer>
inline void simple_seq_fit_impl<MutexFamily, VoidPointer>::zero_free_memory()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   block_ctrl *block = ipcdetail::to_raw_pointer(m_header.m_root.m_next);

   //Iterate through all free portions
   do{
      //Just clear user the memory part reserved for the user
      std::memset( priv_get_user_buffer(block)
                 , 0
             , block->get_user_bytes());
      block = ipcdetail::to_raw_pointer(block->m_next);
   }
   while(block != &m_header.m_root);
}

template<class MutexFamily, class VoidPointer>
inline bool simple_seq_fit_impl<MutexFamily, VoidPointer>::
    check_sanity()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   block_ctrl *block = ipcdetail::to_raw_pointer(m_header.m_root.m_next);

   size_type free_memory = 0;

   //Iterate through all blocks obtaining their size
   while(block != &m_header.m_root){
      algo_impl_t::assert_alignment(block);
      if(!algo_impl_t::check_alignment(block))
         return false;
      //Free blocks's next must be always valid
      block_ctrl *next = ipcdetail::to_raw_pointer(block->m_next);
      if(!next){
         return false;
      }
      free_memory += block->m_size*Alignment;
      block = next;
   }

   //Check allocated bytes are less than size
   if(m_header.m_allocated > m_header.m_size){
      return false;
   }

   //Check free bytes are less than size
   if(free_memory > m_header.m_size){
      return false;
   }
   return true;
}

template<class MutexFamily, class VoidPointer>
inline void* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   allocate(size_type nbytes)
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   size_type ignore_recvd = nbytes;
   void *ignore_reuse = 0;
   return priv_allocate(boost::interprocess::allocate_new, nbytes, ignore_recvd, ignore_reuse);
}

template<class MutexFamily, class VoidPointer>
inline void* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   allocate_aligned(size_type nbytes, size_type alignment)
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return algo_impl_t::
      allocate_aligned(this, nbytes, alignment);
}

template<class MutexFamily, class VoidPointer>
template<class T>
inline T* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   allocation_command  (boost::interprocess::allocation_type command,   size_type limit_size,
                        size_type &prefer_in_recvd_out_size, T *&reuse_ptr)
{
   void *raw_reuse = reuse_ptr;
   void * const ret = priv_allocation_command
      (command, limit_size, prefer_in_recvd_out_size, raw_reuse, sizeof(T));
   BOOST_ASSERT(0 == ((std::size_t)ret % ::boost::container::dtl::alignment_of<T>::value));
   reuse_ptr = static_cast<T*>(raw_reuse);
   return static_cast<T*>(ret);
}

template<class MutexFamily, class VoidPointer>
inline void* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   raw_allocation_command  (boost::interprocess::allocation_type command, size_type limit_objects,
                        size_type &prefer_in_recvd_out_size, void *&reuse_ptr, size_type sizeof_object)
{
   size_type const preferred_objects = prefer_in_recvd_out_size;
   if(!sizeof_object){
      return reuse_ptr = 0, static_cast<void*>(0);
  }
   if(command & boost::interprocess::try_shrink_in_place){
      if(!reuse_ptr) return static_cast<void*>(0);
      prefer_in_recvd_out_size = preferred_objects*sizeof_object;
      bool success = algo_impl_t::try_shrink
         ( this, reuse_ptr, limit_objects*sizeof_object, prefer_in_recvd_out_size);
      prefer_in_recvd_out_size /= sizeof_object;
      return success ? reuse_ptr : 0;
   }
   else{
      return priv_allocation_command
         (command, limit_objects, prefer_in_recvd_out_size, reuse_ptr, sizeof_object);
   }
}

template<class MutexFamily, class VoidPointer>
inline void* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_allocation_command (boost::interprocess::allocation_type command,   size_type limit_size,
                       size_type &prefer_in_recvd_out_size, void *&reuse_ptr, size_type sizeof_object)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   command &= ~boost::interprocess::expand_bwd;
   if(!command){
      return reuse_ptr = 0, static_cast<void*>(0);
   }

   size_type max_count = m_header.m_size/sizeof_object;
   if(limit_size > max_count || preferred_size > max_count){
      return reuse_ptr = 0, static_cast<void*>(0);
   }
   size_type l_size = limit_size*sizeof_object;
   size_type r_size = preferred_size*sizeof_object;
   void *ret = 0;
   {
      //-----------------------
      boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
      //-----------------------
      ret = priv_allocate(command, l_size, r_size, reuse_ptr);
   }
   prefer_in_recvd_out_size = r_size/sizeof_object;
   return ret;
}

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>::size(const void *ptr) const
{
   //We need no synchronization since this block is not going
   //to be modified
   //Obtain the real size of the block
   const block_ctrl *block = static_cast<const block_ctrl*>(priv_get_block(ptr));
   return block->get_user_bytes();
}

template<class MutexFamily, class VoidPointer>
void* simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_expand_both_sides(boost::interprocess::allocation_type command
                         ,size_type min_size
                         ,size_type &prefer_in_recvd_out_size
                         ,void *reuse_ptr
                         ,bool only_preferred_backwards)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   typedef std::pair<block_ctrl *, block_ctrl *> prev_block_t;
   block_ctrl *reuse = priv_get_block(reuse_ptr);
   prefer_in_recvd_out_size = 0;

   if(this->size(reuse_ptr) > min_size){
      prefer_in_recvd_out_size = this->size(reuse_ptr);
      return reuse_ptr;
   }

   if(command & boost::interprocess::expand_fwd){
      if(priv_expand(reuse_ptr, min_size, prefer_in_recvd_out_size = preferred_size))
         return reuse_ptr;
   }
   else{
      prefer_in_recvd_out_size = this->size(reuse_ptr);
   }
   if(command & boost::interprocess::expand_bwd){
      size_type extra_forward = !prefer_in_recvd_out_size ? 0 : prefer_in_recvd_out_size + BlockCtrlBytes;
      prev_block_t prev_pair = priv_prev_block_if_free(reuse);
      block_ctrl *prev = prev_pair.second;
      if(!prev){
         return 0;
      }

      size_type needs_backwards =
         ipcdetail::get_rounded_size(preferred_size - extra_forward, Alignment);

      if(!only_preferred_backwards){
            max_value(ipcdetail::get_rounded_size(min_size - extra_forward, Alignment)
                     ,min_value(prev->get_user_bytes(), needs_backwards));
      }

      //Check if previous block has enough size
      if((prev->get_user_bytes()) >=  needs_backwards){
         //Now take all next space. This will succeed
         if(!priv_expand(reuse_ptr, prefer_in_recvd_out_size, prefer_in_recvd_out_size)){
            BOOST_ASSERT(0);
         }

         //We need a minimum size to split the previous one
         if((prev->get_user_bytes() - needs_backwards) > 2*BlockCtrlBytes){
             block_ctrl *new_block = reinterpret_cast<block_ctrl*>
                  (reinterpret_cast<char*>(reuse) - needs_backwards - BlockCtrlBytes);

            new_block->m_next = 0;
            new_block->m_size =
               BlockCtrlUnits + (needs_backwards + extra_forward)/Alignment;
            prev->m_size =
               (prev->get_total_bytes() - needs_backwards)/Alignment - BlockCtrlUnits;
            prefer_in_recvd_out_size = needs_backwards + extra_forward;
            m_header.m_allocated += needs_backwards + BlockCtrlBytes;
            return priv_get_user_buffer(new_block);
         }
         else{
            //Just merge the whole previous block
            block_ctrl *prev_2_block = prev_pair.first;
            //Update received size and allocation
            prefer_in_recvd_out_size = extra_forward + prev->get_user_bytes();
            m_header.m_allocated += prev->get_total_bytes();
            //Now unlink it from previous block
            prev_2_block->m_next = prev->m_next;
            prev->m_size = reuse->m_size + prev->m_size;
            prev->m_next = 0;
            priv_get_user_buffer(prev);
         }
      }
   }
   return 0;
}

template<class MutexFamily, class VoidPointer>
inline void simple_seq_fit_impl<MutexFamily, VoidPointer>::
   deallocate_many(typename simple_seq_fit_impl<MutexFamily, VoidPointer>::multiallocation_chain &chain)
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   while(!chain.empty()){
      this->priv_deallocate(to_raw_pointer(chain.pop_front()));
   }
}

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::size_type
simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_get_total_units(size_type userbytes)
{
   size_type s = ipcdetail::get_rounded_size(userbytes, Alignment)/Alignment;
   if(!s)   ++s;
   return BlockCtrlUnits + s;
}

template<class MutexFamily, class VoidPointer>
void * simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_allocate(boost::interprocess::allocation_type command
                ,size_type limit_size, size_type &prefer_in_recvd_out_size, void *&reuse_ptr)
{
   size_type const preferred_size = prefer_in_recvd_out_size;
   if(command & boost::interprocess::shrink_in_place){
      if(!reuse_ptr)  return static_cast<void*>(0);
      bool success = algo_impl_t::shrink(this, reuse_ptr, limit_size, prefer_in_recvd_out_size);
      return success ? reuse_ptr : 0;
   }
   prefer_in_recvd_out_size = 0;

   if(limit_size > preferred_size){
      return reuse_ptr = 0, static_cast<void*>(0);
   }

   //Number of units to request (including block_ctrl header)
   size_type nunits = ipcdetail::get_rounded_size(preferred_size, Alignment)/Alignment + BlockCtrlUnits;

   //Get the root and the first memory block
   block_ctrl *prev                 = &m_header.m_root;
   block_ctrl *block                = ipcdetail::to_raw_pointer(prev->m_next);
   block_ctrl *root                 = &m_header.m_root;
   block_ctrl *biggest_block        = 0;
   block_ctrl *prev_biggest_block   = 0;
   size_type biggest_size         = 0;

   //Expand in place
   if(reuse_ptr && (command & (boost::interprocess::expand_fwd | boost::interprocess::expand_bwd))){
      void *ret = priv_expand_both_sides(command, limit_size, prefer_in_recvd_out_size = preferred_size, reuse_ptr, true);
      if(ret){
         algo_impl_t::assert_alignment(ret);
         return ret;
      }
   }

   if(command & boost::interprocess::allocate_new){
      prefer_in_recvd_out_size = 0;
      while(block != root){
         //Update biggest block pointers
         if(block->m_size > biggest_size){
            prev_biggest_block = prev;
            biggest_size  = block->m_size;
            biggest_block = block;
         }
         algo_impl_t::assert_alignment(block);
         void *addr = this->priv_check_and_allocate(nunits, prev, block, prefer_in_recvd_out_size);
         if(addr){
            algo_impl_t::assert_alignment(addr);
            return reuse_ptr = 0, addr;
         }
         //Bad luck, let's check next block
         prev  = block;
         block = ipcdetail::to_raw_pointer(block->m_next);
      }

      //Bad luck finding preferred_size, now if we have any biggest_block
      //try with this block
      if(biggest_block){
         size_type limit_units = ipcdetail::get_rounded_size(limit_size, Alignment)/Alignment + BlockCtrlUnits;
         if(biggest_block->m_size < limit_units){
            return reuse_ptr = 0, static_cast<void*>(0);
         }
         void *ret = this->priv_check_and_allocate
            (biggest_block->m_size, prev_biggest_block, biggest_block, prefer_in_recvd_out_size = biggest_block->m_size*Alignment - BlockCtrlUnits);
         BOOST_ASSERT(ret != 0);
         algo_impl_t::assert_alignment(ret);
         return reuse_ptr = 0, ret;
      }
   }
   //Now try to expand both sides with min size
   if(reuse_ptr && (command & (boost::interprocess::expand_fwd | boost::interprocess::expand_bwd))){
      void *ret = priv_expand_both_sides (command, limit_size, prefer_in_recvd_out_size = preferred_size, reuse_ptr, false);
      algo_impl_t::assert_alignment(ret);
      return ret;
   }
   return reuse_ptr = 0, static_cast<void*>(0);
}

template<class MutexFamily, class VoidPointer> inline
bool simple_seq_fit_impl<MutexFamily, VoidPointer>::priv_is_allocated_block
      (typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *block)
{  return block->m_next == 0;  }

template<class MutexFamily, class VoidPointer>
inline typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *
   simple_seq_fit_impl<MutexFamily, VoidPointer>::
      priv_next_block_if_free
         (typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *ptr)
{
   //Take the address where the next block should go
   block_ctrl *next_block = reinterpret_cast<block_ctrl*>
      (reinterpret_cast<char*>(ptr) + ptr->m_size*Alignment);

   //Check if the adjacent block is in the managed segment
   char *this_char_ptr = reinterpret_cast<char*>(this);
   char *next_char_ptr = reinterpret_cast<char*>(next_block);
   size_type distance = (size_type)(next_char_ptr - this_char_ptr)/Alignment;

   if(distance >= (m_header.m_size/Alignment)){
      //"next_block" does not exist so we can't expand "block"
      return 0;
   }

   if(!next_block->m_next)
      return 0;

   return next_block;
}

template<class MutexFamily, class VoidPointer>
inline
   std::pair<typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *
            ,typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *>
   simple_seq_fit_impl<MutexFamily, VoidPointer>::
      priv_prev_block_if_free
         (typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl *ptr)
{
   typedef std::pair<block_ctrl *, block_ctrl *> prev_pair_t;
   //Take the address where the previous block should go
   block_ctrl *root           = &m_header.m_root;
   block_ctrl *prev_2_block   = root;
   block_ctrl *prev_block = ipcdetail::to_raw_pointer(root->m_next);

   while((reinterpret_cast<char*>(prev_block) + prev_block->m_size*Alignment)
            != reinterpret_cast<char*>(ptr)
         && prev_block != root){
      prev_2_block = prev_block;
      prev_block = ipcdetail::to_raw_pointer(prev_block->m_next);
   }

   if(prev_block == root || !prev_block->m_next)
      return prev_pair_t(static_cast<block_ctrl*>(0), static_cast<block_ctrl*>(0));

   //Check if the previous block is in the managed segment
   char *this_char_ptr = reinterpret_cast<char*>(this);
   char *prev_char_ptr = reinterpret_cast<char*>(prev_block);
   size_type distance = (size_type)(prev_char_ptr - this_char_ptr)/Alignment;

   if(distance >= (m_header.m_size/Alignment)){
      //"previous_block" does not exist so we can't expand "block"
      return prev_pair_t(static_cast<block_ctrl*>(0), static_cast<block_ctrl*>(0));
   }
   return prev_pair_t(prev_2_block, prev_block);
}


template<class MutexFamily, class VoidPointer>
inline bool simple_seq_fit_impl<MutexFamily, VoidPointer>::
   priv_expand (void *ptr, size_type min_size, size_type &received_size)
{
   size_type preferred_size = received_size;
   //Obtain the real size of the block
   block_ctrl *block = reinterpret_cast<block_ctrl*>(priv_get_block(ptr));
   size_type old_block_size = block->m_size;

   //All used blocks' next is marked with 0 so check it
   BOOST_ASSERT(block->m_next == 0);

   //Put this to a safe value
   received_size = old_block_size*Alignment - BlockCtrlBytes;

   //Now translate it to Alignment units
   min_size       = ipcdetail::get_rounded_size(min_size, Alignment)/Alignment;
   preferred_size = ipcdetail::get_rounded_size(preferred_size, Alignment)/Alignment;

   //Some parameter checks
   if(min_size > preferred_size)
      return false;

   size_type data_size = old_block_size - BlockCtrlUnits;

   if(data_size >= min_size)
      return true;

   block_ctrl *next_block = priv_next_block_if_free(block);
   if(!next_block){
      return false;
   }

   //Is "block" + "next_block" big enough?
   size_type merged_size = old_block_size + next_block->m_size;

   //Now we can expand this block further than before
   received_size = merged_size*Alignment - BlockCtrlBytes;

   if(merged_size < (min_size + BlockCtrlUnits)){
      return false;
   }

   //We can fill expand. Merge both blocks,
   block->m_next = next_block->m_next;
   block->m_size = merged_size;

   //Find the previous free block of next_block
   block_ctrl *prev = &m_header.m_root;
   while(ipcdetail::to_raw_pointer(prev->m_next) != next_block){
      prev = ipcdetail::to_raw_pointer(prev->m_next);
   }

   //Now insert merged block in the free list
   //This allows reusing allocation logic in this function
   m_header.m_allocated -= old_block_size*Alignment;
   prev->m_next = block;

   //Now use check and allocate to do the allocation logic
   preferred_size += BlockCtrlUnits;
   size_type nunits = preferred_size < merged_size ? preferred_size : merged_size;

   //This must success since nunits is less than merged_size!
   if(!this->priv_check_and_allocate (nunits, prev, block, received_size)){
      //Something very ugly is happening here. This is a bug
      //or there is memory corruption
      BOOST_ASSERT(0);
      return false;
   }
   return true;
}

template<class MutexFamily, class VoidPointer> inline
void* simple_seq_fit_impl<MutexFamily, VoidPointer>::priv_check_and_allocate
   (size_type nunits
   ,typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl* prev
   ,typename simple_seq_fit_impl<MutexFamily, VoidPointer>::block_ctrl* block
   ,size_type &received_size)
{
   size_type upper_nunits = nunits + BlockCtrlUnits;
   bool found = false;

   if (block->m_size > upper_nunits){
      //This block is bigger than needed, split it in
      //two blocks, the first's size will be "units"
      //the second's size will be "block->m_size-units"
      size_type total_size = block->m_size;
      block->m_size  = nunits;

      block_ctrl *new_block = reinterpret_cast<block_ctrl*>
         (reinterpret_cast<char*>(block) + Alignment*nunits);
      new_block->m_size  = total_size - nunits;
      new_block->m_next  = block->m_next;
      prev->m_next = new_block;
      found = true;
   }
   else if (block->m_size >= nunits){
      //This block has exactly the right size with an extra
      //unusable extra bytes.
      prev->m_next = block->m_next;
      found = true;
   }

   if(found){
      //We need block_ctrl for deallocation stuff, so
      //return memory user can overwrite
      m_header.m_allocated += block->m_size*Alignment;
      received_size =  block->get_user_bytes();
      //Mark the block as allocated
      block->m_next = 0;
      //Check alignment
      algo_impl_t::assert_alignment(block);
      return priv_get_user_buffer(block);
   }
   return 0;
}

template<class MutexFamily, class VoidPointer>
void simple_seq_fit_impl<MutexFamily, VoidPointer>::deallocate(void* addr)
{
   if(!addr)   return;
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return this->priv_deallocate(addr);
}

template<class MutexFamily, class VoidPointer>
void simple_seq_fit_impl<MutexFamily, VoidPointer>::priv_deallocate(void* addr)
{
   if(!addr)   return;

   //Let's get free block list. List is always sorted
   //by memory address to allow block merging.
   //Pointer next always points to the first
   //(lower address) block
   block_ctrl * prev  = &m_header.m_root;
   block_ctrl * pos   = ipcdetail::to_raw_pointer(m_header.m_root.m_next);
   block_ctrl * block = reinterpret_cast<block_ctrl*>(priv_get_block(addr));

   //All used blocks' next is marked with 0 so check it
   BOOST_ASSERT(block->m_next == 0);

   //Check if alignment and block size are right
   algo_impl_t::assert_alignment(addr);

   size_type total_size = Alignment*block->m_size;
   BOOST_ASSERT(m_header.m_allocated >= total_size);

   //Update used memory count
   m_header.m_allocated -= total_size;

   //Let's find the previous and the next block of the block to deallocate
   //This ordering comparison must be done with original pointers
   //types since their mapping to raw pointers can be different
   //in each process
   while((ipcdetail::to_raw_pointer(pos) != &m_header.m_root) && (block > pos)){
      prev = pos;
      pos = ipcdetail::to_raw_pointer(pos->m_next);
   }

   //Try to combine with upper block
   char *block_char_ptr = reinterpret_cast<char*>(ipcdetail::to_raw_pointer(block));

   if ((block_char_ptr + Alignment*block->m_size) ==
         reinterpret_cast<char*>(ipcdetail::to_raw_pointer(pos))){
      block->m_size += pos->m_size;
      block->m_next  = pos->m_next;
   }
   else{
      block->m_next = pos;
   }

   //Try to combine with lower block
   if ((reinterpret_cast<char*>(ipcdetail::to_raw_pointer(prev))
            + Alignment*prev->m_size) ==
        block_char_ptr){


      prev->m_size += block->m_size;
      prev->m_next  = block->m_next;
   }
   else{
      prev->m_next = block;
   }
}

}  //namespace ipcdetail {

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_MEM_ALGO_DETAIL_SIMPLE_SEQ_FIT_IMPL_HPP

