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

#ifndef BOOST_TYPE_ERASURE_CONSTRUCTIBLE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_CONSTRUCTIBLE_HPP_INCLUDED

#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/type_erasure/detail/storage.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/config.hpp>
#include <boost/type_erasure/param.hpp>

namespace boost {
namespace type_erasure {

template<class Sig>
struct constructible;

namespace detail {
    
template<class Sig>
struct null_construct;

template<class C>
struct get_null_vtable_entry;

template<class C, class Sig>
struct vtable_adapter;

}

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/**
 * The @ref constructible concept enables calling the
 * constructor of a type contained by an @ref any.
 * @c Sig should be a function signature.  The return
 * type is the placeholder specifying the type to
 * be constructed.  The arguments are the argument
 * types of the constructor.  The arguments of
 * @c Sig may be placeholders.
 *
 * \note @ref constructible may not be specialized and
 * may not be passed to \call as it depends on the
 * implementation details of @ref any.
 */
template<class Sig>
struct constructible {};

#elif !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !BOOST_WORKAROUND(BOOST_MSVC, == 1800)

template<class R, class... T>
struct constructible<R(T...)>
{
    static ::boost::type_erasure::detail::storage
    apply(T... arg)
    {
        ::boost::type_erasure::detail::storage result;
        result.data = new R(::std::forward<T>(arg)...);
        return result;
    }
};

/// INTERNAL ONLY
template<class Base, class Tag, class... T>
struct concept_interface<
    ::boost::type_erasure::constructible<Tag(T...)>,
    Base,
    Tag
> : Base
{
    using Base::_boost_type_erasure_deduce_constructor;
    ::boost::type_erasure::constructible<Tag(T...)>*
    _boost_type_erasure_deduce_constructor(
        typename ::boost::type_erasure::as_param<Base, T>::type...) const
    {
        return 0;
    }
};

namespace detail {

template<class... T>
struct null_construct<void(T...)>
{
    static ::boost::type_erasure::detail::storage
    value(T...)
    {
        ::boost::type_erasure::detail::storage result;
        result.data = 0;
        return result;
    }
};

template<class T, class R, class... U>
struct get_null_vtable_entry<vtable_adapter<constructible<T(const T&)>, R(U...)> >
{
    typedef null_construct<void(U...)> type;
};

}

#else

#define BOOST_PP_FILENAME_1 <boost/type_erasure/constructible.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#endif

}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_ARG_DECL(z, n, data)     \
    typename ::boost::type_erasure::as_param<          \
        Base,                                       \
        BOOST_PP_CAT(T, n)                          \
    >::type

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_TYPE_ERASURE_FORWARD_I(z, n, data) ::std::forward<BOOST_PP_CAT(T, n)>(BOOST_PP_CAT(arg, n))
#define BOOST_TYPE_ERASURE_FORWARD(n) BOOST_PP_ENUM(n, BOOST_TYPE_ERASURE_FORWARD_I, ~)
#else
#define BOOST_TYPE_ERASURE_FORWARD(n) BOOST_PP_ENUM_PARAMS(n, arg)
#endif

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct constructible<R(BOOST_PP_ENUM_PARAMS(N, T))>
{
    static ::boost::type_erasure::detail::storage
    apply(BOOST_PP_ENUM_BINARY_PARAMS(N, T, arg))
    {
        ::boost::type_erasure::detail::storage result;
        result.data = new R(BOOST_TYPE_ERASURE_FORWARD(N));
        return result;
    }
};

template<class Base BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class Tag>
struct concept_interface<
    ::boost::type_erasure::constructible<Tag(BOOST_PP_ENUM_PARAMS(N, T))>,
    Base,
    Tag
> : Base
{
    using Base::_boost_type_erasure_deduce_constructor;
    ::boost::type_erasure::constructible<Tag(BOOST_PP_ENUM_PARAMS(N, T))>*
    _boost_type_erasure_deduce_constructor(
        BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_ARG_DECL, ~)) const
    {
        return 0;
    }
};

namespace detail {

template<BOOST_PP_ENUM_PARAMS(N, class T)>
struct null_construct<void(BOOST_PP_ENUM_PARAMS(N, T))>
{
    static ::boost::type_erasure::detail::storage
    value(BOOST_PP_ENUM_PARAMS(N, T))
    {
        ::boost::type_erasure::detail::storage result;
        result.data = 0;
        return result;
    }
};

template<class T, class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct get_null_vtable_entry<vtable_adapter<constructible<T(const T&)>, R(BOOST_PP_ENUM_PARAMS(N, T))> >
{
    typedef null_construct<void(BOOST_PP_ENUM_PARAMS(N, T))> type;
};

}

#undef BOOST_TYPE_ERASURE_FORWARD
#undef BOOST_TYPE_ERASURE_FORWARD_I

#undef BOOST_TYPE_ERASURE_ARG_DECL
#undef N

#endif
