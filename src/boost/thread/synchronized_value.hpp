// (C) Copyright 2010 Just Software Solutions Ltd http://www.justsoftwaresolutions.co.uk
// (C) Copyright 2012 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_SYNCHRONIZED_VALUE_HPP
#define BOOST_THREAD_SYNCHRONIZED_VALUE_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/lock_factories.hpp>
#include <boost/thread/strict_lock.hpp>
#include <boost/core/swap.hpp>
#include <boost/utility/declval.hpp>
//#include <boost/type_traits.hpp>
//#include <boost/thread/detail/is_nothrow_default_constructible.hpp>
//#if ! defined BOOST_NO_CXX11_HDR_TYPE_TRAITS
//#include <type_traits>
//#endif

#if ! defined(BOOST_THREAD_NO_SYNCHRONIZE)
#include <tuple> // todo change to <boost/tuple.hpp> once Boost.Tuple or Boost.Fusion provides Move semantics on C++98 compilers.
#include <functional>
#endif

#include <boost/utility/result_of.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  /**
   * strict lock providing a const pointer access to the synchronized value type.
   *
   * @param T the value type.
   * @param Lockable the mutex type protecting the value type.
   */
  template <typename T, typename Lockable = mutex>
  class const_strict_lock_ptr
  {
  public:
    typedef T value_type;
    typedef Lockable mutex_type;
  protected:

    // this should be a strict_lock, but unique_lock is needed to be able to return it.
    boost::unique_lock<mutex_type> lk_;
    T const& value_;

  public:
    BOOST_THREAD_MOVABLE_ONLY( const_strict_lock_ptr )

    /**
     * @param value constant reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @effects locks the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    const_strict_lock_ptr(T const& val, Lockable & mtx) :
      lk_(mtx), value_(val)
    {
    }
    const_strict_lock_ptr(T const& val, Lockable & mtx, adopt_lock_t tag) BOOST_NOEXCEPT :
      lk_(mtx, tag), value_(val)
    {
    }
    /**
     * Move constructor.
     * @effects takes ownership of the mutex owned by @c other, stores a reference to the mutex and the value type of @c other.
     */
    const_strict_lock_ptr(BOOST_THREAD_RV_REF(const_strict_lock_ptr) other) BOOST_NOEXCEPT
    : lk_(boost::move(BOOST_THREAD_RV(other).lk_)),value_(BOOST_THREAD_RV(other).value_)
    {
    }

    ~const_strict_lock_ptr()
    {
    }

    /**
     * @return a constant pointer to the protected value
     */
    const T* operator->() const
    {
      return &value_;
    }

    /**
     * @return a constant reference to the protected value
     */
    const T& operator*() const
    {
      return value_;
    }

  };

  /**
   * strict lock providing a pointer access to the synchronized value type.
   *
   * @param T the value type.
   * @param Lockable the mutex type protecting the value type.
   */
  template <typename T, typename Lockable = mutex>
  class strict_lock_ptr : public const_strict_lock_ptr<T,Lockable>
  {
    typedef const_strict_lock_ptr<T,Lockable> base_type;
  public:
    BOOST_THREAD_MOVABLE_ONLY( strict_lock_ptr )

    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @effects locks the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    strict_lock_ptr(T & val, Lockable & mtx) :
    base_type(val, mtx)
    {
    }
    strict_lock_ptr(T & val, Lockable & mtx, adopt_lock_t tag) :
    base_type(val, mtx, tag)
    {
    }

    /**
     * Move constructor.
     * @effects takes ownership of the mutex owned by @c other, stores a reference to the mutex and the value type of @c other.
     */
    strict_lock_ptr(BOOST_THREAD_RV_REF(strict_lock_ptr) other)
    : base_type(boost::move(static_cast<base_type&>(other)))
    {
    }

    ~strict_lock_ptr()
    {
    }

    /**
     * @return a pointer to the protected value
     */
    T* operator->()
    {
      return const_cast<T*>(&this->value_);
    }

    /**
     * @return a reference to the protected value
     */
    T& operator*()
    {
      return const_cast<T&>(this->value_);
    }

  };

  template <typename SV>
  struct synchronized_value_strict_lock_ptr
  {
   typedef strict_lock_ptr<typename SV::value_type, typename SV::mutex_type> type;
  };

  template <typename SV>
  struct synchronized_value_strict_lock_ptr<const SV>
  {
   typedef const_strict_lock_ptr<typename SV::value_type, typename SV::mutex_type> type;
  };
  /**
   * unique_lock providing a const pointer access to the synchronized value type.
   *
   * An object of type const_unique_lock_ptr is a unique_lock that provides a const pointer access to the synchronized value type.
   * As unique_lock controls the ownership of a lockable object within a scope.
   * Ownership of the lockable object may be acquired at construction or after construction,
   * and may be transferred, after acquisition, to another const_unique_lock_ptr object.
   * Objects of type const_unique_lock_ptr are not copyable but are movable.
   * The behavior of a program is undefined if the mutex and the value type
   * pointed do not exist for the entire remaining lifetime of the const_unique_lock_ptr object.
   * The supplied Mutex type shall meet the BasicLockable requirements.
   *
   * @note const_unique_lock_ptr<T, Lockable> meets the Lockable requirements.
   * If Lockable meets the TimedLockable requirements, const_unique_lock_ptr<T,Lockable>
   * also meets the TimedLockable requirements.
   *
   * @param T the value type.
   * @param Lockable the mutex type protecting the value type.
   */
  template <typename T, typename Lockable = mutex>
  class const_unique_lock_ptr : public unique_lock<Lockable>
  {
    typedef unique_lock<Lockable> base_type;
  public:
    typedef T value_type;
    typedef Lockable mutex_type;
  protected:
    T const& value_;

  public:
    BOOST_THREAD_MOVABLE_ONLY(const_unique_lock_ptr)

    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     *
     * @requires If mutex_type is not a recursive mutex the calling thread does not own the mutex.
     *
     * @effects locks the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    const_unique_lock_ptr(T const& val, Lockable & mtx)
    : base_type(mtx), value_(val)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type adopt_lock_t used to differentiate the constructor.
     * @requires The calling thread own the mutex.
     * @effects stores a reference to it and to the value type @c value taking ownership.
     */
    const_unique_lock_ptr(T const& val, Lockable & mtx, adopt_lock_t) BOOST_NOEXCEPT
    : base_type(mtx, adopt_lock), value_(val)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type defer_lock_t used to differentiate the constructor.
     * @effects stores a reference to it and to the value type @c value c.
     */
    const_unique_lock_ptr(T const& val, Lockable & mtx, defer_lock_t) BOOST_NOEXCEPT
    : base_type(mtx, defer_lock), value_(val)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type try_to_lock_t used to differentiate the constructor.
     * @requires If mutex_type is not a recursive mutex the calling thread does not own the mutex.
     * @effects try to lock the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    const_unique_lock_ptr(T const& val, Lockable & mtx, try_to_lock_t) BOOST_NOEXCEPT
    : base_type(mtx, try_to_lock), value_(val)
    {
    }
    /**
     * Move constructor.
     * @effects takes ownership of the mutex owned by @c other, stores a reference to the mutex and the value type of @c other.
     */
    const_unique_lock_ptr(BOOST_THREAD_RV_REF(const_unique_lock_ptr) other) BOOST_NOEXCEPT
    : base_type(boost::move(static_cast<base_type&>(other))), value_(BOOST_THREAD_RV(other).value_)
    {
    }

    /**
     * @effects If owns calls unlock() on the owned mutex.
     */
    ~const_unique_lock_ptr()
    {
    }

    /**
     * @return a constant pointer to the protected value
     */
    const T* operator->() const
    {
      BOOST_ASSERT (this->owns_lock());
      return &value_;
    }

    /**
     * @return a constant reference to the protected value
     */
    const T& operator*() const
    {
      BOOST_ASSERT (this->owns_lock());
      return value_;
    }

  };

  /**
   * unique lock providing a pointer access to the synchronized value type.
   *
   * @param T the value type.
   * @param Lockable the mutex type protecting the value type.
   */
  template <typename T, typename Lockable = mutex>
  class unique_lock_ptr : public const_unique_lock_ptr<T, Lockable>
  {
    typedef const_unique_lock_ptr<T, Lockable> base_type;
  public:
    typedef T value_type;
    typedef Lockable mutex_type;

    BOOST_THREAD_MOVABLE_ONLY(unique_lock_ptr)

    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @effects locks the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    unique_lock_ptr(T & val, Lockable & mtx)
    : base_type(val, mtx)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type adopt_lock_t used to differentiate the constructor.
     * @effects stores a reference to it and to the value type @c value taking ownership.
     */
    unique_lock_ptr(T & value, Lockable & mtx, adopt_lock_t) BOOST_NOEXCEPT
    : base_type(value, mtx, adopt_lock)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type defer_lock_t used to differentiate the constructor.
     * @effects stores a reference to it and to the value type @c value c.
     */
    unique_lock_ptr(T & value, Lockable & mtx, defer_lock_t) BOOST_NOEXCEPT
    : base_type(value, mtx, defer_lock)
    {
    }
    /**
     * @param value reference of the value to protect.
     * @param mtx reference to the mutex used to protect the value.
     * @param tag of type try_to_lock_t used to differentiate the constructor.
     * @effects try to lock the mutex @c mtx, stores a reference to it and to the value type @c value.
     */
    unique_lock_ptr(T & value, Lockable & mtx, try_to_lock_t) BOOST_NOEXCEPT
    : base_type(value, mtx, try_to_lock)
    {
    }
    /**
     * Move constructor.
     * @effects takes ownership of the mutex owned by @c other, stores a reference to the mutex and the value type of @c other.
     */
    unique_lock_ptr(BOOST_THREAD_RV_REF(unique_lock_ptr) other) BOOST_NOEXCEPT
    : base_type(boost::move(static_cast<base_type&>(other)))
    {
    }

    ~unique_lock_ptr()
    {
    }

    /**
     * @return a pointer to the protected value
     */
    T* operator->()
    {
      BOOST_ASSERT (this->owns_lock());
      return const_cast<T*>(&this->value_);
    }

    /**
     * @return a reference to the protected value
     */
    T& operator*()
    {
      BOOST_ASSERT (this->owns_lock());
      return const_cast<T&>(this->value_);
    }


  };

  template <typename SV>
  struct synchronized_value_unique_lock_ptr
  {
   typedef unique_lock_ptr<typename SV::value_type, typename SV::mutex_type> type;
  };

  template <typename SV>
  struct synchronized_value_unique_lock_ptr<const SV>
  {
   typedef const_unique_lock_ptr<typename SV::value_type, typename SV::mutex_type> type;
  };
  /**
   * cloaks a value type and the mutex used to protect it together.
   * @param T the value type.
   * @param Lockable the mutex type protecting the value type.
   */
  template <typename T, typename Lockable = mutex>
  class synchronized_value
  {

#if ! defined(BOOST_THREAD_NO_MAKE_UNIQUE_LOCKS)
#if ! defined BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template <typename ...SV>
    friend std::tuple<typename synchronized_value_strict_lock_ptr<SV>::type ...> synchronize(SV& ...sv);
#else
    template <typename SV1, typename SV2>
    friend std::tuple<
      typename synchronized_value_strict_lock_ptr<SV1>::type,
      typename synchronized_value_strict_lock_ptr<SV2>::type
    >
    synchronize(SV1& sv1, SV2& sv2);
    template <typename SV1, typename SV2, typename SV3>
    friend std::tuple<
      typename synchronized_value_strict_lock_ptr<SV1>::type,
      typename synchronized_value_strict_lock_ptr<SV2>::type,
      typename synchronized_value_strict_lock_ptr<SV3>::type
    >
    synchronize(SV1& sv1, SV2& sv2, SV3& sv3);
#endif
#endif

  public:
    typedef T value_type;
    typedef Lockable mutex_type;
  private:
    T value_;
    mutable mutex_type mtx_;
  public:
    // construction/destruction
    /**
     * Default constructor.
     *
     * @Requires: T is DefaultConstructible
     */
    synchronized_value()
    //BOOST_NOEXCEPT_IF(is_nothrow_default_constructible<T>::value)
    : value_()
    {
    }

    /**
     * Constructor from copy constructible value.
     *
     * Requires: T is CopyConstructible
     */
    synchronized_value(T const& other)
    //BOOST_NOEXCEPT_IF(is_nothrow_copy_constructible<T>::value)
    : value_(other)
    {
    }

    /**
     * Move Constructor.
     *
     * Requires: T is CopyMovable
     */
    synchronized_value(BOOST_THREAD_RV_REF(T) other)
    //BOOST_NOEXCEPT_IF(is_nothrow_move_constructible<T>::value)
    : value_(boost::move(other))
    {
    }

    /**
     * Constructor from value type.
     *
     * Requires: T is DefaultConstructible and Assignable
     * Effects: Assigns the value on a scope protected by the mutex of the rhs. The mutex is not copied.
     */
    synchronized_value(synchronized_value const& rhs)
    {
      strict_lock<mutex_type> lk(rhs.mtx_);
      value_ = rhs.value_;
    }

    /**
     * Move Constructor from movable value type
     *
     */
    synchronized_value(BOOST_THREAD_RV_REF(synchronized_value) other)
    {
      strict_lock<mutex_type> lk(BOOST_THREAD_RV(other).mtx_);
      value_= boost::move(BOOST_THREAD_RV(other).value_);
    }

    // mutation
    /**
     * Assignment operator.
     *
     * Effects: Copies the underlying value on a scope protected by the two mutexes.
     * The mutex is not copied. The locks are acquired using lock, so deadlock is avoided.
     * For example, there is no problem if one thread assigns a = b and the other assigns b = a.
     *
     * Return: *this
     */

    synchronized_value& operator=(synchronized_value const& rhs)
    {
      if(&rhs != this)
      {
        // auto _ = make_unique_locks(mtx_, rhs.mtx_);
        unique_lock<mutex_type> lk1(mtx_, defer_lock);
        unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
        lock(lk1,lk2);

        value_ = rhs.value_;
      }
      return *this;
    }
    /**
     * Assignment operator from a T const&.
     * Effects: The operator copies the value on a scope protected by the mutex.
     * Return: *this
     */
    synchronized_value& operator=(value_type const& val)
    {
      {
        strict_lock<mutex_type> lk(mtx_);
        value_ = val;
      }
      return *this;
    }

    //observers
    /**
     * Explicit conversion to value type.
     *
     * Requires: T is CopyConstructible
     * Return: A copy of the protected value obtained on a scope protected by the mutex.
     *
     */
    T get() const
    {
      strict_lock<mutex_type> lk(mtx_);
      return value_;
    }
    /**
     * Explicit conversion to value type.
     *
     * Requires: T is CopyConstructible
     * Return: A copy of the protected value obtained on a scope protected by the mutex.
     *
     */
#if ! defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
    explicit operator T() const
    {
      return get();
    }
#endif

    /**
     * value type getter.
     *
     * Return: A constant reference to the protected value.
     *
     * Note: Not thread safe
     *
     */
    T const& value() const
    {
      return value_;
    }
    /**
     * mutex getter.
     *
     * Return: A constant reference to the protecting mutex.
     *
     * Note: Not thread safe
     *
     */
    mutex_type const& mutex() const
    {
      return mtx_;
    }
    /**
     * Swap
     *
     * Effects: Swaps the data. Again, locks are acquired using lock(). The mutexes are not swapped.
     * A swap method accepts a T& and swaps the data inside a critical section.
     * This is by far the preferred method of changing the guarded datum wholesale because it keeps the lock only
     * for a short time, thus lowering the pressure on the mutex.
     */
    void swap(synchronized_value & rhs)
    {
      if (this == &rhs) {
        return;
      }
      // auto _ = make_unique_locks(mtx_, rhs.mtx_);
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);
      boost::swap(value_, rhs.value_);
    }
    /**
     * Swap with the underlying value type
     *
     * Effects: Swaps the data on a scope protected by the mutex.
     */
    void swap(value_type & rhs)
    {
      strict_lock<mutex_type> lk(mtx_);
      boost::swap(value_, rhs);
    }

    /**
     * Essentially calling a method obj->foo(x, y, z) calls the method foo(x, y, z) inside a critical section as
     * long-lived as the call itself.
     */
    strict_lock_ptr<T,Lockable> operator->()
    {
      return BOOST_THREAD_MAKE_RV_REF((strict_lock_ptr<T,Lockable>(value_, mtx_)));
    }
    /**
     * If the synchronized_value object involved is const-qualified, then you'll only be able to call const methods
     * through operator->. So, for example, vec->push_back("xyz") won't work if vec were const-qualified.
     * The locking mechanism capitalizes on the assumption that const methods don't modify their underlying data.
     */
    const_strict_lock_ptr<T,Lockable> operator->() const
    {
      return BOOST_THREAD_MAKE_RV_REF((const_strict_lock_ptr<T,Lockable>(value_, mtx_)));
    }

    /**
     * Call function on a locked block.
     *
     * @requires fct(value_) is well formed.
     *
     * Example
     *   void fun(synchronized_value<vector<int>> & v) {
     *     v ( [](vector<int>> & vec)
     *     {
     *       vec.push_back(42);
     *       assert(vec.back() == 42);
     *     } );
     *   }
     */
    template <typename F>
    inline
    typename boost::result_of<F(value_type&)>::type
    operator()(BOOST_THREAD_RV_REF(F) fct)
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }
    template <typename F>
    inline
    typename boost::result_of<F(value_type const&)>::type
    operator()(BOOST_THREAD_RV_REF(F) fct) const
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }


#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename F>
    inline
    typename boost::result_of<F(value_type&)>::type
    operator()(F const & fct)
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }
    template <typename F>
    inline
    typename boost::result_of<F(value_type const&)>::type
    operator()(F const & fct) const
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }

    template <typename R>
    inline
    R operator()(R(*fct)(value_type&))
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }
    template <typename R>
    inline
    R operator()(R(*fct)(value_type const&)) const
    {
      strict_lock<mutex_type> lk(mtx_);
      return fct(value_);
    }
#endif


    /**
     * The synchronize() factory make easier to lock on a scope.
     * As discussed, operator-> can only lock over the duration of a call, so it is insufficient for complex operations.
     * With synchronize() you get to lock the object in a scoped and to directly access the object inside that scope.
     *
     * Example
     *   void fun(synchronized_value<vector<int>> & v) {
     *     auto&& vec=v.synchronize();
     *     vec.push_back(42);
     *     assert(vec.back() == 42);
     *   }
     */
    strict_lock_ptr<T,Lockable> synchronize()
    {
      return BOOST_THREAD_MAKE_RV_REF((strict_lock_ptr<T,Lockable>(value_, mtx_)));
    }
    const_strict_lock_ptr<T,Lockable> synchronize() const
    {
      return BOOST_THREAD_MAKE_RV_REF((const_strict_lock_ptr<T,Lockable>(value_, mtx_)));
    }

    unique_lock_ptr<T,Lockable> unique_synchronize()
    {
      return BOOST_THREAD_MAKE_RV_REF((unique_lock_ptr<T,Lockable>(value_, mtx_)));
    }
    const_unique_lock_ptr<T,Lockable> unique_synchronize() const
    {
      return BOOST_THREAD_MAKE_RV_REF((const_unique_lock_ptr<T,Lockable>(value_, mtx_)));
    }
    unique_lock_ptr<T,Lockable> unique_synchronize(defer_lock_t tag)
    {
      return BOOST_THREAD_MAKE_RV_REF((unique_lock_ptr<T,Lockable>(value_, mtx_, tag)));
    }
    const_unique_lock_ptr<T,Lockable> unique_synchronize(defer_lock_t tag) const
    {
      return BOOST_THREAD_MAKE_RV_REF((const_unique_lock_ptr<T,Lockable>(value_, mtx_, tag)));
    }
    unique_lock_ptr<T,Lockable> defer_synchronize() BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((unique_lock_ptr<T,Lockable>(value_, mtx_, defer_lock)));
    }
    const_unique_lock_ptr<T,Lockable> defer_synchronize() const BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((const_unique_lock_ptr<T,Lockable>(value_, mtx_, defer_lock)));
    }
    unique_lock_ptr<T,Lockable> try_to_synchronize() BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((unique_lock_ptr<T,Lockable>(value_, mtx_, try_to_lock)));
    }
    const_unique_lock_ptr<T,Lockable> try_to_synchronize() const BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((const_unique_lock_ptr<T,Lockable>(value_, mtx_, try_to_lock)));
    }
    unique_lock_ptr<T,Lockable> adopt_synchronize() BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((unique_lock_ptr<T,Lockable>(value_, mtx_, adopt_lock)));
    }
    const_unique_lock_ptr<T,Lockable> adopt_synchronize() const BOOST_NOEXCEPT
    {
      return BOOST_THREAD_MAKE_RV_REF((const_unique_lock_ptr<T,Lockable>(value_, mtx_, adopt_lock)));
    }


#if ! defined __IBMCPP__
    private:
#endif
    class deref_value
    {
    private:
      friend class synchronized_value;

      boost::unique_lock<mutex_type> lk_;
      T& value_;

      explicit deref_value(synchronized_value& outer):
      lk_(outer.mtx_),value_(outer.value_)
      {}

    public:
      BOOST_THREAD_MOVABLE_ONLY(deref_value)

      deref_value(BOOST_THREAD_RV_REF(deref_value) other):
      lk_(boost::move(BOOST_THREAD_RV(other).lk_)),value_(BOOST_THREAD_RV(other).value_)
      {}
      operator T&()
      {
        return value_;
      }

      deref_value& operator=(T const& newVal)
      {
        value_=newVal;
        return *this;
      }
    };
    class const_deref_value
    {
    private:
      friend class synchronized_value;

      boost::unique_lock<mutex_type> lk_;
      const T& value_;

      explicit const_deref_value(synchronized_value const& outer):
      lk_(outer.mtx_), value_(outer.value_)
      {}

    public:
      BOOST_THREAD_MOVABLE_ONLY(const_deref_value)

      const_deref_value(BOOST_THREAD_RV_REF(const_deref_value) other):
      lk_(boost::move(BOOST_THREAD_RV(other).lk_)), value_(BOOST_THREAD_RV(other).value_)
      {}

      operator const T&()
      {
        return value_;
      }
    };

  public:
    deref_value operator*()
    {
      return BOOST_THREAD_MAKE_RV_REF(deref_value(*this));
    }

    const_deref_value operator*() const
    {
      return BOOST_THREAD_MAKE_RV_REF(const_deref_value(*this));
    }

    // io functions
    /**
     * @requires T is OutputStreamable
     * @effects saves the value type on the output stream @c os.
     */
    template <typename OStream>
    void save(OStream& os) const
    {
      strict_lock<mutex_type> lk(mtx_);
      os << value_;
    }
    /**
     * @requires T is InputStreamable
     * @effects loads the value type from the input stream @c is.
     */
    template <typename IStream>
    void load(IStream& is)
    {
      strict_lock<mutex_type> lk(mtx_);
      is >> value_;
    }

    // relational operators
    /**
     * @requires T is EqualityComparable
     *
     */
    bool operator==(synchronized_value const& rhs)  const
    {
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);

      return value_ == rhs.value_;
    }
    /**
     * @requires T is LessThanComparable
     *
     */
    bool operator<(synchronized_value const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);

      return value_ < rhs.value_;
    }
    /**
     * @requires T is GreaterThanComparable
     *
     */
    bool operator>(synchronized_value const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);

      return value_ > rhs.value_;
    }
    bool operator<=(synchronized_value const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);

      return value_ <= rhs.value_;
    }
    bool operator>=(synchronized_value const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_, defer_lock);
      unique_lock<mutex_type> lk2(rhs.mtx_, defer_lock);
      lock(lk1,lk2);

      return value_ >= rhs.value_;
    }
    bool operator==(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ == rhs;
    }
    bool operator!=(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ != rhs;
    }
    bool operator<(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ < rhs;
    }
    bool operator<=(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ <= rhs;
    }
    bool operator>(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ > rhs;
    }
    bool operator>=(value_type const& rhs) const
    {
      unique_lock<mutex_type> lk1(mtx_);

      return value_ >= rhs;
    }

  };

  // Specialized algorithms
  /**
   *
   */
  template <typename T, typename L>
  inline void swap(synchronized_value<T,L> & lhs, synchronized_value<T,L> & rhs)
  {
    lhs.swap(rhs);
  }
  template <typename T, typename L>
  inline void swap(synchronized_value<T,L> & lhs, T & rhs)
  {
    lhs.swap(rhs);
  }
  template <typename T, typename L>
  inline void swap(T & lhs, synchronized_value<T,L> & rhs)
  {
    rhs.swap(lhs);
  }

  //Hash support

//  template <class T> struct hash;
//  template <typename T, typename L>
//  struct hash<synchronized_value<T,L> >;

  // Comparison with T
  template <typename T, typename L>
  bool operator!=(synchronized_value<T,L> const&lhs, synchronized_value<T,L> const& rhs)
  {
    return ! (lhs==rhs);
  }

  template <typename T, typename L>
  bool operator==(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs==lhs;
  }
  template <typename T, typename L>
  bool operator!=(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs!=lhs;
  }
  template <typename T, typename L>
  bool operator<(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs>lhs;
  }
  template <typename T, typename L>
  bool operator<=(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs>=lhs;
  }
  template <typename T, typename L>
  bool operator>(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs<lhs;
  }
  template <typename T, typename L>
  bool operator>=(T const& lhs, synchronized_value<T,L> const&rhs)
  {
    return rhs<=lhs;
  }

  /**
   *
   */
  template <typename OStream, typename T, typename L>
  inline OStream& operator<<(OStream& os, synchronized_value<T,L> const& rhs)
  {
    rhs.save(os);
    return os;
  }
  template <typename IStream, typename T, typename L>
  inline IStream& operator>>(IStream& is, synchronized_value<T,L>& rhs)
  {
    rhs.load(is);
    return is;
  }

#if ! defined(BOOST_THREAD_NO_SYNCHRONIZE)
#if ! defined BOOST_NO_CXX11_VARIADIC_TEMPLATES

  template <typename ...SV>
  std::tuple<typename synchronized_value_strict_lock_ptr<SV>::type ...> synchronize(SV& ...sv)
  {
    boost::lock(sv.mtx_ ...);
    typedef std::tuple<typename synchronized_value_strict_lock_ptr<SV>::type ...> t_type;

    return t_type(typename synchronized_value_strict_lock_ptr<SV>::type(sv.value_, sv.mtx_, adopt_lock) ...);
  }
#else

  template <typename SV1, typename SV2>
  std::tuple<
    typename synchronized_value_strict_lock_ptr<SV1>::type,
    typename synchronized_value_strict_lock_ptr<SV2>::type
  >
  synchronize(SV1& sv1, SV2& sv2)
  {
    boost::lock(sv1.mtx_, sv2.mtx_);
    typedef std::tuple<
        typename synchronized_value_strict_lock_ptr<SV1>::type,
        typename synchronized_value_strict_lock_ptr<SV2>::type
        > t_type;

    return t_type(
        typename synchronized_value_strict_lock_ptr<SV1>::type(sv1.value_, sv1.mtx_, adopt_lock),
        typename synchronized_value_strict_lock_ptr<SV2>::type(sv2.value_, sv2.mtx_, adopt_lock)
        );

  }
  template <typename SV1, typename SV2, typename SV3>
  std::tuple<
    typename synchronized_value_strict_lock_ptr<SV1>::type,
    typename synchronized_value_strict_lock_ptr<SV2>::type,
    typename synchronized_value_strict_lock_ptr<SV3>::type
  >
  synchronize(SV1& sv1, SV2& sv2, SV3& sv3)
  {
    boost::lock(sv1.mtx_, sv2.mtx_);
    typedef std::tuple<
        typename synchronized_value_strict_lock_ptr<SV1>::type,
        typename synchronized_value_strict_lock_ptr<SV2>::type,
        typename synchronized_value_strict_lock_ptr<SV3>::type
        > t_type;

    return t_type(
        typename synchronized_value_strict_lock_ptr<SV1>::type(sv1.value_, sv1.mtx_, adopt_lock),
        typename synchronized_value_strict_lock_ptr<SV2>::type(sv2.value_, sv2.mtx_, adopt_lock),
        typename synchronized_value_strict_lock_ptr<SV3>::type(sv3.value_, sv3.mtx_, adopt_lock)
        );

  }
#endif
#endif
}

#include <boost/config/abi_suffix.hpp>

#endif // header
