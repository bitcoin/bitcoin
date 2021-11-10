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

#ifndef BOOST_TYPE_ERASURE_DETAIL_ADAPT_TO_VTABLE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_ADAPT_TO_VTABLE_HPP_INCLUDED

#include <boost/detail/workaround.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/type_erasure/detail/get_signature.hpp>
#include <boost/type_erasure/detail/storage.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/type_erasure/config.hpp>

namespace boost {
namespace type_erasure {

namespace detail {
    
template<class T, class Out>
struct get_placeholders;

template<class PrimitiveConcept, class Sig>
struct vtable_adapter;

template<class PrimitiveConcept, class Sig, class Out>
struct get_placeholders<vtable_adapter<PrimitiveConcept, Sig>, Out>
{
    typedef typename get_placeholders<PrimitiveConcept, Out>::type type;
};

template<class T>
struct replace_param_for_vtable
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        const ::boost::type_erasure::detail::storage&,
        T
    >::type type;
};

template<class T>
struct replace_param_for_vtable<T&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage&,
        T&
    >::type type;
};

template<class T>
struct replace_param_for_vtable<const T&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        const ::boost::type_erasure::detail::storage&,
        const T&
    >::type type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct replace_param_for_vtable<T&&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage&&,
        T&&
    >::type type;
};

#endif

template<class T>
struct replace_result_for_vtable
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage,
        T
    >::type type;
};

template<class T>
struct replace_result_for_vtable<T&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage&,
        T&
    >::type type;
};

template<class T>
struct replace_result_for_vtable<const T&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage&,
        const T&
    >::type type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct replace_result_for_vtable<T&&>
{
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<typename ::boost::remove_cv<T>::type>,
        ::boost::type_erasure::detail::storage&&,
        T&&
    >::type type;
};

#endif

template<class Sig>
struct get_vtable_signature;

BOOST_MPL_HAS_XXX_TRAIT_DEF(type)

template<class T>
struct is_internal_concept :
    ::boost::type_erasure::detail::has_type<T>
{};

template<class PrimitiveConcept>
struct adapt_to_vtable
{
    typedef ::boost::type_erasure::detail::vtable_adapter<
        PrimitiveConcept,
        typename ::boost::type_erasure::detail::get_vtable_signature<
            typename ::boost::type_erasure::detail::get_signature<
                PrimitiveConcept
            >::type
        >::type
    > type;
};

template<class Concept>
struct maybe_adapt_to_vtable
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::type_erasure::detail::is_internal_concept<Concept>,
        ::boost::mpl::identity<Concept>,
        ::boost::type_erasure::detail::adapt_to_vtable<Concept>
    >::type type;
};

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !BOOST_WORKAROUND(BOOST_MSVC, == 1800)

template<class PrimitiveConcept, class Sig, class ConceptSig>
struct vtable_adapter_impl;

template<class PrimitiveConcept, class R, class... T, class R2, class... U>
struct vtable_adapter_impl<PrimitiveConcept, R(T...), R2(U...)>
{
    typedef R (*type)(T...);
    static R value(T... arg)
    {
        return PrimitiveConcept::apply(
            ::boost::type_erasure::detail::extract<U>(std::forward<T>(arg))...);
    }
};

template<class PrimitiveConcept, class... T, class R2, class... U>
struct vtable_adapter_impl<PrimitiveConcept, ::boost::type_erasure::detail::storage(T...), R2(U...)>
{
    typedef ::boost::type_erasure::detail::storage (*type)(T...);
    static ::boost::type_erasure::detail::storage value(T... arg)
    {
        return ::boost::type_erasure::detail::storage(
            PrimitiveConcept::apply(::boost::type_erasure::detail::extract<U>(std::forward<T>(arg))...));
    }
};

template<class PrimitiveConcept, class... T, class R2, class... U>
struct vtable_adapter_impl<PrimitiveConcept, ::boost::type_erasure::detail::storage&(T...), R2(U...)>
{
    typedef ::boost::type_erasure::detail::storage (*type)(T...);
    static ::boost::type_erasure::detail::storage value(T... arg)
    {
        ::boost::type_erasure::detail::storage result;
        typename ::boost::remove_reference<R2>::type* p =
            ::boost::addressof(
                PrimitiveConcept::apply(::boost::type_erasure::detail::extract<U>(std::forward<T>(arg))...));
        result.data = const_cast<void*>(static_cast<const void*>(p));
        return result;
    }
};

template<class PrimitiveConcept, class... T, class R2, class... U>
struct vtable_adapter_impl<PrimitiveConcept, ::boost::type_erasure::detail::storage&&(T...), R2(U...)>
{
    typedef ::boost::type_erasure::detail::storage (*type)(T...);
    static ::boost::type_erasure::detail::storage value(T... arg)
    {
        ::boost::type_erasure::detail::storage result;
        R2 tmp = PrimitiveConcept::apply(::boost::type_erasure::detail::extract<U>(std::forward<T>(arg))...);
        typename ::boost::remove_reference<R2>::type* p = ::boost::addressof(tmp);
        result.data = const_cast<void*>(static_cast<const void*>(p));
        return result;
    }
};

template<class PrimitiveConcept, class Sig>
struct vtable_adapter
    : vtable_adapter_impl<
        PrimitiveConcept,
        Sig,
        typename ::boost::type_erasure::detail::get_signature<
            PrimitiveConcept
        >::type
    >
{};

template<class R, class... T>
struct get_vtable_signature<R(T...)>
{
    typedef typename ::boost::type_erasure::detail::replace_result_for_vtable<
        R
    >::type type(typename ::boost::type_erasure::detail::replace_param_for_vtable<T>::type...);
};

#else

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/adapt_to_vtable.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#endif

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

#define BOOST_TYPE_ERASURE_EXTRACT(z, n, data)                  \
    ::boost::type_erasure::detail::extract<                     \
        typename traits::                                       \
        BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(n)), _type) \
    >(std::forward<BOOST_PP_CAT(T, n)>(BOOST_PP_CAT(arg, n)))

#else

#define BOOST_TYPE_ERASURE_EXTRACT(z, n, data)                  \
    ::boost::type_erasure::detail::extract<                     \
        typename traits::                                       \
        BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(n)), _type) \
    >(BOOST_PP_CAT(arg, n))

#endif

#define BOOST_TYPE_ERASURE_REPLACE_PARAM(z, n, data)                    \
    typename ::boost::type_erasure::detail::replace_param_for_vtable<   \
        BOOST_PP_CAT(T, n)>::type

template<class PrimitiveConcept, class R
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct vtable_adapter<PrimitiveConcept, R(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef R (*type)(BOOST_PP_ENUM_PARAMS(N, T));
    static R value(BOOST_PP_ENUM_BINARY_PARAMS(N, T, arg))
    {
#if N > 0
        typedef typename ::boost::function_traits<
            typename ::boost::type_erasure::detail::get_signature<
                PrimitiveConcept
            >::type
        > traits;
#endif
        return PrimitiveConcept::apply(
            BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_EXTRACT, ~));
    }
};

template<class PrimitiveConcept
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct vtable_adapter<PrimitiveConcept, ::boost::type_erasure::detail::storage(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef ::boost::type_erasure::detail::storage (*type)(BOOST_PP_ENUM_PARAMS(N, T));
    static ::boost::type_erasure::detail::storage value(BOOST_PP_ENUM_BINARY_PARAMS(N, T, arg))
    {
#if N > 0
        typedef typename ::boost::function_traits<
            typename ::boost::type_erasure::detail::get_signature<
                PrimitiveConcept
            >::type
        > traits;
#endif
        return ::boost::type_erasure::detail::storage(
            PrimitiveConcept::apply(
                BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_EXTRACT, ~)));
    }
};

template<class PrimitiveConcept
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct vtable_adapter<PrimitiveConcept, ::boost::type_erasure::detail::storage&(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef ::boost::type_erasure::detail::storage (*type)(BOOST_PP_ENUM_PARAMS(N, T));
    static ::boost::type_erasure::detail::storage value(BOOST_PP_ENUM_BINARY_PARAMS(N, T, arg))
    {
        typedef typename ::boost::function_traits<
            typename ::boost::type_erasure::detail::get_signature<
                PrimitiveConcept
            >::type
        > traits;
        ::boost::type_erasure::detail::storage result;
        typename ::boost::remove_reference<typename traits::result_type>::type* p =
            ::boost::addressof(
                PrimitiveConcept::apply(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_EXTRACT, ~)));
        result.data = const_cast<void*>(static_cast<const void*>(p));
        return result;
    }
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class PrimitiveConcept
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct vtable_adapter<PrimitiveConcept, ::boost::type_erasure::detail::storage&&(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef ::boost::type_erasure::detail::storage (*type)(BOOST_PP_ENUM_PARAMS(N, T));
    static ::boost::type_erasure::detail::storage value(BOOST_PP_ENUM_BINARY_PARAMS(N, T, arg))
    {
        typedef typename ::boost::function_traits<
            typename ::boost::type_erasure::detail::get_signature<
                PrimitiveConcept
            >::type
        > traits;
        ::boost::type_erasure::detail::storage result;
        typename traits::result_type tmp =
            PrimitiveConcept::apply(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_EXTRACT, ~));
        typename ::boost::remove_reference<typename traits::result_type>::type* p =
            ::boost::addressof(tmp);
        result.data = const_cast<void*>(static_cast<const void*>(p));
        return result;
    }
};

#endif

template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct get_vtable_signature<R(BOOST_PP_ENUM_PARAMS(N, T))>
{
    typedef typename ::boost::type_erasure::detail::replace_result_for_vtable<
        R
    >::type type(BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_REPLACE_PARAM, ~));
};

#undef BOOST_TYPE_ERASURE_REPLACE_PARAM
#undef BOOST_TYPE_ERASURE_EXTRACT
#undef N

#endif
