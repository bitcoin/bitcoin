// Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file allocator.hpp
 *
 *  This header provides an STL-compliant allocator that uses the
 *  MPI-2 memory allocation facilities.
 */
#ifndef BOOST_MPI_ALLOCATOR_HPP
#define BOOST_MPI_ALLOCATOR_HPP

#include <boost/mpi/config.hpp>
#include <boost/mpi/exception.hpp>
#include <cstddef>
#include <memory>
#include <boost/limits.hpp>

namespace boost { namespace mpi {

#if defined(BOOST_MPI_HAS_MEMORY_ALLOCATION)
template<typename T> class allocator;

/** @brief Allocator specialization for @c void value types.
 *
 *  The @c void specialization of @c allocator is useful only for
 *  rebinding to another, different value type.
 */
template<> 
class BOOST_MPI_DECL allocator<void> 
{ 
public: 
  typedef void* pointer; 
  typedef const void* const_pointer; 
  typedef void value_type; 

  template <class U> 
  struct rebind 
  { 
    typedef allocator<U> other; 
  }; 
};

/** @brief Standard Library-compliant allocator for the MPI-2 memory
 *  allocation routines.
 *
 *  This allocator provides a standard C++ interface to the @c
 *  MPI_Alloc_mem and @c MPI_Free_mem routines of MPI-2. It is
 *  intended to be used with the containers in the Standard Library
 *  (@c vector, in particular) in cases where the contents of the
 *  container will be directly transmitted via MPI. This allocator is
 *  also used internally by the library for character buffers that
 *  will be used in the transmission of data.
 *
 *  The @c allocator class template only provides MPI memory
 *  allocation when the underlying MPI implementation is either MPI-2
 *  compliant or is known to provide @c MPI_Alloc_mem and @c
 *  MPI_Free_mem as extensions. When the MPI memory allocation
 *  routines are not available, @c allocator is brought in directly
 *  from namespace @c std, so that standard allocators are used
 *  throughout. The macro @c BOOST_MPI_HAS_MEMORY_ALLOCATION will be
 *  defined when the MPI-2 memory allocation facilities are available.
 */
template<typename T> 
class BOOST_MPI_DECL allocator 
{
public:
  /// Holds the size of objects
  typedef std::size_t     size_type;

  /// Holds the number of elements between two pointers
  typedef std::ptrdiff_t  difference_type;

  /// A pointer to an object of type @c T
  typedef T*              pointer;

  /// A pointer to a constant object of type @c T
  typedef const T*        const_pointer;

  /// A reference to an object of type @c T
  typedef T&              reference;

  /// A reference to a constant object of type @c T
  typedef const T&        const_reference;

  /// The type of memory allocated by this allocator
  typedef T               value_type;

  /** @brief Retrieve the type of an allocator similar to this
   * allocator but for a different value type.
   */
  template <typename U> 
  struct rebind 
  { 
    typedef allocator<U> other; 
  };

  /** Default-construct an allocator. */
  allocator() throw() { }

  /** Copy-construct an allocator. */
  allocator(const allocator&) throw() { }

  /** 
   * Copy-construct an allocator from another allocator for a
   * different value type.
   */
  template <typename U> 
  allocator(const allocator<U>&) throw() { }

  /** Destroy an allocator. */
  ~allocator() throw() { }

  /** Returns the address of object @p x. */
  pointer address(reference x) const
  {
    return &x;
  }

  /** Returns the address of object @p x. */
  const_pointer address(const_reference x) const
  {
    return &x;
  }

  /** 
   *  Allocate enough memory for @p n elements of type @c T.
   *
   *  @param n The number of elements for which memory should be
   *  allocated.
   *
   *  @return a pointer to the newly-allocated memory
   */
  pointer allocate(size_type n, allocator<void>::const_pointer /*hint*/ = 0)
  {
    pointer result;
    BOOST_MPI_CHECK_RESULT(MPI_Alloc_mem,
                           (static_cast<MPI_Aint>(n * sizeof(T)), 
                            MPI_INFO_NULL, 
                            &result));
    return result;
  }

  /**
   *  Deallocate memory referred to by the pointer @c p.
   *
   *  @param p The pointer whose memory should be deallocated. This
   *  pointer shall have been returned from the @c allocate() function
   *  and not have already been freed.
   */
  void deallocate(pointer p, size_type /*n*/)
  {
    BOOST_MPI_CHECK_RESULT(MPI_Free_mem, (p));
  }

  /** 
   * Returns the maximum number of elements that can be allocated
   * with @c allocate().
   */
  size_type max_size() const throw()
  {
    return (std::numeric_limits<std::size_t>::max)() / sizeof(T);
  }

  /** Construct a copy of @p val at the location referenced by @c p. */
  void construct(pointer p, const T& val)
  {
    new ((void *)p) T(val);
  }

  /** Destroy the object referenced by @c p. */
  void destroy(pointer p)
  {
    ((T*)p)->~T();
  }
};

/** @brief Compare two allocators for equality.
 *
 *  Since MPI allocators have no state, all MPI allocators are equal.
 *
 *  @returns @c true
 */
template<typename T1, typename T2>
inline bool operator==(const allocator<T1>&, const allocator<T2>&) throw()
{
  return true;
}

/** @brief Compare two allocators for inequality.
 *
 *  Since MPI allocators have no state, all MPI allocators are equal.
 *
 *  @returns @c false
 */
template<typename T1, typename T2>
inline bool operator!=(const allocator<T1>&, const allocator<T2>&) throw()
{
  return false;
}
#else
// Bring in the default allocator from namespace std.
using std::allocator;
#endif

} } /// end namespace boost::mpi

#endif // BOOST_MPI_ALLOCATOR_HPP
