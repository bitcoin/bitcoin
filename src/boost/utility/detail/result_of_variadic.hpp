// Boost result_of library

//  Copyright Douglas Gregor 2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Copyright Daniel Walker, Eric Niebler, Michel Morin 2008-2012.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or
//  copy at http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org/libs/utility

#ifndef BOOST_RESULT_OF_HPP
# error Boost result_of - do not include this file!
#endif

template<typename F, typename... Args>
struct tr1_result_of<F(Args...)>
    : conditional<
        is_pointer<F>::value || is_member_function_pointer<F>::value
        , boost::detail::tr1_result_of_impl<
            typename remove_cv<F>::type,
            typename remove_cv<F>::type(Args...),
            (boost::detail::result_of_has_result_type<F>::value)>
        , boost::detail::tr1_result_of_impl<
            F,
            F(Args...),
            (boost::detail::result_of_has_result_type<F>::value)> >::type { };

#ifdef BOOST_RESULT_OF_USE_DECLTYPE
template<typename F, typename... Args>
struct result_of<F(Args...)>
    : detail::cpp0x_result_of<F(Args...)> { };
#endif // BOOST_RESULT_OF_USE_DECLTYPE

#ifdef BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK
template<typename F, typename... Args>
struct result_of<F(Args...)>
    : conditional<detail::result_of_has_result_type<F>::value || detail::result_of_has_result<F>::value,
               tr1_result_of<F(Args...)>,
               detail::cpp0x_result_of<F(Args...)> >::type { };
#endif // BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK

#if defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

namespace detail {

template<typename F, typename... Args>
struct cpp0x_result_of<F(Args...)>
    : conditional<
          is_member_function_pointer<F>::value
        , detail::tr1_result_of_impl<
            typename remove_cv<F>::type,
            typename remove_cv<F>::type(Args...), false
          >
        , detail::cpp0x_result_of_impl<
              F(Args...)
          >
      >::type
{};

#ifdef BOOST_NO_SFINAE_EXPR

template<typename F>
struct result_of_callable_fun_2;

template<typename R, typename... Args>
struct result_of_callable_fun_2<R(Args...)> {
    R operator()(Args...) const;
    typedef result_of_private_type const &(*pfn_t)(...);
    operator pfn_t() const volatile;
};

template<typename F>
struct result_of_callable_fun
  : result_of_callable_fun_2<F>
{};

template<typename F>
struct result_of_callable_fun<F *>
  : result_of_callable_fun_2<F>
{};

template<typename F>
struct result_of_select_call_wrapper_type
  : conditional<
        is_class<typename remove_reference<F>::type>::value,
        result_of_wrap_callable_class<F>,
        type_identity<result_of_callable_fun<typename remove_cv<typename remove_reference<F>::type>::type> >
    >::type
{};

template<typename F, typename... Args>
struct result_of_is_callable {
    typedef typename result_of_select_call_wrapper_type<F>::type wrapper_t;
    static const bool value = (
        sizeof(result_of_no_type) == sizeof(detail::result_of_is_private_type(
            (boost::declval<wrapper_t>()(boost::declval<Args>()...), result_of_weird_type())
        ))
    );
    typedef integral_constant<bool, value> type;
};

template<typename F, typename... Args>
struct cpp0x_result_of_impl<F(Args...), true>
    : lazy_enable_if<
          result_of_is_callable<F, Args...>
        , cpp0x_result_of_impl<F(Args...), false>
      >
{};

template<typename F, typename... Args>
struct cpp0x_result_of_impl<F(Args...), false>
{
  typedef decltype(
    boost::declval<F>()(
      boost::declval<Args>()...
    )
  ) type;
};

#else // BOOST_NO_SFINAE_EXPR

template<typename F, typename... Args>
struct cpp0x_result_of_impl<F(Args...),
                            typename result_of_always_void<decltype(
                                boost::declval<F>()(
                                    boost::declval<Args>()...
                                )
                            )>::type> {
  typedef decltype(
    boost::declval<F>()(
      boost::declval<Args>()...
    )
  ) type;
};

#endif // BOOST_NO_SFINAE_EXPR

} // namespace detail

#else // defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

template<typename F, typename... Args>
struct result_of<F(Args...)>
    : tr1_result_of<F(Args...)> { };

#endif // defined(BOOST_RESULT_OF_USE_DECLTYPE)

namespace detail {

template<typename R, typename FArgs, typename... Args>
struct tr1_result_of_impl<R (*)(Args...), FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs, typename... Args>
struct tr1_result_of_impl<R (&)(Args...), FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs, typename C, typename... Args>
struct tr1_result_of_impl<R (C::*)(Args...), FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs, typename C, typename... Args>
struct tr1_result_of_impl<R (C::*)(Args...) const, FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs, typename C, typename... Args>
struct tr1_result_of_impl<R (C::*)(Args...) volatile, FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs, typename C, typename... Args>
struct tr1_result_of_impl<R (C::*)(Args...) const volatile, FArgs, false>
{
  typedef R type;
};

}
