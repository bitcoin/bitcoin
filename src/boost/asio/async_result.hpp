//
// async_result.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_ASYNC_RESULT_HPP
#define BOOST_ASIO_ASYNC_RESULT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(BOOST_ASIO_HAS_CONCEPTS) \
  && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(BOOST_ASIO_HAS_DECLTYPE)

namespace detail {

template <typename T>
struct is_completion_signature : false_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...)> : true_type
{
};

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) &> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) &&> : true_type
{
};

# if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) noexcept> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) & noexcept> : true_type
{
};

template <typename R, typename... Args>
struct is_completion_signature<R(Args...) && noexcept> : true_type
{
};

# endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename... T>
struct are_completion_signatures : false_type
{
};

template <typename T0>
struct are_completion_signatures<T0>
  : is_completion_signature<T0>
{
};

template <typename T0, typename... TN>
struct are_completion_signatures<T0, TN...>
  : integral_constant<bool, (
      is_completion_signature<T0>::value
        && are_completion_signatures<TN...>::value)>
{
};

template <typename T, typename... Args>
BOOST_ASIO_CONCEPT callable_with = requires(T&& t, Args&&... args)
{
  static_cast<T&&>(t)(static_cast<Args&&>(args)...);
};

template <typename T, typename... Signatures>
struct is_completion_handler_for : false_type
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...)>
  : integral_constant<bool, (callable_with<T, Args...>)>
{
};

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) &>
  : integral_constant<bool, (callable_with<T&, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) &&>
  : integral_constant<bool, (callable_with<T&&, Args...>)>
{
};

# if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) noexcept>
  : integral_constant<bool, (callable_with<T, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) & noexcept>
  : integral_constant<bool, (callable_with<T&, Args...>)>
{
};

template <typename T, typename R, typename... Args>
struct is_completion_handler_for<T, R(Args...) && noexcept>
  : integral_constant<bool, (callable_with<T&&, Args...>)>
{
};

# endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename T, typename Signature0, typename... SignatureN>
struct is_completion_handler_for<T, Signature0, SignatureN...>
  : integral_constant<bool, (
      is_completion_handler_for<T, Signature0>::value
        && is_completion_handler_for<T, SignatureN...>::value)>
{
};

} // namespace detail

template <typename T>
BOOST_ASIO_CONCEPT completion_signature =
  detail::is_completion_signature<T>::value;

#define BOOST_ASIO_COMPLETION_SIGNATURE \
  ::boost::asio::completion_signature

template <typename T, typename... Signatures>
BOOST_ASIO_CONCEPT completion_handler_for =
  detail::are_completion_signatures<Signatures...>::value
    && detail::is_completion_handler_for<T, Signatures...>::value;

#define BOOST_ASIO_COMPLETION_HANDLER_FOR(sig) \
  ::boost::asio::completion_handler_for<sig>
#define BOOST_ASIO_COMPLETION_HANDLER_FOR2(sig0, sig1) \
  ::boost::asio::completion_handler_for<sig0, sig1>
#define BOOST_ASIO_COMPLETION_HANDLER_FOR3(sig0, sig1, sig2) \
  ::boost::asio::completion_handler_for<sig0, sig1, sig2>

#else // defined(BOOST_ASIO_HAS_CONCEPTS)
      //   && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
      //   && defined(BOOST_ASIO_HAS_DECLTYPE)

#define BOOST_ASIO_COMPLETION_SIGNATURE typename
#define BOOST_ASIO_COMPLETION_HANDLER_FOR(sig) typename
#define BOOST_ASIO_COMPLETION_HANDLER_FOR2(sig0, sig1) typename
#define BOOST_ASIO_COMPLETION_HANDLER_FOR3(sig0, sig1, sig2) typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)
       //   && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_DECLTYPE)

namespace detail {

template <typename T>
struct is_simple_completion_signature : false_type
{
};

template <typename T>
struct simple_completion_signature;

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename... Args>
struct is_simple_completion_signature<R(Args...)> : true_type
{
};

template <typename... Signatures>
struct are_simple_completion_signatures : false_type
{
};

template <typename Sig0>
struct are_simple_completion_signatures<Sig0>
  : is_simple_completion_signature<Sig0>
{
};

template <typename Sig0, typename... SigN>
struct are_simple_completion_signatures<Sig0, SigN...>
  : integral_constant<bool, (
      is_simple_completion_signature<Sig0>::value
        && are_simple_completion_signatures<SigN...>::value)>
{
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...)>
{
  typedef R type(Args...);
};

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) &>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) &&>
{
  typedef R type(Args...);
};

# if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) noexcept>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) & noexcept>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct simple_completion_signature<R(Args...) && noexcept>
{
  typedef R type(Args...);
};

# endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R>
struct is_simple_completion_signature<R()> : true_type
{
};

#define BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct is_simple_completion_signature<R(BOOST_ASIO_VARIADIC_TARGS(n))> \
    : true_type \
  { \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF

template <typename Sig0 = void, typename Sig1 = void,
    typename Sig2 = void, typename = void>
struct are_simple_completion_signatures : false_type
{
};

template <typename Sig0>
struct are_simple_completion_signatures<Sig0>
  : is_simple_completion_signature<Sig0>
{
};

template <typename Sig0, typename Sig1>
struct are_simple_completion_signatures<Sig0, Sig1>
  : integral_constant<bool,
      (is_simple_completion_signature<Sig0>::value
        && is_simple_completion_signature<Sig1>::value)>
{
};

template <typename Sig0, typename Sig1, typename Sig2>
struct are_simple_completion_signatures<Sig0, Sig1, Sig2>
  : integral_constant<bool,
      (is_simple_completion_signature<Sig0>::value
        && is_simple_completion_signature<Sig1>::value
        && is_simple_completion_signature<Sig2>::value)>
{
};

template <typename R>
struct simple_completion_signature<R()>
{
  typedef R type();
};

#define BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature<R(BOOST_ASIO_VARIADIC_TARGS(n))> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R>
struct simple_completion_signature<R() &>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() &&>
{
  typedef R type();
};

#define BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(BOOST_ASIO_VARIADIC_TARGS(n)) &> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(BOOST_ASIO_VARIADIC_TARGS(n)) &&> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF

# if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R>
struct simple_completion_signature<R() noexcept>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() & noexcept>
{
  typedef R type();
};

template <typename R>
struct simple_completion_signature<R() && noexcept>
{
  typedef R type();
};

#define BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(BOOST_ASIO_VARIADIC_TARGS(n)) noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(BOOST_ASIO_VARIADIC_TARGS(n)) & noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct simple_completion_signature< \
    R(BOOST_ASIO_VARIADIC_TARGS(n)) && noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF)
#undef BOOST_ASIO_PRIVATE_SIMPLE_SIG_DEF

# endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)

# define BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS \
    BOOST_ASIO_COMPLETION_SIGNATURE... Signatures

# define BOOST_ASIO_COMPLETION_SIGNATURES_TSPECPARAMS \
    BOOST_ASIO_COMPLETION_SIGNATURE... Signatures

# define BOOST_ASIO_COMPLETION_SIGNATURES_TARGS Signatures...

# define BOOST_ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS \
    typename boost::asio::detail::simple_completion_signature< \
      Signatures>::type...

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
      //   || defined(GENERATING_DOCUMENTATION)

# define BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS \
    typename Sig0 = void, \
    typename Sig1 = void, \
    typename Sig2 = void

# define BOOST_ASIO_COMPLETION_SIGNATURES_TSPECPARAMS \
    typename Sig0, \
    typename Sig1, \
    typename Sig2

# define BOOST_ASIO_COMPLETION_SIGNATURES_TARGS Sig0, Sig1, Sig2

# define BOOST_ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS \
    typename ::boost::asio::detail::simple_completion_signature<Sig0>::type, \
    typename ::boost::asio::detail::simple_completion_signature<Sig1>::type, \
    typename ::boost::asio::detail::simple_completion_signature<Sig2>::type

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
class completion_handler_async_result
{
public:
  typedef CompletionToken completion_handler_type;
  typedef void return_type;

  explicit completion_handler_async_result(completion_handler_type&)
  {
  }

  return_type get()
  {
  }

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation,
      BOOST_ASIO_COMPLETION_HANDLER_FOR(Signatures...) RawCompletionToken,
      typename... Args>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token,
      BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
        BOOST_ASIO_MOVE_CAST(RawCompletionToken)(token),
        BOOST_ASIO_MOVE_CAST(Args)(args)...);
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation, typename RawCompletionToken>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token)
  {
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
        BOOST_ASIO_MOVE_CAST(RawCompletionToken)(token));
  }

#define BOOST_ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, \
      typename RawCompletionToken, \
      BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate( \
      BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)( \
        BOOST_ASIO_MOVE_CAST(RawCompletionToken)(token), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_INITIATE_DEF)
#undef BOOST_ASIO_PRIVATE_INITIATE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

private:
  completion_handler_async_result(
      const completion_handler_async_result&) BOOST_ASIO_DELETED;
  completion_handler_async_result& operator=(
      const completion_handler_async_result&) BOOST_ASIO_DELETED;
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)

/// An interface for customising the behaviour of an initiating function.
/**
 * The async_result traits class is used for determining:
 *
 * @li the concrete completion handler type to be called at the end of the
 * asynchronous operation;
 *
 * @li the initiating function return type; and
 *
 * @li how the return value of the initiating function is obtained.
 *
 * The trait allows the handler and return types to be determined at the point
 * where the specific completion handler signature is known.
 *
 * This template may be specialised for user-defined completion token types.
 * The primary template assumes that the CompletionToken is the completion
 * handler.
 */
template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result
{
public:
  /// The concrete completion handler type for the specific signature.
  typedef CompletionToken completion_handler_type;

  /// The return type of the initiating function.
  typedef void return_type;

  /// Construct an async result from a given handler.
  /**
   * When using a specalised async_result, the constructor has an opportunity
   * to initialise some state associated with the completion handler, which is
   * then returned from the initiating function.
   */
  explicit async_result(completion_handler_type& h);

  /// Obtain the value to be returned from the initiating function.
  return_type get();

  /// Initiate the asynchronous operation that will produce the result, and
  /// obtain the value to be returned from the initiating function.
  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token,
      BOOST_ASIO_MOVE_ARG(Args)... args);

private:
  async_result(const async_result&) BOOST_ASIO_DELETED;
  async_result& operator=(const async_result&) BOOST_ASIO_DELETED;
};

#else // defined(GENERATING_DOCUMENTATION)

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result :
  public conditional<
      detail::are_simple_completion_signatures<
        BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>::value,
      detail::completion_handler_async_result<
        CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>,
      async_result<CompletionToken,
        BOOST_ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS>
    >::type
{
public:
  typedef typename conditional<
      detail::are_simple_completion_signatures<
        BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>::value,
      detail::completion_handler_async_result<
        CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>,
      async_result<CompletionToken,
        BOOST_ASIO_COMPLETION_SIGNATURES_TSIMPLEARGS>
    >::type base_type;

  using base_type::base_type;

private:
  async_result(const async_result&) BOOST_ASIO_DELETED;
  async_result& operator=(const async_result&) BOOST_ASIO_DELETED;
};

#else // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
class async_result :
  public detail::completion_handler_async_result<
    CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>
{
public:
  explicit async_result(CompletionToken& h)
    : detail::completion_handler_async_result<
        CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>(h)
  {
  }

private:
  async_result(const async_result&) BOOST_ASIO_DELETED;
  async_result& operator=(const async_result&) BOOST_ASIO_DELETED;
};

#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <BOOST_ASIO_COMPLETION_SIGNATURES_TSPECPARAMS>
class async_result<void, BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>
{
  // Empty.
};

#endif // defined(GENERATING_DOCUMENTATION)

/// Helper template to deduce the handler type from a CompletionToken, capture
/// a local copy of the handler, and then create an async_result for the
/// handler.
template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_completion
{
  /// The real handler type to be used for the asynchronous operation.
  typedef typename boost::asio::async_result<
    typename decay<CompletionToken>::type,
      BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>::completion_handler_type
        completion_handler_type;

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Constructor.
  /**
   * The constructor creates the concrete completion handler and makes the link
   * between the handler and the asynchronous result.
   */
  explicit async_completion(CompletionToken& token)
    : completion_handler(static_cast<typename conditional<
        is_same<CompletionToken, completion_handler_type>::value,
        completion_handler_type&, CompletionToken&&>::type>(token)),
      result(completion_handler)
  {
  }
#else // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  explicit async_completion(typename decay<CompletionToken>::type& token)
    : completion_handler(token),
      result(completion_handler)
  {
  }

  explicit async_completion(const typename decay<CompletionToken>::type& token)
    : completion_handler(token),
      result(completion_handler)
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// A copy of, or reference to, a real handler object.
#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  typename conditional<
    is_same<CompletionToken, completion_handler_type>::value,
    completion_handler_type&, completion_handler_type>::type completion_handler;
#else // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  completion_handler_type completion_handler;
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// The result of the asynchronous operation's initiating function.
  async_result<typename decay<CompletionToken>::type,
    BOOST_ASIO_COMPLETION_SIGNATURES_TARGS> result;
};

namespace detail {

template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_result_helper
  : async_result<typename decay<CompletionToken>::type,
      BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>
{
};

struct async_result_memfns_base
{
  void initiate();
};

template <typename T>
struct async_result_memfns_derived
  : T, async_result_memfns_base
{
};

template <typename T, T>
struct async_result_memfns_check
{
};

template <typename>
char (&async_result_initiate_memfn_helper(...))[2];

template <typename T>
char async_result_initiate_memfn_helper(
    async_result_memfns_check<
      void (async_result_memfns_base::*)(),
      &async_result_memfns_derived<T>::initiate>*);

template <typename CompletionToken, BOOST_ASIO_COMPLETION_SIGNATURES_TPARAMS>
struct async_result_has_initiate_memfn
  : integral_constant<bool, sizeof(async_result_initiate_memfn_helper<
      async_result<typename decay<CompletionToken>::type,
        BOOST_ASIO_COMPLETION_SIGNATURES_TARGS>
    >(0)) != 1>
{
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_INITFN_RESULT_TYPE(ct, sig) \
  void_or_deduced
# define BOOST_ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  void_or_deduced
# define BOOST_ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  void_or_deduced
#elif defined(_MSC_VER) && (_MSC_VER < 1500)
# define BOOST_ASIO_INITFN_RESULT_TYPE(ct, sig) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig>::return_type
# define BOOST_ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig0, sig1>::return_type
# define BOOST_ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig0, sig1, sig2>::return_type
#define BOOST_ASIO_HANDLER_TYPE(ct, sig) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig>::completion_handler_type
#define BOOST_ASIO_HANDLER_TYPE2(ct, sig0, sig1) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig0, sig1>::completion_handler_type
#define BOOST_ASIO_HANDLER_TYPE3(ct, sig0, sig1, sig2) \
  typename ::boost::asio::detail::async_result_helper< \
    ct, sig0, sig1, sig2>::completion_handler_type
#else
# define BOOST_ASIO_INITFN_RESULT_TYPE(ct, sig) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, sig>::return_type
# define BOOST_ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, sig0, sig1>::return_type
# define BOOST_ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, sig0, sig1, sig2>::return_type
#define BOOST_ASIO_HANDLER_TYPE(ct, sig) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, sig>::completion_handler_type
#define BOOST_ASIO_HANDLER_TYPE2(ct, sig0, sig1) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, \
      sig0, sig1>::completion_handler_type
#define BOOST_ASIO_HANDLER_TYPE3(ct, sig0, sig1, sig2) \
  typename ::boost::asio::async_result< \
    typename ::boost::asio::decay<ct>::type, \
      sig0, sig1, sig2>::completion_handler_type
#endif

#if defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  auto
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  auto
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  auto
#elif defined(BOOST_ASIO_HAS_RETURN_TYPE_DEDUCTION)
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  auto
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  auto
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  auto
#else
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ct, sig) \
  BOOST_ASIO_INITFN_RESULT_TYPE(ct, sig)
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE2(ct, sig0, sig1) \
  BOOST_ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1)
# define BOOST_ASIO_INITFN_AUTO_RESULT_TYPE3(ct, sig0, sig1, sig2) \
  BOOST_ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2)
#endif

#if defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  void_or_deduced
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  void_or_deduced
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  void_or_deduced
#elif defined(BOOST_ASIO_HAS_DECLTYPE)
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  decltype expr
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  decltype expr
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  decltype expr
#else
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(ct, sig, expr) \
  BOOST_ASIO_INITFN_RESULT_TYPE(ct, sig)
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE2(ct, sig0, sig1, expr) \
  BOOST_ASIO_INITFN_RESULT_TYPE2(ct, sig0, sig1)
# define BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE3(ct, sig0, sig1, sig2, expr) \
  BOOST_ASIO_INITFN_RESULT_TYPE3(ct, sig0, sig1, sig2)
#endif

#if defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken,
    completion_signature... Signatures,
    typename Initiation, typename... Args>
void_or_deduced async_initiate(
    BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken),
    BOOST_ASIO_MOVE_ARG(Args)... args);

#elif defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE... Signatures,
    typename Initiation, typename... Args>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Signatures...>::value,
    BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken, Signatures...,
      (async_result<typename decay<CompletionToken>::type,
        Signatures...>::initiate(declval<BOOST_ASIO_MOVE_ARG(Initiation)>(),
          declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>(),
          declval<BOOST_ASIO_MOVE_ARG(Args)>()...)))>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    BOOST_ASIO_MOVE_ARG(Args)... args)
{
  return async_result<typename decay<CompletionToken>::type,
    Signatures...>::initiate(BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
      BOOST_ASIO_MOVE_CAST(CompletionToken)(token),
      BOOST_ASIO_MOVE_CAST(Args)(args)...);
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE... Signatures,
    typename Initiation, typename... Args>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Signatures...>::value,
    BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, Signatures...)>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    BOOST_ASIO_MOVE_ARG(Args)... args)
{
  async_completion<CompletionToken, Signatures...> completion(token);

  BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
      BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE(CompletionToken,
        Signatures...))(completion.completion_handler),
      BOOST_ASIO_MOVE_CAST(Args)(args)...);

  return completion.result.get();
}

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0>::value,
    BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken, Sig0,
      (async_result<typename decay<CompletionToken>::type,
        Sig0>::initiate(declval<BOOST_ASIO_MOVE_ARG(Initiation)>(),
          declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0>::initiate(BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
      BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig1,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1>::value,
    BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE2(CompletionToken, Sig0, Sig1,
      (async_result<typename decay<CompletionToken>::type,
        Sig0, Sig1>::initiate(declval<BOOST_ASIO_MOVE_ARG(Initiation)>(),
          declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0, Sig1>::initiate(BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
      BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig1,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig2,
    typename Initiation>
inline typename constraint<
    detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1, Sig2>::value,
    BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2,
      (async_result<typename decay<CompletionToken>::type,
        Sig0, Sig1, Sig2>::initiate(declval<BOOST_ASIO_MOVE_ARG(Initiation)>(),
          declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>())))>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  return async_result<typename decay<CompletionToken>::type,
    Sig0, Sig1, Sig2>::initiate(BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
      BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0>::value,
    BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, Sig0)>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0> completion(token);

  BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
      BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE(CompletionToken,
        Sig0))(completion.completion_handler));

  return completion.result.get();
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig1,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1>::value,
    BOOST_ASIO_INITFN_RESULT_TYPE2(CompletionToken, Sig0, Sig1)>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0, Sig1> completion(token);

  BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
      BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE2(CompletionToken,
        Sig0, Sig1))(completion.completion_handler));

  return completion.result.get();
}

template <typename CompletionToken,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig0,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig1,
    BOOST_ASIO_COMPLETION_SIGNATURE Sig2,
    typename Initiation>
inline typename constraint<
    !detail::async_result_has_initiate_memfn<
      CompletionToken, Sig0, Sig1, Sig2>::value,
    BOOST_ASIO_INITFN_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2)>::type
async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
    BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token)
{
  async_completion<CompletionToken, Sig0, Sig1, Sig2> completion(token);

  BOOST_ASIO_MOVE_CAST(Initiation)(initiation)(
      BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE3(CompletionToken,
        Sig0, Sig1, Sig2))(completion.completion_handler));

  return completion.result.get();
}

#define BOOST_ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0>::value, \
      BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE( \
        CompletionToken, Sig0, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0>::initiate( \
            declval<BOOST_ASIO_MOVE_ARG(Initiation)>(), \
            declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>(), \
            BOOST_ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0>::initiate( \
        BOOST_ASIO_MOVE_CAST(Initiation)(initiation), \
        BOOST_ASIO_MOVE_CAST(CompletionToken)(token), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig1, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1>::value, \
      BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE2( \
        CompletionToken, Sig0, Sig1, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0, Sig1>::initiate( \
            declval<BOOST_ASIO_MOVE_ARG(Initiation)>(), \
            declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>(), \
            BOOST_ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0, Sig1>::initiate( \
        BOOST_ASIO_MOVE_CAST(Initiation)(initiation), \
        BOOST_ASIO_MOVE_CAST(CompletionToken)(token), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig1, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig2, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1, Sig2>::value, \
      BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE3( \
        CompletionToken, Sig0, Sig1, Sig2, \
        (async_result<typename decay<CompletionToken>::type, \
          Sig0, Sig1, Sig2>::initiate( \
            declval<BOOST_ASIO_MOVE_ARG(Initiation)>(), \
            declval<BOOST_ASIO_MOVE_ARG(CompletionToken)>(), \
            BOOST_ASIO_VARIADIC_MOVE_DECLVAL(n))))>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_result<typename decay<CompletionToken>::type, \
      Sig0, Sig1, Sig2>::initiate( \
        BOOST_ASIO_MOVE_CAST(Initiation)(initiation), \
        BOOST_ASIO_MOVE_CAST(CompletionToken)(token), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0>::value, \
      BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, Sig0)>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0> completion(token); \
  \
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)( \
        BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE(CompletionToken, \
          Sig0))(completion.completion_handler), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig1, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1>::value, \
      BOOST_ASIO_INITFN_RESULT_TYPE2(CompletionToken, Sig0, Sig1)>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0, Sig1> completion(token); \
  \
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)( \
        BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE2(CompletionToken, \
          Sig0, Sig1))(completion.completion_handler), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  \
  template <typename CompletionToken, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig0, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig1, \
      BOOST_ASIO_COMPLETION_SIGNATURE Sig2, \
      typename Initiation, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  inline typename constraint< \
      !detail::async_result_has_initiate_memfn< \
        CompletionToken, Sig0, Sig1, Sig2>::value, \
      BOOST_ASIO_INITFN_RESULT_TYPE3(CompletionToken, Sig0, Sig1, Sig2)>::type \
  async_initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    async_completion<CompletionToken, \
      Sig0, Sig1, Sig2> completion(token); \
  \
    BOOST_ASIO_MOVE_CAST(Initiation)(initiation)( \
        BOOST_ASIO_MOVE_CAST(BOOST_ASIO_HANDLER_TYPE3(CompletionToken, \
          Sig0, Sig1, Sig2))(completion.completion_handler), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  \
    return completion.result.get(); \
  } \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_INITIATE_DEF)
#undef BOOST_ASIO_PRIVATE_INITIATE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS) \
  && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES) \
  && defined(BOOST_ASIO_HAS_DECLTYPE)

namespace detail {

template <typename... Signatures>
struct initiation_archetype
{
  template <completion_handler_for<Signatures...> CompletionHandler>
  void operator()(CompletionHandler&&) const
  {
  }
};

} // namespace detail

template <typename T, typename... Signatures>
BOOST_ASIO_CONCEPT completion_token_for =
  detail::are_completion_signatures<Signatures...>::value
  &&
  requires(T&& t)
  {
    async_initiate<T, Signatures...>(
        detail::initiation_archetype<Signatures...>{}, t);
  };

#define BOOST_ASIO_COMPLETION_TOKEN_FOR(sig) \
  ::boost::asio::completion_token_for<sig>
#define BOOST_ASIO_COMPLETION_TOKEN_FOR2(sig0, sig1) \
  ::boost::asio::completion_token_for<sig0, sig1>
#define BOOST_ASIO_COMPLETION_TOKEN_FOR3(sig0, sig1, sig2) \
  ::boost::asio::completion_token_for<sig0, sig1, sig2>

#else // defined(BOOST_ASIO_HAS_CONCEPTS)
      //   && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
      //   && defined(BOOST_ASIO_HAS_DECLTYPE)

#define BOOST_ASIO_COMPLETION_TOKEN_FOR(sig) typename
#define BOOST_ASIO_COMPLETION_TOKEN_FOR2(sig0, sig1) typename
#define BOOST_ASIO_COMPLETION_TOKEN_FOR3(sig0, sig1, sig2) typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)
       //   && defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
       //   && defined(BOOST_ASIO_HAS_DECLTYPE)

namespace detail {

template <typename T, typename = void>
struct default_completion_token_impl
{
  typedef void type;
};

template <typename T>
struct default_completion_token_impl<T,
  typename void_type<typename T::default_completion_token_type>::type>
{
  typedef typename T::default_completion_token_type type;
};

} // namespace detail

#if defined(GENERATING_DOCUMENTATION)

/// Traits type used to determine the default completion token type associated
/// with a type (such as an executor).
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type.
 *
 * Specialisations of this trait may provide a nested typedef @c type, which is
 * a default-constructible completion token type.
 */
template <typename T>
struct default_completion_token
{
  /// If @c T has a nested type @c default_completion_token_type,
  /// <tt>T::default_completion_token_type</tt>. Otherwise the typedef @c type
  /// is not defined.
  typedef see_below type;
};
#else
template <typename T>
struct default_completion_token
  : detail::default_completion_token_impl<T>
{
};
#endif

#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

template <typename T>
using default_completion_token_t = typename default_completion_token<T>::type;

#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)

#if defined(BOOST_ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

#define BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(e) \
  = typename ::boost::asio::default_completion_token<e>::type
#define BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(e) \
  = typename ::boost::asio::default_completion_token<e>::type()

#else // defined(BOOST_ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

#define BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(e)
#define BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(e)

#endif // defined(BOOST_ASIO_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_ASYNC_RESULT_HPP
