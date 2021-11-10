// Copyright (C) 2000, 2001 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_SINGLETON_POOL_HPP
#define BOOST_SINGLETON_POOL_HPP

/*!
  \file
  \brief The <tt>singleton_pool</tt> class allows other pool interfaces
  for types of the same size to share the same underlying pool.

  \details Header singleton_pool.hpp provides a template class <tt>singleton_pool</tt>,
  which provides access to a pool as a singleton object.
  
*/

#include <boost/pool/poolfwd.hpp>

// boost::pool
#include <boost/pool/pool.hpp>
// boost::details::pool::guard
#include <boost/pool/detail/guard.hpp>

#include <boost/type_traits/aligned_storage.hpp>

namespace boost {

 /*! 
 The singleton_pool class allows other pool interfaces
 for types of the same size to share the same pool.  Template
 parameters are as follows:

 <b>Tag</b> User-specified type to uniquely identify this pool: allows different unbounded sets of singleton pools to exist.

 <b>RequestedSize</b> The size of each chunk returned by member function <tt>malloc()</tt>.

 <B>UserAllocator</b> User allocator, default = default_user_allocator_new_delete.

 <b>Mutex</B> This class is the type of mutex to use to protect simultaneous access to the underlying Pool. 
 Can be any Boost.Thread Mutex type or <tt>boost::details::pool::null_mutex</tt>.
 It is exposed so that users may declare some singleton pools normally (i.e., with synchronization), but 
 some singleton pools without synchronization (by specifying <tt>boost::details::pool::null_mutex</tt>) for efficiency reasons.
 The member typedef <tt>mutex</tt> exposes the value of this template parameter.  The default for this
 parameter is boost::details::pool::default_mutex which is a synonym for either <tt>boost::details::pool::null_mutex</tt>
 (when threading support is turned off in the compiler (so BOOST_HAS_THREADS is not set), or threading support
 has ben explicitly disabled with BOOST_DISABLE_THREADS (Boost-wide disabling of threads) or BOOST_POOL_NO_MT (this library only))
 or for <tt>boost::mutex</tt> (when threading support is enabled in the compiler).

 <B>NextSize</b> The value of this parameter is passed to the underlying Pool when it is created and
 specifies the number of chunks to allocate in the first allocation request (defaults to 32).
 The member typedef <tt>static const value next_size</tt> exposes the value of this template parameter.

 <b>MaxSize</B>The value of this parameter is passed to the underlying Pool when it is created and
 specifies the maximum number of chunks to allocate in any single allocation request (defaults to 0).

  <b>Notes:</b>

  The underlying pool <i>p</i> referenced by the static functions
  in singleton_pool is actually declared in a way that is:

  1 Thread-safe if there is only one thread running before main() begins and after main() ends
  -- all of the static functions of singleton_pool synchronize their access to p.

  2 Guaranteed to be constructed before it is used --
  thus, the simple static object in the synopsis above would actually be an incorrect implementation.
  The actual implementation to guarantee this is considerably more complicated.

  3 Note too that a different underlying pool p exists
  for each different set of template parameters,
  including implementation-specific ones.

  4 The underlying pool is constructed "as if" by:

  pool<UserAllocator> p(RequestedSize, NextSize, MaxSize);

  \attention
  The underlying pool constructed by the singleton 
  <b>is never freed</b>.  This means that memory allocated
  by a singleton_pool can be still used after main() has
  completed, but may mean that some memory checking programs
  will complain about leaks from singleton_pool.
 
  */

 template <typename Tag,
    unsigned RequestedSize,
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
class singleton_pool
{
  public:
    typedef Tag tag; /*!< The Tag template parameter uniquely
                     identifies this pool and allows
      different unbounded sets of singleton pools to exist.
      For example, the pool allocators use two tag classes to ensure that the
      two different allocator types never share the same underlying singleton pool.
      Tag is never actually used by singleton_pool.
    */
    typedef Mutex mutex; //!< The type of mutex used to synchonise access to this pool (default <tt>details::pool::default_mutex</tt>).
    typedef UserAllocator user_allocator; //!< The user-allocator used by this pool, default = <tt>default_user_allocator_new_delete</tt>.
    typedef typename pool<UserAllocator>::size_type size_type; //!< size_type of user allocator.
    typedef typename pool<UserAllocator>::difference_type difference_type; //!< difference_type of user allocator.

    BOOST_STATIC_CONSTANT(unsigned, requested_size = RequestedSize); //!< The size of each chunk allocated by this pool.
    BOOST_STATIC_CONSTANT(unsigned, next_size = NextSize); //!< The number of chunks to allocate on the first allocation.

private:
    singleton_pool();

#ifndef BOOST_DOXYGEN
    struct pool_type: public Mutex, public pool<UserAllocator>
    {
      pool_type() : pool<UserAllocator>(RequestedSize, NextSize, MaxSize) {}
    }; //  struct pool_type: Mutex

#else
    //
    // This is invoked when we build with Doxygen only:
    //
public:
    static pool<UserAllocator> p; //!< For exposition only!
#endif


  public:
    static void * malloc BOOST_PREVENT_MACRO_SUBSTITUTION()
    { //! Equivalent to SingletonPool::p.malloc(); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return (p.malloc)();
    }
    static void * ordered_malloc()
    {  //! Equivalent to SingletonPool::p.ordered_malloc(); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return p.ordered_malloc();
    }
    static void * ordered_malloc(const size_type n)
    { //! Equivalent to SingletonPool::p.ordered_malloc(n); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return p.ordered_malloc(n);
    }
    static bool is_from(void * const ptr)
    { //! Equivalent to SingletonPool::p.is_from(chunk); synchronized.
      //! \returns true if chunk is from SingletonPool::is_from(chunk)
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return p.is_from(ptr);
    }
    static void free BOOST_PREVENT_MACRO_SUBSTITUTION(void * const ptr)
    { //! Equivalent to SingletonPool::p.free(chunk); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      (p.free)(ptr);
    }
    static void ordered_free(void * const ptr)
    { //! Equivalent to SingletonPool::p.ordered_free(chunk); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      p.ordered_free(ptr);
    }
    static void free BOOST_PREVENT_MACRO_SUBSTITUTION(void * const ptr, const size_type n)
    { //! Equivalent to SingletonPool::p.free(chunk, n); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      (p.free)(ptr, n);
    }
    static void ordered_free(void * const ptr, const size_type n)
    { //! Equivalent to SingletonPool::p.ordered_free(chunk, n); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      p.ordered_free(ptr, n);
    }
    static bool release_memory()
    { //! Equivalent to SingletonPool::p.release_memory(); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return p.release_memory();
    }
    static bool purge_memory()
    { //! Equivalent to SingletonPool::p.purge_memory(); synchronized.
      pool_type & p = get_pool();
      details::pool::guard<Mutex> g(p);
      return p.purge_memory();
    }

private:
   typedef boost::aligned_storage<sizeof(pool_type), boost::alignment_of<pool_type>::value> storage_type;
   static storage_type storage;

   static pool_type& get_pool()
   {
      static bool f = false;
      if(!f)
      {
         // This code *must* be called before main() starts, 
         // and when only one thread is executing.
         f = true;
         new (&storage) pool_type;
      }

      // The following line does nothing else than force the instantiation
      //  of singleton<T>::create_object, whose constructor is
      //  called before main() begins.
      create_object.do_nothing();

      return *static_cast<pool_type*>(static_cast<void*>(&storage));
   }

   struct object_creator
   {
      object_creator()
      {  // This constructor does nothing more than ensure that instance()
         //  is called before main() begins, thus creating the static
         //  T object before multithreading race issues can come up.
         singleton_pool<Tag, RequestedSize, UserAllocator, Mutex, NextSize, MaxSize>::get_pool();
      }
      inline void do_nothing() const
      {
      }
   };
   static object_creator create_object;
}; // struct singleton_pool

template <typename Tag,
    unsigned RequestedSize,
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
typename singleton_pool<Tag, RequestedSize, UserAllocator, Mutex, NextSize, MaxSize>::storage_type singleton_pool<Tag, RequestedSize, UserAllocator, Mutex, NextSize, MaxSize>::storage;

template <typename Tag,
    unsigned RequestedSize,
    typename UserAllocator,
    typename Mutex,
    unsigned NextSize,
    unsigned MaxSize >
typename singleton_pool<Tag, RequestedSize, UserAllocator, Mutex, NextSize, MaxSize>::object_creator singleton_pool<Tag, RequestedSize, UserAllocator, Mutex, NextSize, MaxSize>::create_object;

} // namespace boost

#endif
