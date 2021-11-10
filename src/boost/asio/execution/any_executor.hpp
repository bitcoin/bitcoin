//
// execution/any_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_ANY_EXECUTOR_HPP
#define BOOST_ASIO_EXECUTION_ANY_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <new>
#include <typeinfo>
#include <boost/asio/detail/assert.hpp>
#include <boost/asio/detail/cstddef.hpp>
#include <boost/asio/detail/executor_function.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <boost/asio/detail/scoped_ptr.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/throw_exception.hpp>
#include <boost/asio/detail/variadic_templates.hpp>
#include <boost/asio/execution/bad_executor.hpp>
#include <boost/asio/execution/blocking.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/prefer.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/require.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// Polymorphic executor wrapper.
template <typename... SupportableProperties>
class any_executor
{
public:
  /// Default constructor.
  any_executor() noexcept;

  /// Construct in an empty state. Equivalent effects to default constructor.
  any_executor(nullptr_t) noexcept;

  /// Copy constructor.
  any_executor(const any_executor& e) noexcept;

  /// Move constructor.
  any_executor(any_executor&& e) noexcept;

  /// Construct to point to the same target as another any_executor.
  template <class... OtherSupportableProperties>
    any_executor(any_executor<OtherSupportableProperties...> e);

  /// Construct a polymorphic wrapper for the specified executor.
  template <typename Executor>
  any_executor(Executor e);

  /// Assignment operator.
  any_executor& operator=(const any_executor& e) noexcept;

  /// Move assignment operator.
  any_executor& operator=(any_executor&& e) noexcept;

  /// Assignment operator that sets the polymorphic wrapper to the empty state.
  any_executor& operator=(nullptr_t);

  /// Assignment operator to create a polymorphic wrapper for the specified
  /// executor.
  template <typename Executor>
  any_executor& operator=(Executor e);

  /// Destructor.
  ~any_executor();

  /// Swap targets with another polymorphic wrapper.
  void swap(any_executor& other) noexcept;

  /// Obtain a polymorphic wrapper with the specified property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require and boost::asio::prefer customisation points.
   *
   * For example:
   * @code execution::any_executor<execution::blocking_t::possibly_t> ex = ...;
   * auto ex2 = boost::asio::requre(ex, execution::blocking.possibly); @endcode
   */
  template <typename Property>
  any_executor require(Property) const;

  /// Obtain a polymorphic wrapper with the specified property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::prefer customisation point.
   *
   * For example:
   * @code execution::any_executor<execution::blocking_t::possibly_t> ex = ...;
   * auto ex2 = boost::asio::prefer(ex, execution::blocking.possibly); @endcode
   */
  template <typename Property>
  any_executor prefer(Property) const;

  /// Obtain the value associated with the specified property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code execution::any_executor<execution::occupancy_t> ex = ...;
   * size_t n = boost::asio::query(ex, execution::occupancy); @endcode
   */
  template <typename Property>
  typename Property::polymorphic_query_result_type query(Property) const;

  /// Execute the function on the target executor.
  /**
   * Do not call this function directly. It is intended for use with the
   * execution::execute customisation point.
   *
   * For example:
   * @code execution::any_executor<> ex = ...;
   * execution::execute(ex, my_function_object); @endcode
   *
   * Throws boost::asio::bad_executor if the polymorphic wrapper has no target.
   */
  template <typename Function>
  void execute(Function&& f) const;

  /// Obtain the underlying execution context.
  /**
   * This function is provided for backward compatibility. It is automatically
   * defined when the @c SupportableProperties... list includes a property of
   * type <tt>execution::context_as<U></tt>, for some type <tt>U</tt>.
   */
  automatically_determined context() const;

  /// Determine whether the wrapper has a target executor.
  /**
   * @returns @c true if the polymorphic wrapper has a target executor,
   * otherwise false.
   */
  explicit operator bool() const noexcept;

  /// Get the type of the target executor.
  const type_info& target_type() const noexcept;

  /// Get a pointer to the target executor.
  template <typename Executor> Executor* target() noexcept;

  /// Get a pointer to the target executor.
  template <typename Executor> const Executor* target() const noexcept;
};

/// Equality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator==(const any_executor<SupportableProperties...>& a,
    const any_executor<SupportableProperties...>& b) noexcept;

/// Equality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator==(const any_executor<SupportableProperties...>& a,
    nullptr_t) noexcept;

/// Equality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator==(nullptr_t,
    const any_executor<SupportableProperties...>& b) noexcept;

/// Inequality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator!=(const any_executor<SupportableProperties...>& a,
    const any_executor<SupportableProperties...>& b) noexcept;

/// Inequality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator!=(const any_executor<SupportableProperties...>& a,
    nullptr_t) noexcept;

/// Inequality operator.
/**
 * @relates any_executor
 */
template <typename... SupportableProperties>
bool operator!=(nullptr_t,
    const any_executor<SupportableProperties...>& b) noexcept;

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {

#if !defined(BOOST_ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)
#define BOOST_ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... SupportableProperties>
class any_executor;

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void>
class any_executor;

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#endif // !defined(BOOST_ASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)

template <typename U>
struct context_as_t;

namespace detail {

// Traits used to detect whether a property is requirable or preferable, taking
// into account that T::is_requirable or T::is_preferable may not not be well
// formed.

template <typename T, typename = void>
struct is_requirable : false_type {};

template <typename T>
struct is_requirable<T, typename enable_if<T::is_requirable>::type> :
  true_type {};

template <typename T, typename = void>
struct is_preferable : false_type {};

template <typename T>
struct is_preferable<T, typename enable_if<T::is_preferable>::type> :
  true_type {};

// Trait used to detect context_as property, for backward compatibility.

template <typename T>
struct is_context_as : false_type {};

template <typename U>
struct is_context_as<context_as_t<U> > : true_type {};

// Helper template to:
// - Check if a target can supply the supportable properties.
// - Find the first convertible-from-T property in the list.

template <std::size_t I, typename Props>
struct supportable_properties;

template <std::size_t I, typename Prop>
struct supportable_properties<I, void(Prop)>
{
  template <typename T>
  struct is_valid_target : integral_constant<bool,
      (
        is_requirable<Prop>::value
          ? can_require<T, Prop>::value
          : true
      )
      &&
      (
        is_preferable<Prop>::value
          ? can_prefer<T, Prop>::value
          : true
      )
      &&
      (
        !is_requirable<Prop>::value && !is_preferable<Prop>::value
          ? can_query<T, Prop>::value
          : true
      )
    >
  {
  };

  struct found
  {
    BOOST_ASIO_STATIC_CONSTEXPR(bool, value = true);
    typedef Prop type;
    typedef typename Prop::polymorphic_query_result_type query_result_type;
    BOOST_ASIO_STATIC_CONSTEXPR(std::size_t, index = I);
  };

  struct not_found
  {
    BOOST_ASIO_STATIC_CONSTEXPR(bool, value = false);
  };

  template <typename T>
  struct find_convertible_property :
      conditional<
        is_same<T, Prop>::value || is_convertible<T, Prop>::value,
        found,
        not_found
      >::type {};

  template <typename T>
  struct find_convertible_requirable_property :
      conditional<
        is_requirable<Prop>::value
          && (is_same<T, Prop>::value || is_convertible<T, Prop>::value),
        found,
        not_found
      >::type {};

  template <typename T>
  struct find_convertible_preferable_property :
      conditional<
        is_preferable<Prop>::value
          && (is_same<T, Prop>::value || is_convertible<T, Prop>::value),
        found,
        not_found
      >::type {};

  struct find_context_as_property :
      conditional<
        is_context_as<Prop>::value,
        found,
        not_found
      >::type {};
};

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <std::size_t I, typename Head, typename... Tail>
struct supportable_properties<I, void(Head, Tail...)>
{
  template <typename T>
  struct is_valid_target : integral_constant<bool,
      (
        supportable_properties<I,
          void(Head)>::template is_valid_target<T>::value
        &&
        supportable_properties<I + 1,
          void(Tail...)>::template is_valid_target<T>::value
      )
    >
  {
  };

  template <typename T>
  struct find_convertible_property :
      conditional<
        is_convertible<T, Head>::value,
        typename supportable_properties<I, void(Head)>::found,
        typename supportable_properties<I + 1,
            void(Tail...)>::template find_convertible_property<T>
      >::type {};

  template <typename T>
  struct find_convertible_requirable_property :
      conditional<
        is_requirable<Head>::value
          && is_convertible<T, Head>::value,
        typename supportable_properties<I, void(Head)>::found,
        typename supportable_properties<I + 1,
            void(Tail...)>::template find_convertible_requirable_property<T>
      >::type {};

  template <typename T>
  struct find_convertible_preferable_property :
      conditional<
        is_preferable<Head>::value
          && is_convertible<T, Head>::value,
        typename supportable_properties<I, void(Head)>::found,
        typename supportable_properties<I + 1,
            void(Tail...)>::template find_convertible_preferable_property<T>
      >::type {};

  struct find_context_as_property :
      conditional<
        is_context_as<Head>::value,
        typename supportable_properties<I, void(Head)>::found,
        typename supportable_properties<I + 1,
            void(Tail...)>::find_context_as_property
      >::type {};
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROPS_BASE_DEF(n) \
  template <std::size_t I, \
    typename Head, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct supportable_properties<I, \
      void(Head, BOOST_ASIO_VARIADIC_TARGS(n))> \
  { \
    template <typename T> \
    struct is_valid_target : integral_constant<bool, \
        ( \
          supportable_properties<I, \
            void(Head)>::template is_valid_target<T>::value \
          && \
          supportable_properties<I + 1, \
              void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
                is_valid_target<T>::value \
        ) \
      > \
    { \
    }; \
  \
    template <typename T> \
    struct find_convertible_property : \
        conditional< \
          is_convertible<T, Head>::value, \
          typename supportable_properties<I, void(Head)>::found, \
          typename supportable_properties<I + 1, \
              void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
                find_convertible_property<T> \
        >::type {}; \
  \
    template <typename T> \
    struct find_convertible_requirable_property : \
        conditional< \
          is_requirable<Head>::value \
            && is_convertible<T, Head>::value, \
          typename supportable_properties<I, void(Head)>::found, \
          typename supportable_properties<I + 1, \
              void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
                find_convertible_requirable_property<T> \
        >::type {}; \
  \
    template <typename T> \
    struct find_convertible_preferable_property : \
        conditional< \
          is_preferable<Head>::value \
            && is_convertible<T, Head>::value, \
          typename supportable_properties<I, void(Head)>::found, \
          typename supportable_properties<I + 1, \
              void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
                find_convertible_preferable_property<T> \
        >::type {}; \
  \
    struct find_context_as_property : \
        conditional< \
          is_context_as<Head>::value, \
          typename supportable_properties<I, void(Head)>::found, \
          typename supportable_properties<I + 1, void( \
            BOOST_ASIO_VARIADIC_TARGS(n))>::find_context_as_property \
        >::type {}; \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROPS_BASE_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROPS_BASE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename Props>
struct is_valid_target_executor :
  conditional<
    is_executor<T>::value,
    typename supportable_properties<0, Props>::template is_valid_target<T>,
    false_type
  >::type
{
};

class any_executor_base
{
public:
  any_executor_base() BOOST_ASIO_NOEXCEPT
    : object_fns_(object_fns_table<void>()),
      target_(0),
      target_fns_(target_fns_table<void>())
  {
  }

  template <BOOST_ASIO_EXECUTION_EXECUTOR Executor>
  any_executor_base(Executor ex, false_type)
    : target_fns_(target_fns_table<Executor>(
          any_executor_base::query_blocking(ex,
            can_query<const Executor&, const execution::blocking_t&>())
          == execution::blocking.always))
  {
    any_executor_base::construct_object(ex,
        integral_constant<bool,
          sizeof(Executor) <= sizeof(object_type)
            && alignment_of<Executor>::value <= alignment_of<object_type>::value
        >());
  }

  template <BOOST_ASIO_EXECUTION_EXECUTOR Executor>
  any_executor_base(Executor other, true_type)
    : object_fns_(object_fns_table<boost::asio::detail::shared_ptr<void> >()),
      target_fns_(other.target_fns_)
  {
    boost::asio::detail::shared_ptr<Executor> p =
      boost::asio::detail::make_shared<Executor>(
          BOOST_ASIO_MOVE_CAST(Executor)(other));
    target_ = p->template target<void>();
    new (&object_) boost::asio::detail::shared_ptr<void>(
        BOOST_ASIO_MOVE_CAST(boost::asio::detail::shared_ptr<Executor>)(p));
  }

  any_executor_base(const any_executor_base& other) BOOST_ASIO_NOEXCEPT
    : object_fns_(other.object_fns_),
      target_fns_(other.target_fns_)
  {
    object_fns_->copy(*this, other);
  }

  ~any_executor_base() BOOST_ASIO_NOEXCEPT
  {
    object_fns_->destroy(*this);
  }

  any_executor_base& operator=(
      const any_executor_base& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      object_fns_->destroy(*this);
      object_fns_ = other.object_fns_;
      target_fns_ = other.target_fns_;
      object_fns_->copy(*this, other);
    }
    return *this;
  }

  any_executor_base& operator=(nullptr_t) BOOST_ASIO_NOEXCEPT
  {
    object_fns_->destroy(*this);
    target_ = 0;
    object_fns_ = object_fns_table<void>();
    target_fns_ = target_fns_table<void>();
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE)

  any_executor_base(any_executor_base&& other) BOOST_ASIO_NOEXCEPT
    : object_fns_(other.object_fns_),
      target_fns_(other.target_fns_)
  {
    other.object_fns_ = object_fns_table<void>();
    other.target_fns_ = target_fns_table<void>();
    object_fns_->move(*this, other);
    other.target_ = 0;
  }

  any_executor_base& operator=(
      any_executor_base&& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      object_fns_->destroy(*this);
      object_fns_ = other.object_fns_;
      other.object_fns_ = object_fns_table<void>();
      target_fns_ = other.target_fns_;
      other.target_fns_ = target_fns_table<void>();
      object_fns_->move(*this, other);
      other.target_ = 0;
    }
    return *this;
  }

#endif // defined(BOOST_ASIO_HAS_MOVE)

  void swap(any_executor_base& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      any_executor_base tmp(BOOST_ASIO_MOVE_CAST(any_executor_base)(other));
      other = BOOST_ASIO_MOVE_CAST(any_executor_base)(*this);
      *this = BOOST_ASIO_MOVE_CAST(any_executor_base)(tmp);
    }
  }

  template <typename F>
  void execute(BOOST_ASIO_MOVE_ARG(F) f) const
  {
    if (target_fns_->blocking_execute != 0)
    {
      boost::asio::detail::non_const_lvalue<F> f2(f);
      target_fns_->blocking_execute(*this, function_view(f2.value));
    }
    else
    {
      target_fns_->execute(*this,
          function(BOOST_ASIO_MOVE_CAST(F)(f), std::allocator<void>()));
    }
  }

  template <typename Executor>
  Executor* target()
  {
    return static_cast<Executor*>(target_);
  }

  template <typename Executor>
  const Executor* target() const
  {
    return static_cast<Executor*>(target_);
  }

#if !defined(BOOST_ASIO_NO_TYPEID)
  const std::type_info& target_type() const
#else // !defined(BOOST_ASIO_NO_TYPEID)
  const void* target_type() const
#endif // !defined(BOOST_ASIO_NO_TYPEID)
  {
    return target_fns_->target_type();
  }

  struct unspecified_bool_type_t {};
  typedef void (*unspecified_bool_type)(unspecified_bool_type_t);
  static void unspecified_bool_true(unspecified_bool_type_t) {}

  operator unspecified_bool_type() const BOOST_ASIO_NOEXCEPT
  {
    return target_ ? &any_executor_base::unspecified_bool_true : 0;
  }

  bool operator!() const BOOST_ASIO_NOEXCEPT
  {
    return target_ == 0;
  }

protected:
  bool equality_helper(const any_executor_base& other) const BOOST_ASIO_NOEXCEPT
  {
    if (target_ == other.target_)
      return true;
    if (target_ && !other.target_)
      return false;
    if (!target_ && other.target_)
      return false;
    if (target_fns_ != other.target_fns_)
      return false;
    return target_fns_->equal(*this, other);
  }

  template <typename Ex>
  Ex& object()
  {
    return *static_cast<Ex*>(static_cast<void*>(&object_));
  }

  template <typename Ex>
  const Ex& object() const
  {
    return *static_cast<const Ex*>(static_cast<const void*>(&object_));
  }

  struct object_fns
  {
    void (*destroy)(any_executor_base&);
    void (*copy)(any_executor_base&, const any_executor_base&);
    void (*move)(any_executor_base&, any_executor_base&);
    const void* (*target)(const any_executor_base&);
  };

  static void destroy_void(any_executor_base&)
  {
  }

  static void copy_void(any_executor_base& ex1, const any_executor_base&)
  {
    ex1.target_ = 0;
  }

  static void move_void(any_executor_base& ex1, any_executor_base&)
  {
    ex1.target_ = 0;
  }

  static const void* target_void(const any_executor_base&)
  {
    return 0;
  }

  template <typename Obj>
  static const object_fns* object_fns_table(
      typename enable_if<
        is_same<Obj, void>::value
      >::type* = 0)
  {
    static const object_fns fns =
    {
      &any_executor_base::destroy_void,
      &any_executor_base::copy_void,
      &any_executor_base::move_void,
      &any_executor_base::target_void
    };
    return &fns;
  }

  static void destroy_shared(any_executor_base& ex)
  {
    typedef boost::asio::detail::shared_ptr<void> type;
    ex.object<type>().~type();
  }

  static void copy_shared(any_executor_base& ex1, const any_executor_base& ex2)
  {
    typedef boost::asio::detail::shared_ptr<void> type;
    new (&ex1.object_) type(ex2.object<type>());
    ex1.target_ = ex2.target_;
  }

  static void move_shared(any_executor_base& ex1, any_executor_base& ex2)
  {
    typedef boost::asio::detail::shared_ptr<void> type;
    new (&ex1.object_) type(BOOST_ASIO_MOVE_CAST(type)(ex2.object<type>()));
    ex1.target_ = ex2.target_;
    ex2.object<type>().~type();
  }

  static const void* target_shared(const any_executor_base& ex)
  {
    typedef boost::asio::detail::shared_ptr<void> type;
    return ex.object<type>().get();
  }

  template <typename Obj>
  static const object_fns* object_fns_table(
      typename enable_if<
        is_same<Obj, boost::asio::detail::shared_ptr<void> >::value
      >::type* = 0)
  {
    static const object_fns fns =
    {
      &any_executor_base::destroy_shared,
      &any_executor_base::copy_shared,
      &any_executor_base::move_shared,
      &any_executor_base::target_shared
    };
    return &fns;
  }

  template <typename Obj>
  static void destroy_object(any_executor_base& ex)
  {
    ex.object<Obj>().~Obj();
  }

  template <typename Obj>
  static void copy_object(any_executor_base& ex1, const any_executor_base& ex2)
  {
    new (&ex1.object_) Obj(ex2.object<Obj>());
    ex1.target_ = &ex1.object<Obj>();
  }

  template <typename Obj>
  static void move_object(any_executor_base& ex1, any_executor_base& ex2)
  {
    new (&ex1.object_) Obj(BOOST_ASIO_MOVE_CAST(Obj)(ex2.object<Obj>()));
    ex1.target_ = &ex1.object<Obj>();
    ex2.object<Obj>().~Obj();
  }

  template <typename Obj>
  static const void* target_object(const any_executor_base& ex)
  {
    return &ex.object<Obj>();
  }

  template <typename Obj>
  static const object_fns* object_fns_table(
      typename enable_if<
        !is_same<Obj, void>::value
          && !is_same<Obj, boost::asio::detail::shared_ptr<void> >::value
      >::type* = 0)
  {
    static const object_fns fns =
    {
      &any_executor_base::destroy_object<Obj>,
      &any_executor_base::copy_object<Obj>,
      &any_executor_base::move_object<Obj>,
      &any_executor_base::target_object<Obj>
    };
    return &fns;
  }

  typedef boost::asio::detail::executor_function function;
  typedef boost::asio::detail::executor_function_view function_view;

  struct target_fns
  {
#if !defined(BOOST_ASIO_NO_TYPEID)
    const std::type_info& (*target_type)();
#else // !defined(BOOST_ASIO_NO_TYPEID)
    const void* (*target_type)();
#endif // !defined(BOOST_ASIO_NO_TYPEID)
    bool (*equal)(const any_executor_base&, const any_executor_base&);
    void (*execute)(const any_executor_base&, BOOST_ASIO_MOVE_ARG(function));
    void (*blocking_execute)(const any_executor_base&, function_view);
  };

#if !defined(BOOST_ASIO_NO_TYPEID)
  static const std::type_info& target_type_void()
  {
    return typeid(void);
  }
#else // !defined(BOOST_ASIO_NO_TYPEID)
  static const void* target_type_void()
  {
    return 0;
  }
#endif // !defined(BOOST_ASIO_NO_TYPEID)

  static bool equal_void(const any_executor_base&, const any_executor_base&)
  {
    return true;
  }

  static void execute_void(const any_executor_base&,
      BOOST_ASIO_MOVE_ARG(function))
  {
    bad_executor ex;
    boost::asio::detail::throw_exception(ex);
  }

  static void blocking_execute_void(const any_executor_base&, function_view)
  {
    bad_executor ex;
    boost::asio::detail::throw_exception(ex);
  }

  template <typename Ex>
  static const target_fns* target_fns_table(
      typename enable_if<
        is_same<Ex, void>::value
      >::type* = 0)
  {
    static const target_fns fns =
    {
      &any_executor_base::target_type_void,
      &any_executor_base::equal_void,
      &any_executor_base::execute_void,
      &any_executor_base::blocking_execute_void
    };
    return &fns;
  }

#if !defined(BOOST_ASIO_NO_TYPEID)
  template <typename Ex>
  static const std::type_info& target_type_ex()
  {
    return typeid(Ex);
  }
#else // !defined(BOOST_ASIO_NO_TYPEID)
  template <typename Ex>
  static const void* target_type_ex()
  {
    static int unique_id;
    return &unique_id;
  }
#endif // !defined(BOOST_ASIO_NO_TYPEID)

  template <typename Ex>
  static bool equal_ex(const any_executor_base& ex1,
      const any_executor_base& ex2)
  {
    return *ex1.target<Ex>() == *ex2.target<Ex>();
  }

  template <typename Ex>
  static void execute_ex(const any_executor_base& ex,
      BOOST_ASIO_MOVE_ARG(function) f)
  {
    execution::execute(*ex.target<Ex>(), BOOST_ASIO_MOVE_CAST(function)(f));
  }

  template <typename Ex>
  static void blocking_execute_ex(const any_executor_base& ex, function_view f)
  {
    execution::execute(*ex.target<Ex>(), f);
  }

  template <typename Ex>
  static const target_fns* target_fns_table(bool is_always_blocking,
      typename enable_if<
        !is_same<Ex, void>::value
      >::type* = 0)
  {
    static const target_fns fns_with_execute =
    {
      &any_executor_base::target_type_ex<Ex>,
      &any_executor_base::equal_ex<Ex>,
      &any_executor_base::execute_ex<Ex>,
      0
    };

    static const target_fns fns_with_blocking_execute =
    {
      &any_executor_base::target_type_ex<Ex>,
      &any_executor_base::equal_ex<Ex>,
      0,
      &any_executor_base::blocking_execute_ex<Ex>
    };

    return is_always_blocking ? &fns_with_blocking_execute : &fns_with_execute;
  }

#if defined(BOOST_ASIO_MSVC)
# pragma warning (push)
# pragma warning (disable:4702)
#endif // defined(BOOST_ASIO_MSVC)

  static void query_fn_void(void*, const void*, const void*)
  {
    bad_executor ex;
    boost::asio::detail::throw_exception(ex);
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void*, const void* ex, const void* prop,
      typename enable_if<
        boost::asio::can_query<const Ex&, const Prop&>::value
          && is_same<typename Prop::polymorphic_query_result_type, void>::value
      >::type*)
  {
    boost::asio::query(*static_cast<const Ex*>(ex),
        *static_cast<const Prop*>(prop));
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void*, const void*, const void*,
      typename enable_if<
        !boost::asio::can_query<const Ex&, const Prop&>::value
          && is_same<typename Prop::polymorphic_query_result_type, void>::value
      >::type*)
  {
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void* result, const void* ex, const void* prop,
      typename enable_if<
        boost::asio::can_query<const Ex&, const Prop&>::value
          && !is_same<typename Prop::polymorphic_query_result_type, void>::value
          && is_reference<typename Prop::polymorphic_query_result_type>::value
      >::type*)
  {
    *static_cast<typename remove_reference<
      typename Prop::polymorphic_query_result_type>::type**>(result)
        = &static_cast<typename Prop::polymorphic_query_result_type>(
            boost::asio::query(*static_cast<const Ex*>(ex),
              *static_cast<const Prop*>(prop)));
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void*, const void*, const void*,
      typename enable_if<
        !boost::asio::can_query<const Ex&, const Prop&>::value
          && !is_same<typename Prop::polymorphic_query_result_type, void>::value
          && is_reference<typename Prop::polymorphic_query_result_type>::value
      >::type*)
  {
    std::terminate(); // Combination should not be possible.
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void* result, const void* ex, const void* prop,
      typename enable_if<
        boost::asio::can_query<const Ex&, const Prop&>::value
          && !is_same<typename Prop::polymorphic_query_result_type, void>::value
          && is_scalar<typename Prop::polymorphic_query_result_type>::value
      >::type*)
  {
    *static_cast<typename Prop::polymorphic_query_result_type*>(result)
      = static_cast<typename Prop::polymorphic_query_result_type>(
          boost::asio::query(*static_cast<const Ex*>(ex),
            *static_cast<const Prop*>(prop)));
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void* result, const void*, const void*,
      typename enable_if<
        !boost::asio::can_query<const Ex&, const Prop&>::value
          && !is_same<typename Prop::polymorphic_query_result_type, void>::value
          && is_scalar<typename Prop::polymorphic_query_result_type>::value
      >::type*)
  {
    *static_cast<typename Prop::polymorphic_query_result_type*>(result)
      = typename Prop::polymorphic_query_result_type();
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void* result, const void* ex, const void* prop,
      typename enable_if<
        boost::asio::can_query<const Ex&, const Prop&>::value
          && !is_same<typename Prop::polymorphic_query_result_type, void>::value
          && !is_reference<typename Prop::polymorphic_query_result_type>::value
          && !is_scalar<typename Prop::polymorphic_query_result_type>::value
      >::type*)
  {
    *static_cast<typename Prop::polymorphic_query_result_type**>(result)
      = new typename Prop::polymorphic_query_result_type(
          boost::asio::query(*static_cast<const Ex*>(ex),
            *static_cast<const Prop*>(prop)));
  }

  template <typename Ex, class Prop>
  static void query_fn_non_void(void* result, const void*, const void*, ...)
  {
    *static_cast<typename Prop::polymorphic_query_result_type**>(result)
      = new typename Prop::polymorphic_query_result_type();
  }

  template <typename Ex, class Prop>
  static void query_fn_impl(void* result, const void* ex, const void* prop,
      typename enable_if<
        is_same<Ex, void>::value
      >::type*)
  {
    query_fn_void(result, ex, prop);
  }

  template <typename Ex, class Prop>
  static void query_fn_impl(void* result, const void* ex, const void* prop,
      typename enable_if<
        !is_same<Ex, void>::value
      >::type*)
  {
    query_fn_non_void<Ex, Prop>(result, ex, prop, 0);
  }

  template <typename Ex, class Prop>
  static void query_fn(void* result, const void* ex, const void* prop)
  {
    query_fn_impl<Ex, Prop>(result, ex, prop, 0);
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly require_fn_impl(const void*, const void*,
      typename enable_if<
        is_same<Ex, void>::value
      >::type*)
  {
    bad_executor ex;
    boost::asio::detail::throw_exception(ex);
    return Poly();
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly require_fn_impl(const void* ex, const void* prop,
      typename enable_if<
        !is_same<Ex, void>::value && Prop::is_requirable
      >::type*)
  {
    return boost::asio::require(*static_cast<const Ex*>(ex),
        *static_cast<const Prop*>(prop));
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly require_fn_impl(const void*, const void*, ...)
  {
    return Poly();
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly require_fn(const void* ex, const void* prop)
  {
    return require_fn_impl<Poly, Ex, Prop>(ex, prop, 0);
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly prefer_fn_impl(const void*, const void*,
      typename enable_if<
        is_same<Ex, void>::value
      >::type*)
  {
    bad_executor ex;
    boost::asio::detail::throw_exception(ex);
    return Poly();
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly prefer_fn_impl(const void* ex, const void* prop,
      typename enable_if<
        !is_same<Ex, void>::value && Prop::is_preferable
      >::type*)
  {
    return boost::asio::prefer(*static_cast<const Ex*>(ex),
        *static_cast<const Prop*>(prop));
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly prefer_fn_impl(const void*, const void*, ...)
  {
    return Poly();
  }

  template <typename Poly, typename Ex, class Prop>
  static Poly prefer_fn(const void* ex, const void* prop)
  {
    return prefer_fn_impl<Poly, Ex, Prop>(ex, prop, 0);
  }

  template <typename Poly>
  struct prop_fns
  {
    void (*query)(void*, const void*, const void*);
    Poly (*require)(const void*, const void*);
    Poly (*prefer)(const void*, const void*);
  };

#if defined(BOOST_ASIO_MSVC)
# pragma warning (pop)
#endif // defined(BOOST_ASIO_MSVC)

private:
  template <typename Executor>
  static execution::blocking_t query_blocking(const Executor& ex, true_type)
  {
    return boost::asio::query(ex, execution::blocking);
  }

  template <typename Executor>
  static execution::blocking_t query_blocking(const Executor&, false_type)
  {
    return execution::blocking_t();
  }

  template <typename Executor>
  void construct_object(Executor& ex, true_type)
  {
    object_fns_ = object_fns_table<Executor>();
    target_ = new (&object_) Executor(BOOST_ASIO_MOVE_CAST(Executor)(ex));
  }

  template <typename Executor>
  void construct_object(Executor& ex, false_type)
  {
    object_fns_ = object_fns_table<boost::asio::detail::shared_ptr<void> >();
    boost::asio::detail::shared_ptr<Executor> p =
      boost::asio::detail::make_shared<Executor>(
          BOOST_ASIO_MOVE_CAST(Executor)(ex));
    target_ = p.get();
    new (&object_) boost::asio::detail::shared_ptr<void>(
        BOOST_ASIO_MOVE_CAST(boost::asio::detail::shared_ptr<Executor>)(p));
  }

/*private:*/public:
//  template <typename...> friend class any_executor;

  typedef aligned_storage<
      sizeof(boost::asio::detail::shared_ptr<void>),
      alignment_of<boost::asio::detail::shared_ptr<void> >::value
    >::type object_type;

  object_type object_;
  const object_fns* object_fns_;
  void* target_;
  const target_fns* target_fns_;
};

template <typename Derived, typename Property, typename = void>
struct any_executor_context
{
};

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)

template <typename Derived, typename Property>
struct any_executor_context<Derived, Property,
    typename enable_if<Property::value>::type>
{
  typename Property::query_result_type context() const
  {
    return static_cast<const Derived*>(this)->query(typename Property::type());
  }
};

#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

} // namespace detail

template <>
class any_executor<> : public detail::any_executor_base
{
public:
  any_executor() BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base()
  {
  }

  any_executor(nullptr_t) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base()
  {
  }

  template <typename Executor>
  any_executor(Executor ex,
      typename enable_if<
        conditional<
          !is_same<Executor, any_executor>::value
            && !is_base_of<detail::any_executor_base, Executor>::value,
          is_executor<Executor>,
          false_type
        >::type::value
      >::type* = 0)
    : detail::any_executor_base(
        BOOST_ASIO_MOVE_CAST(Executor)(ex), false_type())
  {
  }

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename... OtherSupportableProperties>
  any_executor(any_executor<OtherSupportableProperties...> other)
    : detail::any_executor_base(
        static_cast<const detail::any_executor_base&>(other))
  {
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename U0, typename U1, typename U2, typename U3,
      typename U4, typename U5, typename U6, typename U7>
  any_executor(any_executor<U0, U1, U2, U3, U4, U5, U6, U7> other)
    : detail::any_executor_base(
        static_cast<const detail::any_executor_base&>(other))
  {
  }

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  any_executor(const any_executor& other) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(
        static_cast<const detail::any_executor_base&>(other))
  {
  }

  any_executor& operator=(const any_executor& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      detail::any_executor_base::operator=(
          static_cast<const detail::any_executor_base&>(other));
    }
    return *this;
  }

  any_executor& operator=(nullptr_t p) BOOST_ASIO_NOEXCEPT
  {
    detail::any_executor_base::operator=(p);
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE)

  any_executor(any_executor&& other) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(
        static_cast<any_executor_base&&>(
          static_cast<any_executor_base&>(other)))
  {
  }

  any_executor& operator=(any_executor&& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      detail::any_executor_base::operator=(
          static_cast<detail::any_executor_base&&>(
            static_cast<detail::any_executor_base&>(other)));
    }
    return *this;
  }

#endif // defined(BOOST_ASIO_HAS_MOVE)

  void swap(any_executor& other) BOOST_ASIO_NOEXCEPT
  {
    detail::any_executor_base::swap(
        static_cast<detail::any_executor_base&>(other));
  }

  using detail::any_executor_base::execute;
  using detail::any_executor_base::target;
  using detail::any_executor_base::target_type;
  using detail::any_executor_base::operator unspecified_bool_type;
  using detail::any_executor_base::operator!;

  bool equality_helper(const any_executor& other) const BOOST_ASIO_NOEXCEPT
  {
    return any_executor_base::equality_helper(other);
  }

  template <typename AnyExecutor1, typename AnyExecutor2>
  friend typename enable_if<
    is_base_of<any_executor, AnyExecutor1>::value
      || is_base_of<any_executor, AnyExecutor2>::value,
    bool
  >::type operator==(const AnyExecutor1& a,
      const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT
  {
    return static_cast<const any_executor&>(a).equality_helper(b);
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator==(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT
  {
    return !a;
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator==(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT
  {
    return !b;
  }

  template <typename AnyExecutor1, typename AnyExecutor2>
  friend typename enable_if<
    is_base_of<any_executor, AnyExecutor1>::value
      || is_base_of<any_executor, AnyExecutor2>::value,
    bool
  >::type operator!=(const AnyExecutor1& a,
      const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT
  {
    return !static_cast<const any_executor&>(a).equality_helper(b);
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator!=(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT
  {
    return !!a;
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator!=(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT
  {
    return !!b;
  }
};

inline void swap(any_executor<>& a, any_executor<>& b) BOOST_ASIO_NOEXCEPT
{
  return a.swap(b);
}

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... SupportableProperties>
class any_executor :
  public detail::any_executor_base,
  public detail::any_executor_context<
    any_executor<SupportableProperties...>,
      typename detail::supportable_properties<
        0, void(SupportableProperties...)>::find_context_as_property>
{
public:
  any_executor() BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(),
      prop_fns_(prop_fns_table<void>())
  {
  }

  any_executor(nullptr_t) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(),
      prop_fns_(prop_fns_table<void>())
  {
  }

  template <typename Executor>
  any_executor(Executor ex,
      typename enable_if<
        conditional<
          !is_same<Executor, any_executor>::value
            && !is_base_of<detail::any_executor_base, Executor>::value,
          detail::is_valid_target_executor<
            Executor, void(SupportableProperties...)>,
          false_type
        >::type::value
      >::type* = 0)
    : detail::any_executor_base(
        BOOST_ASIO_MOVE_CAST(Executor)(ex), false_type()),
      prop_fns_(prop_fns_table<Executor>())
  {
  }

  template <typename... OtherSupportableProperties>
  any_executor(any_executor<OtherSupportableProperties...> other,
      typename enable_if<
        conditional<
          !is_same<
            any_executor<OtherSupportableProperties...>,
            any_executor
          >::value,
          typename detail::supportable_properties<
            0, void(SupportableProperties...)>::template is_valid_target<
              any_executor<OtherSupportableProperties...> >,
          false_type
        >::type::value
      >::type* = 0)
    : detail::any_executor_base(BOOST_ASIO_MOVE_CAST(
          any_executor<OtherSupportableProperties...>)(other), true_type()),
      prop_fns_(prop_fns_table<any_executor<OtherSupportableProperties...> >())
  {
  }

  any_executor(const any_executor& other) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(
        static_cast<const detail::any_executor_base&>(other)),
      prop_fns_(other.prop_fns_)
  {
  }

  any_executor& operator=(const any_executor& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      prop_fns_ = other.prop_fns_;
      detail::any_executor_base::operator=(
          static_cast<const detail::any_executor_base&>(other));
    }
    return *this;
  }

  any_executor& operator=(nullptr_t p) BOOST_ASIO_NOEXCEPT
  {
    prop_fns_ = prop_fns_table<void>();
    detail::any_executor_base::operator=(p);
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE)

  any_executor(any_executor&& other) BOOST_ASIO_NOEXCEPT
    : detail::any_executor_base(
        static_cast<any_executor_base&&>(
          static_cast<any_executor_base&>(other))),
      prop_fns_(other.prop_fns_)
  {
    other.prop_fns_ = prop_fns_table<void>();
  }

  any_executor& operator=(any_executor&& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      prop_fns_ = other.prop_fns_;
      detail::any_executor_base::operator=(
          static_cast<detail::any_executor_base&&>(
            static_cast<detail::any_executor_base&>(other)));
    }
    return *this;
  }

#endif // defined(BOOST_ASIO_HAS_MOVE)

  void swap(any_executor& other) BOOST_ASIO_NOEXCEPT
  {
    if (this != &other)
    {
      detail::any_executor_base::swap(
          static_cast<detail::any_executor_base&>(other));
      const prop_fns<any_executor>* tmp_prop_fns = other.prop_fns_;
      other.prop_fns_ = prop_fns_;
      prop_fns_ = tmp_prop_fns;
    }
  }

  using detail::any_executor_base::execute;
  using detail::any_executor_base::target;
  using detail::any_executor_base::target_type;
  using detail::any_executor_base::operator unspecified_bool_type;
  using detail::any_executor_base::operator!;

  bool equality_helper(const any_executor& other) const BOOST_ASIO_NOEXCEPT
  {
    return any_executor_base::equality_helper(other);
  }

  template <typename AnyExecutor1, typename AnyExecutor2>
  friend typename enable_if<
    is_base_of<any_executor, AnyExecutor1>::value
      || is_base_of<any_executor, AnyExecutor2>::value,
    bool
  >::type operator==(const AnyExecutor1& a,
      const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT
  {
    return static_cast<const any_executor&>(a).equality_helper(b);
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator==(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT
  {
    return !a;
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator==(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT
  {
    return !b;
  }

  template <typename AnyExecutor1, typename AnyExecutor2>
  friend typename enable_if<
    is_base_of<any_executor, AnyExecutor1>::value
      || is_base_of<any_executor, AnyExecutor2>::value,
    bool
  >::type operator!=(const AnyExecutor1& a,
      const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT
  {
    return !static_cast<const any_executor&>(a).equality_helper(b);
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator!=(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT
  {
    return !!a;
  }

  template <typename AnyExecutor>
  friend typename enable_if<
    is_same<AnyExecutor, any_executor>::value,
    bool
  >::type operator!=(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT
  {
    return !!b;
  }

  template <typename T>
  struct find_convertible_property :
      detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_property<T> {};

  template <typename Property>
  void query(const Property& p,
      typename enable_if<
        is_same<
          typename find_convertible_property<Property>::query_result_type,
          void
        >::value
      >::type* = 0) const
  {
    typedef find_convertible_property<Property> found;
    prop_fns_[found::index].query(0, object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
  }

  template <typename Property>
  typename find_convertible_property<Property>::query_result_type
  query(const Property& p,
      typename enable_if<
        !is_same<
          typename find_convertible_property<Property>::query_result_type,
          void
        >::value
        &&
        is_reference<
          typename find_convertible_property<Property>::query_result_type
        >::value
      >::type* = 0) const
  {
    typedef find_convertible_property<Property> found;
    typename remove_reference<
      typename found::query_result_type>::type* result = 0;
    prop_fns_[found::index].query(&result, object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
    return *result;
  }

  template <typename Property>
  typename find_convertible_property<Property>::query_result_type
  query(const Property& p,
      typename enable_if<
        !is_same<
          typename find_convertible_property<Property>::query_result_type,
          void
        >::value
        &&
        is_scalar<
          typename find_convertible_property<Property>::query_result_type
        >::value
      >::type* = 0) const
  {
    typedef find_convertible_property<Property> found;
    typename found::query_result_type result;
    prop_fns_[found::index].query(&result, object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
    return result;
  }

  template <typename Property>
  typename find_convertible_property<Property>::query_result_type
  query(const Property& p,
      typename enable_if<
        !is_same<
          typename find_convertible_property<Property>::query_result_type,
          void
        >::value
        &&
        !is_reference<
          typename find_convertible_property<Property>::query_result_type
        >::value
        &&
        !is_scalar<
          typename find_convertible_property<Property>::query_result_type
        >::value
      >::type* = 0) const
  {
    typedef find_convertible_property<Property> found;
    typename found::query_result_type* result;
    prop_fns_[found::index].query(&result, object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
    return *boost::asio::detail::scoped_ptr<
      typename found::query_result_type>(result);
  }

  template <typename T>
  struct find_convertible_requirable_property :
      detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_requirable_property<T> {};

  template <typename Property>
  any_executor require(const Property& p,
      typename enable_if<
        find_convertible_requirable_property<Property>::value
      >::type* = 0) const
  {
    typedef find_convertible_requirable_property<Property> found;
    return prop_fns_[found::index].require(object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
  }

  template <typename T>
  struct find_convertible_preferable_property :
      detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_preferable_property<T> {};

  template <typename Property>
  any_executor prefer(const Property& p,
      typename enable_if<
        find_convertible_preferable_property<Property>::value
      >::type* = 0) const
  {
    typedef find_convertible_preferable_property<Property> found;
    return prop_fns_[found::index].prefer(object_fns_->target(*this),
        &static_cast<const typename found::type&>(p));
  }

//private:
  template <typename Ex>
  static const prop_fns<any_executor>* prop_fns_table()
  {
    static const prop_fns<any_executor> fns[] =
    {
      {
        &detail::any_executor_base::query_fn<
            Ex, SupportableProperties>,
        &detail::any_executor_base::require_fn<
            any_executor, Ex, SupportableProperties>,
        &detail::any_executor_base::prefer_fn<
            any_executor, Ex, SupportableProperties>
      }...
    };
    return fns;
  }

  const prop_fns<any_executor>* prop_fns_;
};

template <typename... SupportableProperties>
inline void swap(any_executor<SupportableProperties...>& a,
    any_executor<SupportableProperties...>& b) BOOST_ASIO_NOEXCEPT
{
  return a.swap(b);
}

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS(n) \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_##n

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_1 \
  {  \
    &detail::any_executor_base::query_fn<Ex, T1>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T1>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T1> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_2 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_1, \
  { \
    &detail::any_executor_base::query_fn<Ex, T2>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T2>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T2> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_3 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_2, \
  { \
    &detail::any_executor_base::query_fn<Ex, T3>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T3>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T3> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_4 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_3, \
  { \
    &detail::any_executor_base::query_fn<Ex, T4>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T4>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T4> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_5 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_4, \
  { \
    &detail::any_executor_base::query_fn<Ex, T5>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T5>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T5> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_6 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_5, \
  { \
    &detail::any_executor_base::query_fn<Ex, T6>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T6>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T6> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_7 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_6, \
  { \
    &detail::any_executor_base::query_fn<Ex, T7>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T7>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T7> \
  }
#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_8 \
  BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_7, \
  { \
    &detail::any_executor_base::query_fn<Ex, T8>, \
    &detail::any_executor_base::require_fn<any_executor, Ex, T8>, \
    &detail::any_executor_base::prefer_fn<any_executor, Ex, T8> \
  }

#if defined(BOOST_ASIO_HAS_MOVE)

# define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_MOVE_OPS \
  any_executor(any_executor&& other) BOOST_ASIO_NOEXCEPT \
    : detail::any_executor_base( \
        static_cast<any_executor_base&&>( \
          static_cast<any_executor_base&>(other))), \
      prop_fns_(other.prop_fns_) \
  { \
    other.prop_fns_ = prop_fns_table<void>(); \
  } \
  \
  any_executor& operator=(any_executor&& other) BOOST_ASIO_NOEXCEPT \
  { \
    if (this != &other) \
    { \
      prop_fns_ = other.prop_fns_; \
      detail::any_executor_base::operator=( \
          static_cast<detail::any_executor_base&&>( \
            static_cast<detail::any_executor_base&>(other))); \
    } \
    return *this; \
  } \
  /**/
#else // defined(BOOST_ASIO_HAS_MOVE)

# define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_MOVE_OPS

#endif // defined(BOOST_ASIO_HAS_MOVE)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_DEF(n) \
  template <BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  class any_executor<BOOST_ASIO_VARIADIC_TARGS(n)> : \
    public detail::any_executor_base, \
    public detail::any_executor_context< \
      any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>, \
        typename detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::find_context_as_property> \
  { \
  public: \
    any_executor() BOOST_ASIO_NOEXCEPT \
      : detail::any_executor_base(), \
        prop_fns_(prop_fns_table<void>()) \
    { \
    } \
    \
    any_executor(nullptr_t) BOOST_ASIO_NOEXCEPT \
      : detail::any_executor_base(), \
        prop_fns_(prop_fns_table<void>()) \
    { \
    } \
    \
    template <BOOST_ASIO_EXECUTION_EXECUTOR Executor> \
    any_executor(Executor ex, \
        typename enable_if< \
          conditional< \
            !is_same<Executor, any_executor>::value \
              && !is_base_of<detail::any_executor_base, Executor>::value, \
            detail::is_valid_target_executor< \
              Executor, void(BOOST_ASIO_VARIADIC_TARGS(n))>, \
            false_type \
          >::type::value \
        >::type* = 0) \
      : detail::any_executor_base(BOOST_ASIO_MOVE_CAST( \
            Executor)(ex), false_type()), \
        prop_fns_(prop_fns_table<Executor>()) \
    { \
    } \
    \
    any_executor(const any_executor& other) BOOST_ASIO_NOEXCEPT \
      : detail::any_executor_base( \
          static_cast<const detail::any_executor_base&>(other)), \
        prop_fns_(other.prop_fns_) \
    { \
    } \
    \
    any_executor(any_executor<> other) \
      : detail::any_executor_base(BOOST_ASIO_MOVE_CAST( \
            any_executor<>)(other), true_type()), \
        prop_fns_(prop_fns_table<any_executor<> >()) \
    { \
    } \
    \
    template <typename OtherAnyExecutor> \
    any_executor(OtherAnyExecutor other, \
        typename enable_if< \
          conditional< \
            !is_same<OtherAnyExecutor, any_executor>::value \
              && is_base_of<detail::any_executor_base, \
                OtherAnyExecutor>::value, \
            typename detail::supportable_properties< \
              0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
                is_valid_target<OtherAnyExecutor>, \
            false_type \
          >::type::value \
        >::type* = 0) \
      : detail::any_executor_base(BOOST_ASIO_MOVE_CAST( \
            OtherAnyExecutor)(other), true_type()), \
        prop_fns_(prop_fns_table<OtherAnyExecutor>()) \
    { \
    } \
    \
    any_executor& operator=(const any_executor& other) BOOST_ASIO_NOEXCEPT \
    { \
      if (this != &other) \
      { \
        prop_fns_ = other.prop_fns_; \
        detail::any_executor_base::operator=( \
            static_cast<const detail::any_executor_base&>(other)); \
      } \
      return *this; \
    } \
    \
    any_executor& operator=(nullptr_t p) BOOST_ASIO_NOEXCEPT \
    { \
      prop_fns_ = prop_fns_table<void>(); \
      detail::any_executor_base::operator=(p); \
      return *this; \
    } \
    \
    BOOST_ASIO_PRIVATE_ANY_EXECUTOR_MOVE_OPS \
    \
    void swap(any_executor& other) BOOST_ASIO_NOEXCEPT \
    { \
      if (this != &other) \
      { \
        detail::any_executor_base::swap( \
            static_cast<detail::any_executor_base&>(other)); \
        const prop_fns<any_executor>* tmp_prop_fns = other.prop_fns_; \
        other.prop_fns_ = prop_fns_; \
        prop_fns_ = tmp_prop_fns; \
      } \
    } \
    \
    using detail::any_executor_base::execute; \
    using detail::any_executor_base::target; \
    using detail::any_executor_base::target_type; \
    using detail::any_executor_base::operator unspecified_bool_type; \
    using detail::any_executor_base::operator!; \
    \
    bool equality_helper(const any_executor& other) const BOOST_ASIO_NOEXCEPT \
    { \
      return any_executor_base::equality_helper(other); \
    } \
    \
    template <typename AnyExecutor1, typename AnyExecutor2> \
    friend typename enable_if< \
      is_base_of<any_executor, AnyExecutor1>::value \
        || is_base_of<any_executor, AnyExecutor2>::value, \
      bool \
    >::type operator==(const AnyExecutor1& a, \
        const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT \
    { \
      return static_cast<const any_executor&>(a).equality_helper(b); \
    } \
    \
    template <typename AnyExecutor> \
    friend typename enable_if< \
      is_same<AnyExecutor, any_executor>::value, \
      bool \
    >::type operator==(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT \
    { \
      return !a; \
    } \
    \
    template <typename AnyExecutor> \
    friend typename enable_if< \
      is_same<AnyExecutor, any_executor>::value, \
      bool \
    >::type operator==(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT \
    { \
      return !b; \
    } \
    \
    template <typename AnyExecutor1, typename AnyExecutor2> \
    friend typename enable_if< \
      is_base_of<any_executor, AnyExecutor1>::value \
        || is_base_of<any_executor, AnyExecutor2>::value, \
      bool \
    >::type operator!=(const AnyExecutor1& a, \
        const AnyExecutor2& b) BOOST_ASIO_NOEXCEPT \
    { \
      return !static_cast<const any_executor&>(a).equality_helper(b); \
    } \
    \
    template <typename AnyExecutor> \
    friend typename enable_if< \
      is_same<AnyExecutor, any_executor>::value, \
      bool \
    >::type operator!=(const AnyExecutor& a, nullptr_t) BOOST_ASIO_NOEXCEPT \
    { \
      return !!a; \
    } \
    \
    template <typename AnyExecutor> \
    friend typename enable_if< \
      is_same<AnyExecutor, any_executor>::value, \
      bool \
    >::type operator!=(nullptr_t, const AnyExecutor& b) BOOST_ASIO_NOEXCEPT \
    { \
      return !!b; \
    } \
    \
    template <typename T> \
    struct find_convertible_property : \
        detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_property<T> {}; \
    \
    template <typename Property> \
    void query(const Property& p, \
        typename enable_if< \
          is_same< \
            typename find_convertible_property<Property>::query_result_type, \
            void \
          >::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_property<Property> found; \
      prop_fns_[found::index].query(0, object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
    } \
    \
    template <typename Property> \
    typename find_convertible_property<Property>::query_result_type \
    query(const Property& p, \
        typename enable_if< \
          !is_same< \
            typename find_convertible_property<Property>::query_result_type, \
            void \
          >::value \
          && \
          is_reference< \
            typename find_convertible_property<Property>::query_result_type \
          >::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_property<Property> found; \
      typename remove_reference< \
        typename found::query_result_type>::type* result; \
      prop_fns_[found::index].query(&result, object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
      return *result; \
    } \
    \
    template <typename Property> \
    typename find_convertible_property<Property>::query_result_type \
    query(const Property& p, \
        typename enable_if< \
          !is_same< \
            typename find_convertible_property<Property>::query_result_type, \
            void \
          >::value \
          && \
          is_scalar< \
            typename find_convertible_property<Property>::query_result_type \
          >::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_property<Property> found; \
      typename found::query_result_type result; \
      prop_fns_[found::index].query(&result, object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
      return result; \
    } \
    \
    template <typename Property> \
    typename find_convertible_property<Property>::query_result_type \
    query(const Property& p, \
        typename enable_if< \
          !is_same< \
            typename find_convertible_property<Property>::query_result_type, \
            void \
          >::value \
          && \
          !is_reference< \
            typename find_convertible_property<Property>::query_result_type \
          >::value \
          && \
          !is_scalar< \
            typename find_convertible_property<Property>::query_result_type \
          >::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_property<Property> found; \
      typename found::query_result_type* result; \
      prop_fns_[found::index].query(&result, object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
      return *boost::asio::detail::scoped_ptr< \
        typename found::query_result_type>(result); \
    } \
    \
    template <typename T> \
    struct find_convertible_requirable_property : \
        detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_requirable_property<T> {}; \
    \
    template <typename Property> \
    any_executor require(const Property& p, \
        typename enable_if< \
          find_convertible_requirable_property<Property>::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_requirable_property<Property> found; \
      return prop_fns_[found::index].require(object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
    } \
    \
    template <typename T> \
    struct find_convertible_preferable_property : \
        detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_preferable_property<T> {}; \
    \
    template <typename Property> \
    any_executor prefer(const Property& p, \
        typename enable_if< \
          find_convertible_preferable_property<Property>::value \
        >::type* = 0) const \
    { \
      typedef find_convertible_preferable_property<Property> found; \
      return prop_fns_[found::index].prefer(object_fns_->target(*this), \
          &static_cast<const typename found::type&>(p)); \
    } \
    \
    template <typename Ex> \
    static const prop_fns<any_executor>* prop_fns_table() \
    { \
      static const prop_fns<any_executor> fns[] = \
      { \
        BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS(n) \
      }; \
      return fns; \
    } \
    \
    const prop_fns<any_executor>* prop_fns_; \
    typedef detail::supportable_properties<0, \
        void(BOOST_ASIO_VARIADIC_TARGS(n))> supportable_properties_type; \
  }; \
  \
  template <BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline void swap(any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>& a, \
      any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>& b) BOOST_ASIO_NOEXCEPT \
  { \
    return a.swap(b); \
  } \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_ANY_EXECUTOR_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_DEF
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_MOVE_OPS
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_1
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_2
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_3
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_4
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_5
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_6
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_7
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PROP_FNS_8

#endif // if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace execution
namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename... SupportableProperties>
struct equality_comparable<execution::any_executor<SupportableProperties...> >
{
  static const bool is_valid = true;
  static const bool is_noexcept = true;
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <>
struct equality_comparable<execution::any_executor<> >
{
  static const bool is_valid = true;
  static const bool is_noexcept = true;
};

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EQUALITY_COMPARABLE_DEF(n) \
  template <BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct equality_comparable< \
      execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)> > \
  { \
    static const bool is_valid = true; \
    static const bool is_noexcept = true; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(
      BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EQUALITY_COMPARABLE_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EQUALITY_COMPARABLE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename F, typename... SupportableProperties>
struct execute_member<execution::any_executor<SupportableProperties...>, F>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef void result_type;
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename F>
struct execute_member<execution::any_executor<>, F>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef void result_type;
};

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EXECUTE_MEMBER_DEF(n) \
  template <typename F, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct execute_member< \
      execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>, F> \
  { \
    static const bool is_valid = true; \
    static const bool is_noexcept = false; \
    typedef void result_type; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(
      BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EXECUTE_MEMBER_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_EXECUTE_MEMBER_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename Prop, typename... SupportableProperties>
struct query_member<
    execution::any_executor<SupportableProperties...>, Prop,
    typename enable_if<
      execution::detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_property<Prop>::value
    >::type>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef typename execution::detail::supportable_properties<
      0, void(SupportableProperties...)>::template
        find_convertible_property<Prop>::query_result_type result_type;
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_QUERY_MEMBER_DEF(n) \
  template <typename Prop, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct query_member< \
      execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>, Prop, \
      typename enable_if< \
        execution::detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_property<Prop>::value \
    >::type> \
  { \
    static const bool is_valid = true; \
    static const bool is_noexcept = false; \
    typedef typename execution::detail::supportable_properties< \
        0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
          find_convertible_property<Prop>::query_result_type result_type; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_ANY_EXECUTOR_QUERY_MEMBER_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_QUERY_MEMBER_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename Prop, typename... SupportableProperties>
struct require_member<
    execution::any_executor<SupportableProperties...>, Prop,
    typename enable_if<
      execution::detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_requirable_property<Prop>::value
    >::type>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef execution::any_executor<SupportableProperties...> result_type;
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_REQUIRE_MEMBER_DEF(n) \
  template <typename Prop, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct require_member< \
      execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>, Prop, \
      typename enable_if< \
        execution::detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_requirable_property<Prop>::value \
      >::type> \
  { \
    static const bool is_valid = true; \
    static const bool is_noexcept = false; \
    typedef execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)> result_type; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(
      BOOST_ASIO_PRIVATE_ANY_EXECUTOR_REQUIRE_MEMBER_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_REQUIRE_MEMBER_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_FREE_TRAIT)
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename Prop, typename... SupportableProperties>
struct prefer_member<
    execution::any_executor<SupportableProperties...>, Prop,
    typename enable_if<
      execution::detail::supportable_properties<
        0, void(SupportableProperties...)>::template
          find_convertible_preferable_property<Prop>::value
    >::type>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef execution::any_executor<SupportableProperties...> result_type;
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#define BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PREFER_FREE_DEF(n) \
  template <typename Prop, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct prefer_member< \
      execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)>, Prop, \
      typename enable_if< \
        execution::detail::supportable_properties< \
          0, void(BOOST_ASIO_VARIADIC_TARGS(n))>::template \
            find_convertible_preferable_property<Prop>::value \
      >::type> \
  { \
    static const bool is_valid = true; \
    static const bool is_noexcept = false; \
    typedef execution::any_executor<BOOST_ASIO_VARIADIC_TARGS(n)> result_type; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PREFER_FREE_DEF)
#undef BOOST_ASIO_PRIVATE_ANY_EXECUTOR_PREFER_FREE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_FREE_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_ANY_EXECUTOR_HPP
