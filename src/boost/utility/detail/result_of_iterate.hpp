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
#if !defined(BOOST_PP_IS_ITERATING)
# error Boost result_of - do not include this file!
#endif

// CWPro8 requires an argument in a function type specialization
#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3002)) && BOOST_PP_ITERATION() == 0
# define BOOST_RESULT_OF_ARGS void
#else
# define BOOST_RESULT_OF_ARGS BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)
#endif

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of<F(BOOST_RESULT_OF_ARGS)>
    : conditional<
        is_pointer<F>::value || is_member_function_pointer<F>::value
        , boost::detail::tr1_result_of_impl<
            typename remove_cv<F>::type,
            typename remove_cv<F>::type(BOOST_RESULT_OF_ARGS),
            (boost::detail::result_of_has_result_type<F>::value)>
        , boost::detail::tr1_result_of_impl<
            F,
            F(BOOST_RESULT_OF_ARGS),
            (boost::detail::result_of_has_result_type<F>::value)> >::type { };
#endif

#ifdef BOOST_RESULT_OF_USE_DECLTYPE
template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct result_of<F(BOOST_RESULT_OF_ARGS)>
    : detail::cpp0x_result_of<F(BOOST_RESULT_OF_ARGS)> { };
#endif // BOOST_RESULT_OF_USE_DECLTYPE

#ifdef BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK
template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct result_of<F(BOOST_RESULT_OF_ARGS)>
    : conditional<detail::result_of_has_result_type<F>::value || detail::result_of_has_result<F>::value,
               tr1_result_of<F(BOOST_RESULT_OF_ARGS)>,
               detail::cpp0x_result_of<F(BOOST_RESULT_OF_ARGS)> >::type { };
#endif // BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK

#if defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

namespace detail {

template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct cpp0x_result_of<F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T))>
    : conditional<
          is_member_function_pointer<F>::value
        , detail::tr1_result_of_impl<
            typename remove_cv<F>::type,
            typename remove_cv<F>::type(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), false
          >
        , detail::cpp0x_result_of_impl<
              F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T))
          >
      >::type
{};

#ifdef BOOST_NO_SFINAE_EXPR

template<typename F>
struct BOOST_PP_CAT(result_of_callable_fun_2_, BOOST_PP_ITERATION());

template<typename R BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), typename T)>
struct BOOST_PP_CAT(result_of_callable_fun_2_, BOOST_PP_ITERATION())<R(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), T))> {
    R operator()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), T)) const;
    typedef result_of_private_type const &(*pfn_t)(...);
    operator pfn_t() const volatile;
};

template<typename F>
struct BOOST_PP_CAT(result_of_callable_fun_, BOOST_PP_ITERATION())
  : BOOST_PP_CAT(result_of_callable_fun_2_, BOOST_PP_ITERATION())<F>
{};

template<typename F>
struct BOOST_PP_CAT(result_of_callable_fun_, BOOST_PP_ITERATION())<F *>
  : BOOST_PP_CAT(result_of_callable_fun_2_, BOOST_PP_ITERATION())<F>
{};

template<typename F>
struct BOOST_PP_CAT(result_of_select_call_wrapper_type_, BOOST_PP_ITERATION())
  : conditional<
        is_class<typename remove_reference<F>::type>::value,
        result_of_wrap_callable_class<F>,
        type_identity<BOOST_PP_CAT(result_of_callable_fun_, BOOST_PP_ITERATION())<typename remove_cv<typename remove_reference<F>::type>::type> >
    >::type
{};

template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), typename T)>
struct BOOST_PP_CAT(result_of_is_callable_, BOOST_PP_ITERATION()) {
    typedef typename BOOST_PP_CAT(result_of_select_call_wrapper_type_, BOOST_PP_ITERATION())<F>::type wrapper_t;
    static const bool value = (
        sizeof(result_of_no_type) == sizeof(detail::result_of_is_private_type(
            (boost::declval<wrapper_t>()(BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), boost::declval<T, >() BOOST_PP_INTERCEPT)), result_of_weird_type())
        ))
    );
    typedef integral_constant<bool, value> type;
};

template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct cpp0x_result_of_impl<F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), true>
    : lazy_enable_if<
          BOOST_PP_CAT(result_of_is_callable_, BOOST_PP_ITERATION())<F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), T)>
        , cpp0x_result_of_impl<F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), false>
      >
{};

template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct cpp0x_result_of_impl<F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), false>
{
  typedef decltype(
    boost::declval<F>()(
      BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), boost::declval<T, >() BOOST_PP_INTERCEPT)
    )
  ) type;
};

#else // BOOST_NO_SFINAE_EXPR

template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct cpp0x_result_of_impl<F(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)),
                            typename result_of_always_void<decltype(
                                boost::declval<F>()(
                                    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), boost::declval<T, >() BOOST_PP_INTERCEPT)
                                )
                            )>::type> {
  typedef decltype(
    boost::declval<F>()(
      BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), boost::declval<T, >() BOOST_PP_INTERCEPT)
    )
  ) type;
};

#endif // BOOST_NO_SFINAE_EXPR

} // namespace detail

#else // defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_RESULT_OF_USE_TR1_WITH_DECLTYPE_FALLBACK)

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
template<typename F BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct result_of<F(BOOST_RESULT_OF_ARGS)>
    : tr1_result_of<F(BOOST_RESULT_OF_ARGS)> { };
#endif

#endif // defined(BOOST_RESULT_OF_USE_DECLTYPE)

#undef BOOST_RESULT_OF_ARGS

#if BOOST_PP_ITERATION() >= 1

namespace detail {

template<typename R,  typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), FArgs, false>
{
  typedef R type;
};

template<typename R,  typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (&)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(),T)), FArgs, false>
{
  typedef R type;
};

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
template<typename R, typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (T0::*)
                     (BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_ITERATION(),T)),
                 FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (T0::*)
                     (BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_ITERATION(),T))
                     const,
                 FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (T0::*)
                     (BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_ITERATION(),T))
                     volatile,
                 FArgs, false>
{
  typedef R type;
};

template<typename R, typename FArgs BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(),typename T)>
struct tr1_result_of_impl<R (T0::*)
                     (BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_ITERATION(),T))
                     const volatile,
                 FArgs, false>
{
  typedef R type;
};
#endif

}
#endif
