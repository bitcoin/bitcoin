// Copyright (C) 2000, 2001 Stephen Cleary
// Copyright (C) 2010 Paul A. Bristow added Doxygen comments.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POOL_ALLOC_HPP
#define BOOST_POOL_ALLOC_HPP

/*!
  \file
  \brief C++ Standard Library compatible pool-based allocators.
  \details  This header provides two template types - 
  \ref pool_allocator and \ref fast_pool_allocator -
  that can be used for fast and efficient memory allocation
  in conjunction with the C++ Standard Library containers.

  These types both satisfy the Standard Allocator requirements [20.1.5]
  and the additional requirements in [20.1.5/4],
  so they can be used with either Standard or user-supplied containers.

  In addition, the fast_pool_allocator also provides an additional allocation
  and an additional deallocation function:

<table>
<tr><th>Expression</th><th>Return Type</th><th>Semantic Equivalence<th></tr>
<tr><td><tt>PoolAlloc::allocate()</tt></td><td><tt>T *</tt></td><td><tt>PoolAlloc::allocate(1)</tt></tr>
<tr><td><tt>PoolAlloc::deallocate(p)</tt></td><td>void</tt></td><td><tt>PoolAlloc::deallocate(p, 1)</tt></tr>
</table>

The typedef user_allocator publishes the value of the UserAllocator template parameter.

<b>Notes</b>

If the allocation functions run out of memory, they will throw <tt>std::bad_alloc</tt>.

The underlying Pool type used by the allocators is accessible through the Singleton Pool Interface.
The identifying tag used for pool_allocator is pool_allocator_tag,
and the tag used for fast_pool_allocator is fast_pool_allocator_tag.
All template parameters of the allocators (including implementation-specific ones)
determine the type of the underlying Pool,
with the exception of the first parameter T, whose size is used instead.

Since the size of T is used to determine the type of the underlying Pool,
each allocator for different types of the same size will share the same underlying pool.
The tag class prevents pools from being shared between pool_allocator and fast_pool_allocator.
For example, on a system where
<tt>sizeof(int) == sizeof(void *)</tt>, <tt>pool_allocator<int></tt> and <tt>pool_allocator<void *></tt>
will both allocate/deallocate from/to the same pool.

If there is only one thread running before main() starts and after main() ends,
then both allocators are completely thread-safe.

<b>Compiler and STL Notes</b>

A number of common STL libraries contain bugs in their using of allocators.
Specifically, they pass null pointers to the deallocate function,
which is explicitly forbidden by the Standard [20.1.5 Table 32].
PoolAlloc will work around these libraries if it detects them;
currently, workarounds are in place for:
Borland C++ (Builder and command-line compiler)
with default (RogueWave) library, ver. 5 and earlier,
STLport (with any compiler), ver. 4.0 and earlier.
*/

// std::numeric_limits
#include <boost/limits.hpp>
// new, std::bad_alloc
#include <new>

#include <boost/throw_exception.hpp>
#include <boost/pool/poolfwd.hpp>

// boost::singleton_pool
#include <boost/pool/singleton_pool.hpp>

#include <boost/detail/workaround.hpp>

// C++11 features detection
#include <boost/config.hpp>

// std::forward
#ifdef BOOST_HAS_VARIADIC_TMPL
#include <utility>
#endif

#ifdef BOOST_POOL_INSTRUMENT
#include <iostream>
#include <iomanip>
#endif

// The following code will be put into Boost.Config in a later revision
#if defined(_RWSTD_VER) || defined(__SGI_STL_PORT) || \
    BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x582))
 #define BOOST_NO_PROPER_STL_DEALLOCATE
#endif

namespace boost {

#ifdef BOOST_POOL_INSTRUMENT

template <bool b>
struct debug_info
{
   static unsigned allocated;
};

template <bool b>
unsigned debug_info<b>::allocated = 0;

#endif

 //! Simple tag type used by pool_allocator as an argument to the
 //! underlying singleton_pool.
 struct pool_allocator_tag
{
};

/*!  \brief A C++ Standard Library conforming allocator, based on an underlying pool.

  Template parameters for pool_allocator are defined as follows:

  <b>T</b> Type of object to allocate/deallocate.

  <b>UserAllocator</B>. Defines the method that the underlying Pool will use to allocate memory from the system. See 
  <a href="boost_pool/pool/pooling.html#boost_pool.pool.pooling.user_allocator">User Allocators</a> for details.

  <b>Mutex</b> Allows the user to determine the type of synchronization to be used on the underlying singleton_pool. 

  <b>NextSize</b> The value of this parameter is passed to the underlying singleton_pool when it is created.

  <b>MaxSize</b> Limit on the maximum size used.

  \attention
  The underlying singleton_pool used by the this allocator
  constructs a pool instance that
  <b>is never freed</b>.  This means that memory allocated
  by the allocator can be still used after main() has
  completed, but may mean that some memory checking programs
  will complain about leaks.
 
  
  */
template <typename T,
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
class pool_allocator
{
  public:
    typedef T value_type;  //!< value_type of template parameter T.
    typedef UserAllocator user_allocator;  //!< allocator that defines the method that the underlying Pool will use to allocate memory from the system.
    typedef Mutex mutex; //!< typedef mutex publishes the value of the template parameter Mutex.
    BOOST_STATIC_CONSTANT(unsigned, next_size = NextSize); //!< next_size publishes the values of the template parameter NextSize.

    typedef value_type * pointer; //!<
    typedef const value_type * const_pointer;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef typename pool<UserAllocator>::size_type size_type;
    typedef typename pool<UserAllocator>::difference_type difference_type;

    //! \brief Nested class rebind allows for transformation from
    //! pool_allocator<T> to pool_allocator<U>.
    //!
    //! Nested class rebind allows for transformation from
    //! pool_allocator<T> to pool_allocator<U> via the member
    //! typedef other.
    template <typename U>
    struct rebind
    { //
      typedef pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> other;
    };

  public:
    pool_allocator()
    { /*! Results in default construction of the underlying singleton_pool IFF an
       instance of this allocator is constructed during global initialization (
         required to ensure construction of singleton_pool IFF an
         instance of this allocator is constructed during global
         initialization. See ticket #2359 for a complete explanation at
         http://svn.boost.org/trac/boost/ticket/2359) .
       */
      singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
                     NextSize, MaxSize>::is_from(0);
    }

    // default copy constructor.

    // default assignment operator.

    // not explicit, mimicking std::allocator [20.4.1]
    template <typename U>
    pool_allocator(const pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> &)
    { /*! Results in the default construction of the underlying singleton_pool, this
         is required to ensure construction of singleton_pool IFF an
         instance of this allocator is constructed during global
         initialization. See ticket #2359 for a complete explanation
         at http://svn.boost.org/trac/boost/ticket/2359 .
       */
      singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
                     NextSize, MaxSize>::is_from(0);
    }

    // default destructor

    static pointer address(reference r)
    { return &r; }
    static const_pointer address(const_reference s)
    { return &s; }
    static size_type max_size()
    { return (std::numeric_limits<size_type>::max)(); }

#if defined(BOOST_HAS_VARIADIC_TMPL) && defined(BOOST_HAS_RVALUE_REFS)
    template <typename U, typename... Args>
    static void construct(U* ptr, Args&&... args)
    { new (ptr) U(std::forward<Args>(args)...); }
#else
    static void construct(const pointer ptr, const value_type & t)
    { new (ptr) T(t); }
#endif

    static void destroy(const pointer ptr)
    {
      ptr->~T();
      (void) ptr; // avoid unused variable warning.
    }

    bool operator==(const pool_allocator &) const
    { return true; }
    bool operator!=(const pool_allocator &) const
    { return false; }

    static pointer allocate(const size_type n)
    {
#ifdef BOOST_POOL_INSTRUMENT
       debug_info<true>::allocated += n * sizeof(T);
       std::cout << "Allocating " << n << " * " << sizeof(T) << " bytes...\n"
          "Total allocated is now " << debug_info<true>::allocated << std::endl;
#endif
      const pointer ret = static_cast<pointer>(
          singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
              NextSize, MaxSize>::ordered_malloc(n) );
      if ((ret == 0) && n)
        boost::throw_exception(std::bad_alloc());
      return ret;
    }
    static pointer allocate(const size_type n, const void * const)
    { //! allocate n bytes
    //! \param n bytes to allocate.
    //! \param unused.
      return allocate(n);
    }
    static void deallocate(const pointer ptr, const size_type n)
    {  //! Deallocate n bytes from ptr
       //! \param ptr location to deallocate from.
       //! \param n number of bytes to deallocate.
#ifdef BOOST_POOL_INSTRUMENT
       debug_info<true>::allocated -= n * sizeof(T);
       std::cout << "Deallocating " << n << " * " << sizeof(T) << " bytes...\n"
          "Total allocated is now " << debug_info<true>::allocated << std::endl;
#endif
#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
      if (ptr == 0 || n == 0)
        return;
#endif
      singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
          NextSize, MaxSize>::ordered_free(ptr, n);
    }
};

/*! \brief Specialization of pool_allocator<void>.

Specialization of pool_allocator for type void: required by the standard to make this a conforming allocator type.
*/
template<
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize>
class pool_allocator<void, UserAllocator, Mutex, NextSize, MaxSize>
{
public:
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;
    //! \brief Nested class rebind allows for transformation from
    //! pool_allocator<T> to pool_allocator<U>.
    //!
    //! Nested class rebind allows for transformation from
    //! pool_allocator<T> to pool_allocator<U> via the member
    //! typedef other.
    template <class U> 
    struct rebind
    {
       typedef pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> other;
    };
};

//! Simple tag type used by fast_pool_allocator as a template parameter to the underlying singleton_pool.
struct fast_pool_allocator_tag
{
};

 /*! \brief A C++ Standard Library conforming allocator geared towards allocating single chunks.

  While class template <tt>pool_allocator</tt> is a more general-purpose solution geared towards
  efficiently servicing requests for any number of contiguous chunks,
  <tt>fast_pool_allocator</tt> is also a general-purpose solution,
  but is geared towards efficiently servicing requests for one chunk at a time;
  it will work for contiguous chunks, but not as well as <tt>pool_allocator</tt>.

  If you are seriously concerned about performance,
  use <tt>fast_pool_allocator</tt> when dealing with containers such as <tt>std::list</tt>,
  and use <tt>pool_allocator</tt> when dealing with containers such as <tt>std::vector</tt>.

  The template parameters are defined as follows:

  <b>T</b> Type of object to allocate/deallocate.

  <b>UserAllocator</b>. Defines the method that the underlying Pool will use to allocate memory from the system. 
  See <a href="boost_pool/pool/pooling.html#boost_pool.pool.pooling.user_allocator">User Allocators</a> for details.

  <b>Mutex</b> Allows the user to determine the type of synchronization to be used on the underlying <tt>singleton_pool</tt>.

  <b>NextSize</b> The value of this parameter is passed to the underlying Pool when it is created.

  <b>MaxSize</b> Limit on the maximum size used.

   \attention
  The underlying singleton_pool used by the this allocator
  constructs a pool instance that
  <b>is never freed</b>.  This means that memory allocated
  by the allocator can be still used after main() has
  completed, but may mean that some memory checking programs
  will complain about leaks.
 
 */

template <typename T,
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
class fast_pool_allocator
{
  public:
    typedef T value_type;
    typedef UserAllocator user_allocator;
    typedef Mutex mutex;
    BOOST_STATIC_CONSTANT(unsigned, next_size = NextSize);

    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef typename pool<UserAllocator>::size_type size_type;
    typedef typename pool<UserAllocator>::difference_type difference_type;

    //! \brief Nested class rebind allows for transformation from
    //! fast_pool_allocator<T> to fast_pool_allocator<U>.
    //!
    //! Nested class rebind allows for transformation from
    //! fast_pool_allocator<T> to fast_pool_allocator<U> via the member
    //! typedef other.
    template <typename U>
    struct rebind
    {
      typedef fast_pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> other;
    };

  public:
    fast_pool_allocator()
    {
      //! Ensures construction of the underlying singleton_pool IFF an
      //! instance of this allocator is constructed during global
      //! initialization. See ticket #2359 for a complete explanation
      //! at http://svn.boost.org/trac/boost/ticket/2359 .
      singleton_pool<fast_pool_allocator_tag, sizeof(T),
                     UserAllocator, Mutex, NextSize, MaxSize>::is_from(0);
    }

    // Default copy constructor used.

    // Default assignment operator used.

    // Not explicit, mimicking std::allocator [20.4.1]
    template <typename U>
    fast_pool_allocator(
        const fast_pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> &)
    {
      //! Ensures construction of the underlying singleton_pool IFF an
      //! instance of this allocator is constructed during global
      //! initialization. See ticket #2359 for a complete explanation
      //! at http://svn.boost.org/trac/boost/ticket/2359 .
      singleton_pool<fast_pool_allocator_tag, sizeof(T),
                     UserAllocator, Mutex, NextSize, MaxSize>::is_from(0);
    }

    // Default destructor used.

    static pointer address(reference r)
    {
      return &r;
    }
    static const_pointer address(const_reference s)
    { return &s; }
    static size_type max_size()
    { return (std::numeric_limits<size_type>::max)(); }

#if defined(BOOST_HAS_VARIADIC_TMPL) && defined(BOOST_HAS_RVALUE_REFS)
    template <typename U, typename... Args>
    void construct(U* ptr, Args&&... args)
    { new (ptr) U(std::forward<Args>(args)...); }
#else
    void construct(const pointer ptr, const value_type & t)
    { new (ptr) T(t); }
#endif

    void destroy(const pointer ptr)
    { //! Destroy ptr using destructor.
      ptr->~T();
      (void) ptr; // Avoid unused variable warning.
    }

    bool operator==(const fast_pool_allocator &) const
    { return true; }
    bool operator!=(const fast_pool_allocator &) const
    { return false; }

    static pointer allocate(const size_type n)
    {
      const pointer ret = (n == 1) ?
          static_cast<pointer>(
              (singleton_pool<fast_pool_allocator_tag, sizeof(T),
                  UserAllocator, Mutex, NextSize, MaxSize>::malloc)() ) :
          static_cast<pointer>(
              singleton_pool<fast_pool_allocator_tag, sizeof(T),
                  UserAllocator, Mutex, NextSize, MaxSize>::ordered_malloc(n) );
      if (ret == 0)
        boost::throw_exception(std::bad_alloc());
      return ret;
    }
    static pointer allocate(const size_type n, const void * const)
    { //! Allocate memory .
      return allocate(n);
    }
    static pointer allocate()
    { //! Allocate memory.
      const pointer ret = static_cast<pointer>(
          (singleton_pool<fast_pool_allocator_tag, sizeof(T),
              UserAllocator, Mutex, NextSize, MaxSize>::malloc)() );
      if (ret == 0)
        boost::throw_exception(std::bad_alloc());
      return ret;
    }
    static void deallocate(const pointer ptr, const size_type n)
    { //! Deallocate memory.

#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
      if (ptr == 0 || n == 0)
        return;
#endif
      if (n == 1)
        (singleton_pool<fast_pool_allocator_tag, sizeof(T),
            UserAllocator, Mutex, NextSize, MaxSize>::free)(ptr);
      else
        (singleton_pool<fast_pool_allocator_tag, sizeof(T),
            UserAllocator, Mutex, NextSize, MaxSize>::free)(ptr, n);
    }
    static void deallocate(const pointer ptr)
    { //! deallocate/free
      (singleton_pool<fast_pool_allocator_tag, sizeof(T),
          UserAllocator, Mutex, NextSize, MaxSize>::free)(ptr);
    }
};

/*!  \brief Specialization of fast_pool_allocator<void>.

Specialization of fast_pool_allocator<void> required to make the allocator standard-conforming.
*/
template<
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
class fast_pool_allocator<void, UserAllocator, Mutex, NextSize, MaxSize>
{
public:
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    //! \brief Nested class rebind allows for transformation from
    //! fast_pool_allocator<T> to fast_pool_allocator<U>.
    //!
    //! Nested class rebind allows for transformation from
    //! fast_pool_allocator<T> to fast_pool_allocator<U> via the member
    //! typedef other.
    template <class U> struct rebind
    {
        typedef fast_pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> other;
    };
};

} // namespace boost

#endif
