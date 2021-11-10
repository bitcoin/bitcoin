// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2008-2009,2012 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_STRICT_LOCK_HPP
#define BOOST_THREAD_STRICT_LOCK_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/lockable_wrapper.hpp>
#include <boost/thread/lock_options.hpp>
#include <boost/thread/lock_traits.hpp>
#include <boost/thread/lockable_traits.hpp>
#include <boost/thread/lockable_concepts.hpp>
#include <boost/thread/lock_concepts.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/throw_exception.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{


  //[strict_lock
  template <typename Lockable>
  class strict_lock
  {

    BOOST_CONCEPT_ASSERT(( BasicLockable<Lockable> ));
  public:
    typedef Lockable mutex_type;

    // construct/copy/destroy:

    BOOST_THREAD_NO_COPYABLE( strict_lock)

    /**
     * Constructor from a mutex reference.
     *
     * @param mtx the mutex to lock.
     *
     * __Effects: Stores a reference to the mutex to lock and locks it.
     * __Throws: Any exception BasicMutex::lock() can throw.
     */
    explicit strict_lock(mutex_type& mtx) :
      mtx_(mtx)
    {
      mtx.lock();
    } /*< locks on construction >*/


#if ! defined BOOST_THREAD_NO_CXX11_HDR_INITIALIZER_LIST
    strict_lock(std::initializer_list<thread_detail::lockable_wrapper<Lockable> > l_) :
      mtx_(*(const_cast<thread_detail::lockable_wrapper<Lockable>*>(l_.begin())->m))
    {
      mtx_.lock();
    }
#endif

    /**
     * Destructor
     *
     * __Effects: unlocks the stored mutex.
     *
     * __Throws
     */
    ~strict_lock()
    {
      mtx_.unlock();
    } /*< unlocks on destruction >*/


    // observers

    /**
     * @return the owned mutex.
     */
    mutex_type* mutex() const BOOST_NOEXCEPT
    {
      return &mtx_;
    }

    /**
     * @return whether this lock is locking a mutex.
     */
    bool owns_lock() const BOOST_NOEXCEPT
    {
      return true;
    }

    /**
     * @return whether this lock is locking that mutex.
     */
    bool owns_lock(const mutex_type* l) const BOOST_NOEXCEPT
    {
      return l == mutex();
    } /*< strict locks specific function >*/

    //BOOST_ADRESS_OF_DELETE(strict_lock) /*< disable aliasing >*/
    //BOOST_HEAP_ALLOCATION_DELETE(strict_lock) /*< disable heap allocation >*/

    /*< no possibility to unlock >*/

  private:
    mutex_type& mtx_;
  };
  //]
  template <typename Lockable>
  struct is_strict_lock_sur_parole<strict_lock<Lockable> > : true_type
  {
  };

  /**
   * A nested strict lock is a scoped lock guard ensuring the mutex is locked on its
   * scope, by taking ownership of an nesting lock, locking the mutex on construction if not already locked
   * and restoring the ownership to the nesting lock on destruction.
   */
  //[nested_strict_lock
  template <typename Lock>
  class nested_strict_lock
  {
    BOOST_CONCEPT_ASSERT(( BasicLock<Lock> )); /*< The Lock must be a movable lock >*/
  public:
    typedef typename Lock::mutex_type mutex_type; /*< Name the lockable type locked by Lock >*/

    BOOST_THREAD_NO_COPYABLE( nested_strict_lock)

    /**
     * Constructor from a nesting @c Lock.
     *
     * @param lk the nesting lock
     *
     * __Requires: <c>lk.mutex() != null_ptr</c>
     * __Effects: Stores the reference to the lock parameter and takes ownership on it.
     * If the lock doesn't owns the mutex @c mtx lock it.
     * __Postconditions: @c owns_lock(lk.mutex())
     * __StrongException
     * __Throws:
     *
     * - lock_error when BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is defined and lk.mutex() == null_ptr
     *
     * - Any exception that @c lk.lock() can throw.
     *
     */
    explicit nested_strict_lock(Lock& lk) :
      lk_(lk) /*< Store reference to lk >*/
    {
      /*< Define BOOST_THREAD_DONT_CHECK_PRECONDITIONS if you don't want to check lk ownership >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.mutex() != 0,
          lock_error()
      );
      if (!lk.owns_lock()) lk.lock(); /*< ensures it is locked >*/
      tmp_lk_ = move(lk); /*< Move ownership to temporary lk >*/
    }

#if ! defined BOOST_THREAD_NO_CXX11_HDR_INITIALIZER_LIST
    nested_strict_lock(std::initializer_list<thread_detail::lockable_wrapper<Lock> > l_) :
      lk_(*(const_cast<thread_detail::lockable_wrapper<Lock>*>(l_.begin())->m))
    {
      /*< Define BOOST_THREAD_DONT_CHECK_PRECONDITIONS if you don't want to check lk ownership >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk_.mutex() != 0,
          lock_error()
      );
      if (!lk_.owns_lock()) lk_.lock(); /*< ensures it is locked >*/
      tmp_lk_ = move(lk_); /*< Move ownership to temporary lk >*/
    }
#endif

    /**
     * Destructor
     *
     * __Effects: Restores ownership to the nesting lock.
     */
    ~nested_strict_lock()BOOST_NOEXCEPT
    {
      lk_ = move(tmp_lk_); /*< Move ownership to nesting lock >*/
    }

    // observers
    /**
     * return @c the owned mutex.
     */
    mutex_type* mutex() const BOOST_NOEXCEPT
    {
      return tmp_lk_.mutex();
    }

    /**
     * @return whether this lock is locking a mutex.
     */
    bool owns_lock() const BOOST_NOEXCEPT
    {
      return true;
    }

    /**
     * @return whether if this lock is locking that mutex.
     */
    bool owns_lock(mutex_type const* l) const BOOST_NOEXCEPT
    {
      return l == mutex();
    }

    //BOOST_ADRESS_OF_DELETE(nested_strict_lock)
    //BOOST_HEAP_ALLOCATEION_DELETE(nested_strict_lock)

  private:
    Lock& lk_;
    Lock tmp_lk_;
  };
  //]

  template <typename Lock>
  struct is_strict_lock_sur_parole<nested_strict_lock<Lock> > : true_type
  {
  };

#if ! defined BOOST_THREAD_NO_MAKE_STRICT_LOCK
  template <typename Lockable>
  strict_lock<Lockable> make_strict_lock(Lockable& mtx)
  {
    return { thread_detail::lockable_wrapper<Lockable>(mtx) };
  }
  template <typename Lock>
  nested_strict_lock<Lock> make_nested_strict_lock(Lock& lk)
  {
    return { thread_detail::lockable_wrapper<Lock>(lk) };
  }
#endif
}
#include <boost/config/abi_suffix.hpp>

#endif
