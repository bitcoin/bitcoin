
// impl/redirect_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_REDIRECT_ERROR_HPP
#define BOOST_ASIO_IMPL_REDIRECT_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associator.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>
#include <boost/system/system_error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Class to adapt a redirect_error_t as a completion handler.
template <typename Handler>
class redirect_error_handler
{
public:
  typedef void result_type;

  template <typename CompletionToken>
  redirect_error_handler(redirect_error_t<CompletionToken> e)
    : ec_(e.ec_),
      handler_(BOOST_ASIO_MOVE_CAST(CompletionToken)(e.token_))
  {
  }

  template <typename RedirectedHandler>
  redirect_error_handler(boost::system::error_code& ec,
      BOOST_ASIO_MOVE_ARG(RedirectedHandler) h)
    : ec_(ec),
      handler_(BOOST_ASIO_MOVE_CAST(RedirectedHandler)(h))
  {
  }

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)();
  }

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Arg, typename... Args>
  typename enable_if<
    !is_same<typename decay<Arg>::type, boost::system::error_code>::value
  >::type
  operator()(BOOST_ASIO_MOVE_ARG(Arg) arg, BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        BOOST_ASIO_MOVE_CAST(Arg)(arg),
        BOOST_ASIO_MOVE_CAST(Args)(args)...);
  }

  template <typename... Args>
  void operator()(const boost::system::error_code& ec,
      BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    ec_ = ec;
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        BOOST_ASIO_MOVE_CAST(Args)(args)...);
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Arg>
  typename enable_if<
    !is_same<typename decay<Arg>::type, boost::system::error_code>::value
  >::type
  operator()(BOOST_ASIO_MOVE_ARG(Arg) arg)
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        BOOST_ASIO_MOVE_CAST(Arg)(arg));
  }

  void operator()(const boost::system::error_code& ec)
  {
    ec_ = ec;
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)();
  }

#define BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF(n) \
  template <typename Arg, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  typename enable_if< \
    !is_same<typename decay<Arg>::type, boost::system::error_code>::value \
  >::type \
  operator()(BOOST_ASIO_MOVE_ARG(Arg) arg, BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)( \
        BOOST_ASIO_MOVE_CAST(Arg)(arg), \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  void operator()(const boost::system::error_code& ec, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    ec_ = ec; \
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)( \
        BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF)
#undef BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

//private:
  boost::system::error_code& ec_;
  Handler handler_;
};

template <typename Handler>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    redirect_error_handler<Handler>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    redirect_error_handler<Handler>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline bool asio_handler_is_continuation(
    redirect_error_handler<Handler>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    redirect_error_handler<Handler>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    redirect_error_handler<Handler>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Signature>
struct redirect_error_signature
{
  typedef Signature type;
};

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename... Args>
struct redirect_error_signature<R(boost::system::error_code, Args...)>
{
  typedef R type(Args...);
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const boost::system::error_code&, Args...)>
{
  typedef R type(Args...);
};

# if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct redirect_error_signature<R(boost::system::error_code, Args...) &>
{
  typedef R type(Args...) &;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const boost::system::error_code&, Args...) &>
{
  typedef R type(Args...) &;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(boost::system::error_code, Args...) &&>
{
  typedef R type(Args...) &&;
};

template <typename R, typename... Args>
struct redirect_error_signature<R(const boost::system::error_code&, Args...) &&>
{
  typedef R type(Args...) &&;
};

#  if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct redirect_error_signature<
  R(boost::system::error_code, Args...) noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const boost::system::error_code&, Args...) noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(boost::system::error_code, Args...) & noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const boost::system::error_code&, Args...) & noexcept>
{
  typedef R type(Args...) & noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(boost::system::error_code, Args...) && noexcept>
{
  typedef R type(Args...) && noexcept;
};

template <typename R, typename... Args>
struct redirect_error_signature<
  R(const boost::system::error_code&, Args...) && noexcept>
{
  typedef R type(Args...) && noexcept;
};

#  endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
# endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)
#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R>
struct redirect_error_signature<R(boost::system::error_code)>
{
  typedef R type();
};

template <typename R>
struct redirect_error_signature<R(const boost::system::error_code&)>
{
  typedef R type();
};

#define BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, BOOST_ASIO_VARIADIC_TARGS(n))> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, BOOST_ASIO_VARIADIC_TARGS(n))> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)); \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF)
#undef BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF

# if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R>
struct redirect_error_signature<R(boost::system::error_code) &>
{
  typedef R type() &;
};

template <typename R>
struct redirect_error_signature<R(const boost::system::error_code&) &>
{
  typedef R type() &;
};

template <typename R>
struct redirect_error_signature<R(boost::system::error_code) &&>
{
  typedef R type() &&;
};

template <typename R>
struct redirect_error_signature<R(const boost::system::error_code&) &&>
{
  typedef R type() &&;
};

#define BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, BOOST_ASIO_VARIADIC_TARGS(n)) &> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) &; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, BOOST_ASIO_VARIADIC_TARGS(n)) &> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) &; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, BOOST_ASIO_VARIADIC_TARGS(n)) &&> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) &&; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, BOOST_ASIO_VARIADIC_TARGS(n)) &&> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) &&; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF)
#undef BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF

#  if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R>
struct redirect_error_signature<
  R(boost::system::error_code) noexcept>
{
  typedef R type() noexcept;
};

template <typename R>
struct redirect_error_signature<
  R(const boost::system::error_code&) noexcept>
{
  typedef R type() noexcept;
};

template <typename R>
struct redirect_error_signature<
  R(boost::system::error_code) & noexcept>
{
  typedef R type() & noexcept;
};

template <typename R>
struct redirect_error_signature<
  R(const boost::system::error_code&) & noexcept>
{
  typedef R type() & noexcept;
};

template <typename R>
struct redirect_error_signature<
  R(boost::system::error_code) && noexcept>
{
  typedef R type() && noexcept;
};

template <typename R>
struct redirect_error_signature<
  R(const boost::system::error_code&) && noexcept>
{
  typedef R type() && noexcept;
};

#define BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF(n) \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, BOOST_ASIO_VARIADIC_TARGS(n)) noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) noexcept; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, \
        BOOST_ASIO_VARIADIC_TARGS(n)) noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) noexcept; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, \
        BOOST_ASIO_VARIADIC_TARGS(n)) & noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) & noexcept; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, \
        BOOST_ASIO_VARIADIC_TARGS(n)) & noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) & noexcept; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(boost::system::error_code, \
        BOOST_ASIO_VARIADIC_TARGS(n)) && noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) && noexcept; \
  }; \
  \
  template <typename R, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct redirect_error_signature< \
      R(const boost::system::error_code&, \
        BOOST_ASIO_VARIADIC_TARGS(n)) && noexcept> \
  { \
    typedef R type(BOOST_ASIO_VARIADIC_TARGS(n)) && noexcept; \
  }; \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF)
#undef BOOST_ASIO_PRIVATE_REDIRECT_ERROR_DEF

#  endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
# endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)
#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, typename Signature>
struct async_result<redirect_error_t<CompletionToken>, Signature>
{
  typedef typename async_result<CompletionToken,
    typename detail::redirect_error_signature<Signature>::type>
      ::return_type return_type;

  template <typename Initiation>
  struct init_wrapper
  {
    template <typename Init>
    init_wrapper(boost::system::error_code& ec, BOOST_ASIO_MOVE_ARG(Init) init)
      : ec_(ec),
        initiation_(BOOST_ASIO_MOVE_CAST(Init)(init))
    {
    }

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

    template <typename Handler, typename... Args>
    void operator()(
        BOOST_ASIO_MOVE_ARG(Handler) handler,
        BOOST_ASIO_MOVE_ARG(Args)... args)
    {
      BOOST_ASIO_MOVE_CAST(Initiation)(initiation_)(
          detail::redirect_error_handler<
            typename decay<Handler>::type>(
              ec_, BOOST_ASIO_MOVE_CAST(Handler)(handler)),
          BOOST_ASIO_MOVE_CAST(Args)(args)...);
    }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

    template <typename Handler>
    void operator()(
        BOOST_ASIO_MOVE_ARG(Handler) handler)
    {
      BOOST_ASIO_MOVE_CAST(Initiation)(initiation_)(
          detail::redirect_error_handler<
            typename decay<Handler>::type>(
              ec_, BOOST_ASIO_MOVE_CAST(Handler)(handler)));
    }

#define BOOST_ASIO_PRIVATE_INIT_WRAPPER_DEF(n) \
    template <typename Handler, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
    void operator()( \
        BOOST_ASIO_MOVE_ARG(Handler) handler, \
        BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
    { \
      BOOST_ASIO_MOVE_CAST(Initiation)(initiation_)( \
          detail::redirect_error_handler< \
            typename decay<Handler>::type>( \
              ec_, BOOST_ASIO_MOVE_CAST(Handler)(handler)), \
          BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
    } \
    /**/
    BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_INIT_WRAPPER_DEF)
#undef BOOST_ASIO_PRIVATE_INIT_WRAPPER_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

    boost::system::error_code& ec_;
    Initiation initiation_;
  };

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token,
      BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    return async_initiate<CompletionToken,
      typename detail::redirect_error_signature<Signature>::type>(
        init_wrapper<typename decay<Initiation>::type>(
          token.ec_, BOOST_ASIO_MOVE_CAST(Initiation)(initiation)),
        token.token_, BOOST_ASIO_MOVE_CAST(Args)(args)...);
  }

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Initiation, typename RawCompletionToken>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token)
  {
    return async_initiate<CompletionToken,
      typename detail::redirect_error_signature<Signature>::type>(
        init_wrapper<typename decay<Initiation>::type>(
          token.ec_, BOOST_ASIO_MOVE_CAST(Initiation)(initiation)),
        token.token_);
  }

#define BOOST_ASIO_PRIVATE_INITIATE_DEF(n) \
  template <typename Initiation, typename RawCompletionToken, \
      BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  static return_type initiate( \
      BOOST_ASIO_MOVE_ARG(Initiation) initiation, \
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)) \
  { \
    return async_initiate<CompletionToken, \
      typename detail::redirect_error_signature<Signature>::type>( \
        init_wrapper<typename decay<Initiation>::type>( \
          token.ec_, BOOST_ASIO_MOVE_CAST(Initiation)(initiation)), \
        token.token_, BOOST_ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_INITIATE_DEF)
#undef BOOST_ASIO_PRIVATE_INITIATE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
};

template <template <typename, typename> class Associator,
    typename Handler, typename DefaultCandidate>
struct associator<Associator,
    detail::redirect_error_handler<Handler>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::redirect_error_handler<Handler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_REDIRECT_ERROR_HPP
