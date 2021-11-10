// Copyright (C) 2000, 2001 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_SIMPLE_SEGREGATED_STORAGE_HPP
#define BOOST_SIMPLE_SEGREGATED_STORAGE_HPP

/*!
  \file
  \brief Simple Segregated Storage.
  \details A simple segregated storage implementation:
  simple segregated storage is the basic idea behind the Boost Pool library.
  Simple segregated storage is the simplest, and probably the fastest,
  memory allocation/deallocation algorithm.
  It begins by partitioning a memory block into fixed-size chunks.
  Where the block comes from is not important until implementation time.
  A Pool is some object that uses Simple Segregated Storage in this fashion.
*/

// std::greater
#include <functional>

#include <boost/pool/poolfwd.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)  // Conditional expression is constant
#endif

#ifdef BOOST_POOL_VALIDATE
#  define BOOST_POOL_VALIDATE_INTERNALS validate();
#else
#  define BOOST_POOL_VALIDATE_INTERNALS
#endif

namespace boost {

/*! 

\brief Simple Segregated Storage is the simplest, and probably the fastest,
memory allocation/deallocation algorithm.  It is responsible for
partitioning a memory block into fixed-size chunks: where the block comes from 
is determined by the client of the class.

\details Template class simple_segregated_storage controls access to a free list of memory chunks. 
Please note that this is a very simple class, with preconditions on almost all its functions. It is intended to 
be the fastest and smallest possible quick memory allocator - e.g., something to use in embedded systems. 
This class delegates many difficult preconditions to the user (i.e., alignment issues).

An object of type simple_segregated_storage<SizeType>  is empty  if its free list is empty. 
If it is not empty, then it is ordered  if its free list is ordered. A free list is ordered if repeated calls 
to <tt>malloc()</tt> will result in a constantly-increasing sequence of values, as determined by <tt>std::less<void *></tt>. 
A member function is <i>order-preserving</i> if the free list maintains its order orientation (that is, an 
ordered free list is still ordered after the member function call).

*/
template <typename SizeType>
class simple_segregated_storage
{
  public:
    typedef SizeType size_type;

  private:
    simple_segregated_storage(const simple_segregated_storage &);
    void operator=(const simple_segregated_storage &);

    static void * try_malloc_n(void * & start, size_type n,
        size_type partition_size);

  protected:
    void * first; /*!< This data member is the free list.
      It points to the first chunk in the free list,
      or is equal to 0 if the free list is empty.
    */

    void * find_prev(void * ptr);

    // for the sake of code readability :)
    static void * & nextof(void * const ptr)
    { //! The return value is just *ptr cast to the appropriate type. ptr must not be 0. (For the sake of code readability :)
    //! As an example, let us assume that we want to truncate the free list after the first chunk.
    //! That is, we want to set *first to 0; this will result in a free list with only one entry.
    //! The normal way to do this is to first cast first to a pointer to a pointer to void,
    //! and then dereference and assign (*static_cast<void **>(first) = 0;).
    //! This can be done more easily through the use of this convenience function (nextof(first) = 0;).
    //! \returns dereferenced pointer.
      return *(static_cast<void **>(ptr));
    }

  public:
    // Post: empty()
    simple_segregated_storage()
    :first(0)
    { //! Construct empty storage area.
      //! \post empty()
    }

    static void * segregate(void * block,
        size_type nsz, size_type npartition_sz,
        void * end = 0);

    // Same preconditions as 'segregate'
    // Post: !empty()
    void add_block(void * const block,
        const size_type nsz, const size_type npartition_sz)
    { //! Add block
      //! Segregate this block and merge its free list into the
      //!  free list referred to by "first".
      //! \pre Same as segregate.
      //!  \post !empty()
      BOOST_POOL_VALIDATE_INTERNALS
      first = segregate(block, nsz, npartition_sz, first);
      BOOST_POOL_VALIDATE_INTERNALS
    }

    // Same preconditions as 'segregate'
    // Post: !empty()
    void add_ordered_block(void * const block,
        const size_type nsz, const size_type npartition_sz)
    { //! add block (ordered into list)
      //! This (slower) version of add_block segregates the
      //!  block and merges its free list into our free list
      //!  in the proper order.
       BOOST_POOL_VALIDATE_INTERNALS
      // Find where "block" would go in the free list
      void * const loc = find_prev(block);

      // Place either at beginning or in middle/end
      if (loc == 0)
        add_block(block, nsz, npartition_sz);
      else
        nextof(loc) = segregate(block, nsz, npartition_sz, nextof(loc));
      BOOST_POOL_VALIDATE_INTERNALS
    }

    // default destructor.

    bool empty() const
    { //! \returns true only if simple_segregated_storage is empty.
      return (first == 0);
    }

    void * malloc BOOST_PREVENT_MACRO_SUBSTITUTION()
    { //! Create a chunk.
      //!  \pre !empty()
      //! Increment the "first" pointer to point to the next chunk.
       BOOST_POOL_VALIDATE_INTERNALS
      void * const ret = first;

      // Increment the "first" pointer to point to the next chunk.
      first = nextof(first);
      BOOST_POOL_VALIDATE_INTERNALS
      return ret;
    }

    void free BOOST_PREVENT_MACRO_SUBSTITUTION(void * const chunk)
    { //! Free a chunk.
      //! \pre chunk was previously returned from a malloc() referring to the same free list.
      //! \post !empty()
       BOOST_POOL_VALIDATE_INTERNALS
      nextof(chunk) = first;
      first = chunk;
      BOOST_POOL_VALIDATE_INTERNALS
    }

    void ordered_free(void * const chunk)
    { //! This (slower) implementation of 'free' places the memory
      //!  back in the list in its proper order.
      //! \pre chunk was previously returned from a malloc() referring to the same free list
      //! \post !empty().

      // Find where "chunk" goes in the free list
       BOOST_POOL_VALIDATE_INTERNALS
      void * const loc = find_prev(chunk);

      // Place either at beginning or in middle/end.
      if (loc == 0)
        (free)(chunk);
      else
      {
        nextof(chunk) = nextof(loc);
        nextof(loc) = chunk;
      }
      BOOST_POOL_VALIDATE_INTERNALS
    }

   void * malloc_n(size_type n, size_type partition_size);
    
    //! \pre chunks was previously allocated from *this with the same
    //!   values for n and partition_size.
    //! \post !empty()
    //! \note If you're allocating/deallocating n a lot, you should
    //!  be using an ordered pool.
    void free_n(void * const chunks, const size_type n,
        const size_type partition_size)
    { 
       BOOST_POOL_VALIDATE_INTERNALS
      if(n != 0)
        add_block(chunks, n * partition_size, partition_size);
       BOOST_POOL_VALIDATE_INTERNALS
    }

    // pre: chunks was previously allocated from *this with the same
    //   values for n and partition_size.
    // post: !empty()
    void ordered_free_n(void * const chunks, const size_type n,
        const size_type partition_size)
    { //! Free n chunks from order list.
      //! \pre chunks was previously allocated from *this with the same
      //!   values for n and partition_size.

      //! \pre n should not be zero (n == 0 has no effect).
       BOOST_POOL_VALIDATE_INTERNALS
      if(n != 0)
        add_ordered_block(chunks, n * partition_size, partition_size);
       BOOST_POOL_VALIDATE_INTERNALS
    }
#ifdef BOOST_POOL_VALIDATE
    void validate()
    {
       int index = 0;
       void* old = 0;
       void* ptr = first;
       while(ptr)
       {
          void* pt = nextof(ptr); // trigger possible segfault *before* we update variables
          ++index;
          old = ptr;
          ptr = nextof(ptr);
       }
    }
#endif
};

//! Traverses the free list referred to by "first",
//!  and returns the iterator previous to where
//!  "ptr" would go if it was in the free list.
//!  Returns 0 if "ptr" would go at the beginning
//!  of the free list (i.e., before "first").

//! \note Note that this function finds the location previous to where ptr would go
//! if it was in the free list.
//! It does not find the entry in the free list before ptr
//! (unless ptr is already in the free list).
//! Specifically, find_prev(0) will return 0,
//! not the last entry in the free list.
//! \returns location previous to where ptr would go if it was in the free list.
template <typename SizeType>
void * simple_segregated_storage<SizeType>::find_prev(void * const ptr)
{ 
  // Handle border case.
  if (first == 0 || std::greater<void *>()(first, ptr))
    return 0;

  void * iter = first;
  while (true)
  {
    // if we're about to hit the end, or if we've found where "ptr" goes.
    if (nextof(iter) == 0 || std::greater<void *>()(nextof(iter), ptr))
      return iter;

    iter = nextof(iter);
  }
}

//! Segregate block into chunks.
//! \pre npartition_sz >= sizeof(void *)
//! \pre    npartition_sz = sizeof(void *) * i, for some integer i
//! \pre    nsz >= npartition_sz
//! \pre Block is properly aligned for an array of object of
//!        size npartition_sz and array of void *.
//! The requirements above guarantee that any pointer to a chunk
//! (which is a pointer to an element in an array of npartition_sz)
//! may be cast to void **.
template <typename SizeType>
void * simple_segregated_storage<SizeType>::segregate(
    void * const block,
    const size_type sz,
    const size_type partition_sz,
    void * const end)
{
  // Get pointer to last valid chunk, preventing overflow on size calculations
  //  The division followed by the multiplication just makes sure that
  //    old == block + partition_sz * i, for some integer i, even if the
  //    block size (sz) is not a multiple of the partition size.
  char * old = static_cast<char *>(block)
      + ((sz - partition_sz) / partition_sz) * partition_sz;

  // Set it to point to the end
  nextof(old) = end;

  // Handle border case where sz == partition_sz (i.e., we're handling an array
  //  of 1 element)
  if (old == block)
    return block;

  // Iterate backwards, building a singly-linked list of pointers
  for (char * iter = old - partition_sz; iter != block;
      old = iter, iter -= partition_sz)
    nextof(iter) = old;

  // Point the first pointer, too
  nextof(block) = old;

  return block;
}

//! \pre (n > 0), (start != 0), (nextof(start) != 0)
//! \post (start != 0)
//! The function attempts to find n contiguous chunks
//!  of size partition_size in the free list, starting at start.
//! If it succeds, it returns the last chunk in that contiguous
//!  sequence, so that the sequence is known by [start, {retval}]
//! If it fails, it does do either because it's at the end of the
//!  free list or hits a non-contiguous chunk.  In either case,
//!  it will return 0, and set start to the last considered
//!  chunk.  You are at the end of the free list if
//!  nextof(start) == 0.  Otherwise, start points to the last
//!  chunk in the contiguous sequence, and nextof(start) points
//!  to the first chunk in the next contiguous sequence (assuming
//!  an ordered free list).
template <typename SizeType>
void * simple_segregated_storage<SizeType>::try_malloc_n(
    void * & start, size_type n, const size_type partition_size)
{
  void * iter = nextof(start);
  while (--n != 0)
  {
    void * next = nextof(iter);
    if (next != static_cast<char *>(iter) + partition_size)
    {
      // next == 0 (end-of-list) or non-contiguous chunk found
      start = iter;
      return 0;
    }
    iter = next;
  }
  return iter;
}

//! Attempts to find a contiguous sequence of n partition_sz-sized chunks. If found, removes them 
//! all from the free list and returns a pointer to the first. If not found, returns 0. It is strongly 
//! recommended (but not required) that the free list be ordered, as this algorithm will fail to find 
//! a contiguous sequence unless it is contiguous in the free list as well. Order-preserving. 
//! O(N) with respect to the size of the free list.
template <typename SizeType>
void * simple_segregated_storage<SizeType>::malloc_n(const size_type n,
    const size_type partition_size)
{
   BOOST_POOL_VALIDATE_INTERNALS
  if(n == 0)
    return 0;
  void * start = &first;
  void * iter;
  do
  {
    if (nextof(start) == 0)
      return 0;
    iter = try_malloc_n(start, n, partition_size);
  } while (iter == 0);
  void * const ret = nextof(start);
  nextof(start) = nextof(iter);
  BOOST_POOL_VALIDATE_INTERNALS
  return ret;
}

} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
