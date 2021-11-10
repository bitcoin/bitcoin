// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_CHECK_CALL_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_CHECK_CALL_HPP_INCLUDED

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/type_erasure/placeholder_of.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<class Sig, class Args>
struct check_call : ::boost::mpl::false_ {};

template<class T, class Enable = void>
struct qualified_placeholder
{
    typedef void type;
};

template<class T>
struct qualified_placeholder<T&, typename T::_boost_type_erasure_is_any>
{
    typedef typename ::boost::type_erasure::placeholder_of<T>::type placeholder;
    typedef typename ::boost::remove_reference<placeholder>::type unref;
    typedef typename ::boost::mpl::if_< ::boost::is_const<T>,
        const unref,
        unref
    >::type add_const;
    typedef typename ::boost::mpl::if_< ::boost::is_reference<placeholder>,
        placeholder,
        add_const&
    >::type type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct qualified_placeholder<T&&, typename T::_boost_type_erasure_is_any>
{
    typedef typename ::boost::type_erasure::placeholder_of<T>::type placeholder;
    typedef placeholder&& type;
};

#endif

template<class P, class A>
struct check_placeholder_arg_impl : ::boost::mpl::false_ {};

template<class P>
struct check_placeholder_arg_impl<P, P&> : ::boost::mpl::true_ {};

template<class P>
struct check_placeholder_arg_impl<P, const P&> : ::boost::mpl::true_ {};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class P>
struct check_placeholder_arg_impl<P, P&&> : ::boost::mpl::true_ {};

#endif

template<class P>
struct check_placeholder_arg_impl<P&, P&> : ::boost::mpl::true_ {};

template<class P>
struct check_placeholder_arg_impl<const P&, P&> : ::boost::mpl::true_ {};

template<class P>
struct check_placeholder_arg_impl<const P&, const P&> : ::boost::mpl::true_ {};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class P>
struct check_placeholder_arg_impl<const P&, P&&> : ::boost::mpl::true_ {};

template<class P>
struct check_placeholder_arg_impl<P&&, P&&> : ::boost::mpl::true_ {};

#endif

template<class P, class Arg>
struct check_placeholder_arg :
    check_placeholder_arg_impl<
        P,
        typename ::boost::type_erasure::detail::qualified_placeholder<Arg>::type
    >::type
{};

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    ((defined(__GNUC__) && !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6))) || \
    defined(__MINGW32__) || defined(__MINGW64__) || \
    BOOST_WORKAROUND(BOOST_MSVC, <= 1700))
#define BOOST_TYPE_ERASURE_BROKEN_RVALUE_IS_CONVERTIBLE
#endif

#ifdef BOOST_TYPE_ERASURE_USE_MP11

template<class P, class Arg>
using check_placeholder_arg_t =
    typename ::boost::type_erasure::detail::check_placeholder_arg_impl<
        P,
        typename ::boost::type_erasure::detail::qualified_placeholder<Arg>::type
    >::type;

template<class T, class Arg>
using check_nonplaceholder_arg_t = typename ::boost::is_convertible<Arg, T>::type;

template<class FormalArg, class ActualArg>
using check_arg_t =
    ::boost::type_erasure::detail::eval_if<
        ::boost::type_erasure::is_placeholder<
            ::boost::remove_cv_t<
                ::boost::remove_reference_t<FormalArg>
            >
        >::value,
        ::boost::type_erasure::detail::check_placeholder_arg_t,
        ::boost::type_erasure::detail::check_nonplaceholder_arg_t,
        FormalArg,
        ActualArg
    >;

// MSVC 14.1 ICE's if we use check_arg_t directly.
template<class FormalArg, class ActualArg>
struct check_arg
{
    typedef ::boost::type_erasure::detail::check_arg_t<FormalArg, ActualArg> type;
};

template<class R, class... T, class... U>
struct check_call<R(T...), void(U...)> {
    typedef ::boost::mp11::mp_all<
        typename ::boost::type_erasure::detail::check_arg<T, U>::type...
    > type;
};

#else

template<class FormalArg, class ActualArg>
struct check_arg
{
    typedef typename ::boost::mpl::eval_if<
        is_placeholder<
            typename ::boost::remove_cv<
                typename ::boost::remove_reference<FormalArg>::type
            >::type
        >,
        ::boost::type_erasure::detail::check_placeholder_arg<FormalArg, ActualArg>,
#ifdef BOOST_TYPE_ERASURE_BROKEN_RVALUE_IS_CONVERTIBLE
        ::boost::mpl::true_
#else
        ::boost::is_convertible<ActualArg, FormalArg>
#endif
    >::type type;
};

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/check_call.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#else

template<class... B>
struct and_;

template<class T1, class... T>
struct and_<T1, T...> : boost::mpl::eval_if_c<T1::type::value, and_<T...>, boost::mpl::false_>::type {};

template<class T1>
struct and_<T1> : T1::type {};

template<>
struct and_<> : boost::mpl::true_ {};

template<class R, class... T, class... U>
struct check_call<R(T...), void(U...)> {
    typedef typename ::boost::type_erasure::detail::and_<
        ::boost::type_erasure::detail::check_arg<T, U>...
    >::type type;
};

#endif

#endif

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_CHECK_ARG(z, n, data)                \
    typedef typename ::boost::type_erasure::detail::check_arg<  \
        BOOST_PP_CAT(T, n),                                     \
        BOOST_PP_CAT(U, n)                                      \
    >::type BOOST_PP_CAT(check, n);                             \
    typedef typename ::boost::mpl::and_<                        \
        BOOST_PP_CAT(type, n),                                  \
        BOOST_PP_CAT(check, n)                                  \
    >::type BOOST_PP_CAT(type, BOOST_PP_INC(n));

template<
    class R
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class U)
>
struct check_call<R(BOOST_PP_ENUM_PARAMS(N, T)), void(BOOST_PP_ENUM_BINARY_PARAMS(N, U, u))> {
    typedef ::boost::mpl::true_ type0;
    BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_CHECK_ARG, ~)
    typedef BOOST_PP_CAT(type, N) type;
};

#undef N

#endif
