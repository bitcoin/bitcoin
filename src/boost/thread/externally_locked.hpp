// (C) Copyright 2012 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_EXTERNALLY_LOCKED_HPP
#define BOOST_THREAD_EXTERNALLY_LOCKED_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/exceptions.hpp>
#include <boost/thread/lock_concepts.hpp>
#include <boost/thread/lock_traits.hpp>
#include <boost/thread/lockable_concepts.hpp>
#include <boost/thread/strict_lock.hpp>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/throw_exception.hpp>
#include <boost/core/swap.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  class mutex;

  /**
   * externally_locked cloaks an object of type T, and actually provides full
   * access to that object through the get and set member functions, provided you
   * pass a reference to a strict lock object
   */

  //[externally_locked
  template <typename T, typename MutexType = boost::mutex>
  class externally_locked;
  template <typename T, typename MutexType>
  class externally_locked
  {
    //BOOST_CONCEPT_ASSERT(( CopyConstructible<T> ));
    BOOST_CONCEPT_ASSERT(( BasicLockable<MutexType> ));

  public:
    typedef MutexType mutex_type;

    BOOST_THREAD_COPYABLE_AND_MOVABLE( externally_locked )
    /**
     * Requires: T is a model of CopyConstructible.
     * Effects: Constructs an externally locked object copying the cloaked type.
     */
    externally_locked(mutex_type& mtx, const T& obj) :
      obj_(obj), mtx_(&mtx)
    {
    }

    /**
     * Requires: T is a model of Movable.
     * Effects: Constructs an externally locked object by moving the cloaked type.
     */
    externally_locked(mutex_type& mtx, BOOST_THREAD_RV_REF(T) obj) :
      obj_(move(obj)), mtx_(&mtx)
    {
    }

    /**
     * Requires: T is a model of DefaultConstructible.
     * Effects: Constructs an externally locked object initializing the cloaked type with the default constructor.
     */
    externally_locked(mutex_type& mtx) // BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(T()))
    : obj_(), mtx_(&mtx)
    {
    }

    /**
     *  Copy constructor
     */
    externally_locked(externally_locked const& rhs) //BOOST_NOEXCEPT
    : obj_(rhs.obj_), mtx_(rhs.mtx_)
    {
    }
    /**
     *  Move constructor
     */
    externally_locked(BOOST_THREAD_RV_REF(externally_locked) rhs) //BOOST_NOEXCEPT
    : obj_(move(rhs.obj_)), mtx_(rhs.mtx_)
    {
    }

    /// assignment
    externally_locked& operator=(externally_locked const& rhs) //BOOST_NOEXCEPT
    {
      obj_=rhs.obj_;
      mtx_=rhs.mtx_;
      return *this;
    }

    /// move assignment
    externally_locked& operator=(BOOST_THREAD_RV_REF(externally_locked) rhs) // BOOST_NOEXCEPT
    {
      obj_=move(BOOST_THREAD_RV(rhs).obj_);
      mtx_=rhs.mtx_;
      return *this;
    }

    void swap(externally_locked& rhs) //BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR)
    {
      swap(obj_, rhs.obj_);
      swap(mtx_, rhs.mtx_);
    }

    /**
     * Requires: The lk parameter must be locking the associated mtx.
     *
     * Returns: The address of the cloaked object..
     *
     * Throws: lock_error if BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is not defined and the lk parameter doesn't satisfy the preconditions
     */
    T& get(strict_lock<mutex_type>& lk)
    {
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return obj_;
    }

    const T& get(strict_lock<mutex_type>& lk) const
    {
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return obj_;
    }

    template <class Lock>
    T& get(nested_strict_lock<Lock>& lk)
    {
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return obj_;
    }

    template <class Lock>
    const T& get(nested_strict_lock<Lock>& lk) const
    {
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return obj_;
    }

    /**
     * Requires: The lk parameter must be locking the associated mtx.
     * Returns: The address of the cloaked object..
     *
     * Throws: lock_error if BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is not defined and the lk parameter doesn't satisfy the preconditions
     */
    template <class Lock>
    T& get(Lock& lk)
    {
      BOOST_CONCEPT_ASSERT(( StrictLock<Lock> ));
      BOOST_STATIC_ASSERT( (is_strict_lock<Lock>::value)); /*< lk is a strict lock "sur parolle" >*/
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/

      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/

      return obj_;
    }

    mutex_type* mutex() const BOOST_NOEXCEPT
    {
      return mtx_;
    }

    // modifiers

    void lock()
    {
      mtx_->lock();
    }
    void unlock()
    {
      mtx_->unlock();
    }
    bool try_lock()
    {
      return mtx_->try_lock();
    }
    // todo add time related functions

  private:
    T obj_;
    mutex_type* mtx_;
  };
  //]

  /**
   * externally_locked<T&,M> specialization for T& that cloaks an reference to an object of type T, and actually
   * provides full access to that object through the get and set member functions, provided you
   * pass a reference to a strict lock object.
   */

  //[externally_locked_ref
  template <typename T, typename MutexType>
  class externally_locked<T&, MutexType>
  {
    //BOOST_CONCEPT_ASSERT(( CopyConstructible<T> ));
    BOOST_CONCEPT_ASSERT(( BasicLockable<MutexType> ));

  public:
    typedef MutexType mutex_type;

    BOOST_THREAD_COPYABLE_AND_MOVABLE( externally_locked )

    /**
     * Effects: Constructs an externally locked object storing the cloaked reference object.
     */
    externally_locked(T& obj, mutex_type& mtx) BOOST_NOEXCEPT :
      obj_(&obj), mtx_(&mtx)
    {
    }

    /// copy constructor
    externally_locked(externally_locked const& rhs) BOOST_NOEXCEPT :
    obj_(rhs.obj_), mtx_(rhs.mtx_)
    {
    }

    /// move constructor
    externally_locked(BOOST_THREAD_RV_REF(externally_locked) rhs) BOOST_NOEXCEPT :
    obj_(rhs.obj_), mtx_(rhs.mtx_)
    {
    }

    /// assignment
    externally_locked& operator=(externally_locked const& rhs) BOOST_NOEXCEPT
    {
      obj_=rhs.obj_;
      mtx_=rhs.mtx_;
      return *this;
    }

    /// move assignment
    externally_locked& operator=(BOOST_THREAD_RV_REF(externally_locked) rhs) BOOST_NOEXCEPT
    {
      obj_=rhs.obj_;
      mtx_=rhs.mtx_;
      return *this;
    }

    void swap(externally_locked& rhs) BOOST_NOEXCEPT
    {
      swap(obj_, rhs.obj_);
      swap(mtx_, rhs.mtx_);
    }
    /**
     * Requires: The lk parameter must be locking the associated mtx.
     *
     * Returns: The address of the cloaked object..
     *
     * Throws: lock_error if BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is not defined and the lk parameter doesn't satisfy the preconditions
     */
    T& get(strict_lock<mutex_type> const& lk)
    {
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }

    const T& get(strict_lock<mutex_type> const& lk) const
    {
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }

    template <class Lock>
    T& get(nested_strict_lock<Lock> const& lk)
    {
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }

    template <class Lock>
    const T& get(nested_strict_lock<Lock> const& lk) const
    {
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }

    /**
     * Requires: The lk parameter must be locking the associated mtx.
     * Returns: The address of the cloaked object..
     *
     * Throws: lock_error if BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is not defined and the lk parameter doesn't satisfy the preconditions
     */
    template <class Lock>
    T& get(Lock const& lk)
    {
      BOOST_CONCEPT_ASSERT(( StrictLock<Lock> ));
      BOOST_STATIC_ASSERT( (is_strict_lock<Lock>::value)); /*< lk is a strict lock "sur parolle" >*/
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }

    /**
     * Requires: The lk parameter must be locking the associated mtx.
     * Returns: The address of the cloaked object..
     *
     * Throws: lock_error if BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED is not defined and the lk parameter doesn't satisfy the preconditions
     */
    template <class Lock>
    T const& get(Lock const& lk) const
    {
      BOOST_CONCEPT_ASSERT(( StrictLock<Lock> ));
      BOOST_STATIC_ASSERT( (is_strict_lock<Lock>::value)); /*< lk is a strict lock "sur parolle" >*/
      BOOST_STATIC_ASSERT( (is_same<mutex_type, typename Lock::mutex_type>::value)); /*< that locks the same type >*/
      BOOST_THREAD_ASSERT_PRECONDITION(  lk.owns_lock(mtx_), lock_error() ); /*< run time check throw if not locks the same >*/
      return *obj_;
    }
    mutex_type* mutex() const BOOST_NOEXCEPT
    {
      return mtx_;
    }

    void lock()
    {
      mtx_->lock();
    }
    void unlock()
    {
      mtx_->unlock();
    }
    bool try_lock()
    {
      return mtx_->try_lock();
    }
    // todo add time related functions

  protected:
    T* obj_;
    mutex_type* mtx_;
  };
  //]

  template <typename T, typename MutexType>
  void swap(externally_locked<T, MutexType> & lhs, externally_locked<T, MutexType> & rhs) // BOOST_NOEXCEPT
  {
    lhs.swap(rhs);
  }

}

#include <boost/config/abi_suffix.hpp>

#endif // header
