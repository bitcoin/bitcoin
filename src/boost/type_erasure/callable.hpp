// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_CALLABLE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_CALLABLE_HPP_INCLUDED

#include <boost/detail/workaround.hpp>
#include <boost/utility/declval.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#include <boost/type_erasure/config.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <boost/type_erasure/param.hpp>

namespace boost {
namespace type_erasure {

template<class Sig, class F = _self>
struct callable;

namespace detail {

template<class Sig>
struct result_of_callable;

}

#if defined(BOOST_TYPE_ERASURE_DOXYGEN)

/**
 * The @ref callable concept allows an @ref any to hold function objects.
 * @c Sig is interpreted in the same way as for Boost.Function, except
 * that the arguments and return type are allowed to be placeholders.
 * @c F must be a @ref placeholder.
 *
 * Multiple instances of @ref callable can be used
 * simultaneously.  Overload resolution works normally.
 * Note that unlike Boost.Function, @ref callable
 * does not provide result_type.  It does, however,
 * support @c boost::result_of.
 */
template<class Sig, class F = _self>
struct callable
{
    /**
     * @c R is the result type of @c Sig and @c T is the argument
     * types of @c Sig.
     */
    static R apply(F& f, T... arg);
};

#elif !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !BOOST_WORKAROUND(BOOST_MSVC, == 1800)

template<class R, class... T, class F>
struct callable<R(T...), F>
{
    static R apply(F& f, T... arg)
    {
        return f(std::forward<T>(arg)...);
    }
};

template<class... T, class F>
struct callable<void(T...), F>
{
    static void apply(F& f, T... arg)
    {
        f(std::forward<T>(arg)...);
    }
};

template<class R, class F, class Base, class Enable, class... T>
struct concept_interface<callable<R(T...), F>, Base, F, Enable>
  : Base
{
    template<class Sig>
    struct result :
        ::boost::type_erasure::detail::result_of_callable<Sig>
    {};
    typedef void _boost_type_erasure_is_callable;
    typedef ::boost::mpl::vector<R> _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[1];
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        typename ::boost::type_erasure::as_param<Base, T>::type...);
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(typename ::boost::type_erasure::as_param<Base, T>::type... arg)
    {
        return ::boost::type_erasure::call(callable<R(T...), F>(), *this,
            ::std::forward<typename ::boost::type_erasure::as_param<Base, T>::type>(arg)...);
    }
};

template<class R, class F, class Base, class Enable, class... T>
struct concept_interface<callable<R(T...), const F>, Base, F, Enable>
  : Base
{
    template<class Sig>
    struct result :
        ::boost::type_erasure::detail::result_of_callable<Sig>
    {};
    typedef void _boost_type_erasure_is_callable;
    typedef ::boost::mpl::vector<R> _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[1];
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        typename ::boost::type_erasure::as_param<Base, T>::type...) const;
    typename ::boost::type_erasure::rebind_any<Base, R>::type operator()(
        typename ::boost::type_erasure::as_param<Base, T>::type... arg) const
    {
        return ::boost::type_erasure::call(callable<R(T...), const F>(), *this,
            ::std::forward<typename ::boost::type_erasure::as_param<Base, T>::type>(arg)...);
    }
};

template<class R, class F, class Base, class... T>
struct concept_interface<
    callable<R(T...), F>,
    Base,
    F,
    typename Base::_boost_type_erasure_is_callable
>
  : Base
{
    typedef typename ::boost::mpl::push_back<
        typename Base::_boost_type_erasure_callable_results,
        R
    >::type _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[
        ::boost::mpl::size<_boost_type_erasure_callable_results>::value];
    using Base::_boost_type_erasure_deduce_callable;
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        typename ::boost::type_erasure::as_param<Base, T>::type...);
    using Base::operator();
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(typename ::boost::type_erasure::as_param<Base, T>::type... arg)
    {
        return ::boost::type_erasure::call(callable<R(T...), F>(), *this,
            ::std::forward<typename ::boost::type_erasure::as_param<Base, T>::type>(arg)...);
    }
};

template<class R, class F, class Base, class... T>
struct concept_interface<
    callable<R(T...), const F>,
    Base,
    F,
    typename Base::_boost_type_erasure_is_callable
>
  : Base
{
    typedef typename ::boost::mpl::push_back<
        typename Base::_boost_type_erasure_callable_results,
        R
    >::type _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[
        ::boost::mpl::size<_boost_type_erasure_callable_results>::value];
    using Base::_boost_type_erasure_deduce_callable;
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        typename ::boost::type_erasure::as_param<Base, T>::type...) const;
    using Base::operator();
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(typename ::boost::type_erasure::as_param<Base, T>::type... arg) const
    {
        return ::boost::type_erasure::call(callable<R(T...), const F>(), *this,
            ::std::forward<typename ::boost::type_erasure::as_param<Base, T>::type>(arg)...);
    }
};

namespace detail {

template<class This, class... T>
struct result_of_callable<This(T...)>
{
    typedef typename ::boost::mpl::at_c<
        typename This::_boost_type_erasure_callable_results,
        sizeof(::boost::declval<This>().
            _boost_type_erasure_deduce_callable(::boost::declval<T>()...)) - 1
    >::type type;
};

}

#else

/** INTERNAL ONLY */
#define BOOST_PP_FILENAME_1 <boost/type_erasure/callable.hpp>
/** INTERNAL ONLY */
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_DEC(BOOST_TYPE_ERASURE_MAX_ARITY))
#include BOOST_PP_ITERATE()

#endif

}
}

#endif

#else

#define N BOOST_PP_ITERATION()
#define BOOST_TYPE_ERASURE_DECLVAL(z, n, data) ::boost::declval<BOOST_PP_CAT(T, n)>()

#define BOOST_TYPE_ERASURE_REBIND(z, n, data)\
    typename ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(T, n)>::type BOOST_PP_CAT(arg, n)

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_TYPE_ERASURE_FORWARD(z, n, data) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 1, data), n)
#define BOOST_TYPE_ERASURE_FORWARD_REBIND(z, n, data) BOOST_PP_CAT(arg, n)
#else
#define BOOST_TYPE_ERASURE_FORWARD(z, n, data) ::std::forward<BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, data), n)>(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 1, data), n))
#define BOOST_TYPE_ERASURE_FORWARD_REBIND(z, n, data) ::std::forward<typename ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(T, n)>::type>(BOOST_PP_CAT(arg, n))
#endif

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class F>
struct callable<R(BOOST_PP_ENUM_PARAMS(N, T)), F>
{
    static R apply(F& f BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, T, arg))
    {
        return f(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_FORWARD, (T, arg)));
    }
};

template<BOOST_PP_ENUM_PARAMS(N, class T) BOOST_PP_COMMA_IF(N) class F>
struct callable<void(BOOST_PP_ENUM_PARAMS(N, T)), F>
{
    static void apply(F& f BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, T, arg))
    {
        f(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_FORWARD, (T, arg)));
    }
};

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class F, class Base, class Enable>
struct concept_interface<
    callable<R(BOOST_PP_ENUM_PARAMS(N, T)), F>,
    Base,
    F,
    Enable
>
  : Base
{
    template<class Sig>
    struct result :
        ::boost::type_erasure::detail::result_of_callable<Sig>
    {};
    typedef void _boost_type_erasure_is_callable;
    typedef ::boost::mpl::vector<R> _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[1];
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~));
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~))
    {
        return ::boost::type_erasure::call(
            callable<R(BOOST_PP_ENUM_PARAMS(N, T)), F>(),
            *this BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FORWARD_REBIND, ~));
    }
};

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class F, class Base, class Enable>
struct concept_interface<
    callable<R(BOOST_PP_ENUM_PARAMS(N, T)), const F>,
    Base,
    F,
    Enable
>
  : Base
{
    template<class Sig>
    struct result :
        ::boost::type_erasure::detail::result_of_callable<Sig>
    {};
    typedef void _boost_type_erasure_is_callable;
    typedef ::boost::mpl::vector<R> _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[1];
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~)) const;
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~)) const
    {
        return ::boost::type_erasure::call(
            callable<R(BOOST_PP_ENUM_PARAMS(N, T)), const F>(),
            *this BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FORWARD_REBIND, ~));
    }
};

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class F, class Base>
struct concept_interface<
    callable<R(BOOST_PP_ENUM_PARAMS(N, T)), F>,
    Base,
    F,
    typename Base::_boost_type_erasure_is_callable
>
  : Base
{
    typedef typename ::boost::mpl::push_back<
        typename Base::_boost_type_erasure_callable_results,
        R
    >::type _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[
        ::boost::mpl::size<_boost_type_erasure_callable_results>::value];
    using Base::_boost_type_erasure_deduce_callable;
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~));
    using Base::operator();
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~))
    {
        return ::boost::type_erasure::call(
            callable<R(BOOST_PP_ENUM_PARAMS(N, T)), F>(),
            *this BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FORWARD_REBIND, ~));
    }
};

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class F, class Base>
struct concept_interface<
    callable<R(BOOST_PP_ENUM_PARAMS(N, T)), const F>,
    Base,
    F,
    typename Base::_boost_type_erasure_is_callable
>
  : Base
{
    typedef typename ::boost::mpl::push_back<
        typename Base::_boost_type_erasure_callable_results,
        R
    >::type _boost_type_erasure_callable_results;
    typedef char (&_boost_type_erasure_callable_size)[
        ::boost::mpl::size<_boost_type_erasure_callable_results>::value];
    using Base::_boost_type_erasure_deduce_callable;
    _boost_type_erasure_callable_size
    _boost_type_erasure_deduce_callable(
        BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~)) const;
    using Base::operator();
    typename ::boost::type_erasure::rebind_any<Base, R>::type
    operator()(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REBIND, ~)) const
    {
        return ::boost::type_erasure::call(
            callable<R(BOOST_PP_ENUM_PARAMS(N, T)), const F>(),
            *this BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FORWARD_REBIND, ~));
    }
};

namespace detail {

template<class This BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct result_of_callable<This(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef typename ::boost::mpl::at_c<
        typename This::_boost_type_erasure_callable_results,
        sizeof(::boost::declval<This>().
            _boost_type_erasure_deduce_callable(
                BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_DECLVAL, ~))) - 1
    >::type type;
};

}

#undef BOOST_TYPE_ERASURE_DECLVAL
#undef BOOST_TYPE_ERASURE_REBIND
#undef N

#endif
