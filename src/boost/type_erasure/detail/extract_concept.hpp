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

#ifndef BOOST_TYPE_ERASURE_DETAIL_EXTRACT_CONCEPT_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_EXTRACT_CONCEPT_HPP_INCLUDED

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/type_erasure/concept_of.hpp>
#include <boost/type_erasure/config.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<class T, class U>
struct combine_concepts;

template<class T>
struct combine_concepts<T, T> { typedef T type; };
template<class T>
struct combine_concepts<T, void> { typedef T type; };
template<class T>
struct combine_concepts<void, T> { typedef T type; };
template<>
struct combine_concepts<void, void> { typedef void type; };

#ifdef BOOST_TYPE_ERASURE_USE_MP11

template<class T, class U>
using combine_concepts_t = typename ::boost::type_erasure::detail::combine_concepts<T, U>::type;

template<class T, class U>
using extract_concept_or_void =
    ::boost::mp11::mp_eval_if_c<
        !::boost::type_erasure::is_placeholder<
            ::boost::remove_cv_t<
                ::boost::remove_reference_t<T>
            >
        >::value,
        void,
        ::boost::type_erasure::concept_of_t, U
    >;

template<class L1, class L2>
using extract_concept_t =
    ::boost::mp11::mp_fold<
        ::boost::mp11::mp_transform< ::boost::type_erasure::detail::extract_concept_or_void, L1, L2>,
        void,
        ::boost::type_erasure::detail::combine_concepts_t
    >;

#else

template<class T, class U>
struct maybe_extract_concept
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::type_erasure::is_placeholder<
            typename ::boost::remove_cv<
                typename ::boost::remove_reference<T>::type
            >::type
        >,
        ::boost::type_erasure::concept_of<typename ::boost::remove_reference<U>::type>,
        ::boost::mpl::identity<void>
    >::type type;
};

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

template<class Args, class... U>
struct extract_concept;

template<class R, class T0, class... T, class U0, class... U>
struct extract_concept<R(T0, T...), U0, U...>
{
    typedef typename ::boost::type_erasure::detail::combine_concepts<
        typename ::boost::type_erasure::detail::maybe_extract_concept<
            T0, U0
        >::type,
        typename ::boost::type_erasure::detail::extract_concept<
            void(T...),
            U...
        >::type
    >::type type;
};

template<>
struct extract_concept<void()>
{
    typedef void type;
};

#else

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/extract_concept.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#endif

#endif

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_EXTRACT_CONCEPT(z, n, data)                  \
    typedef typename ::boost::type_erasure::detail::combine_concepts<   \
        typename ::boost::type_erasure::detail::maybe_extract_concept<  \
            BOOST_PP_CAT(T, n), BOOST_PP_CAT(U, n)                      \
        >::type,                                                        \
        BOOST_PP_CAT(concept, n)                                        \
    >::type BOOST_PP_CAT(concept, BOOST_PP_INC(n));

template<
    BOOST_PP_ENUM_PARAMS(N, class T),
    BOOST_PP_ENUM_PARAMS(N, class U)>
struct BOOST_PP_CAT(extract_concept, N)
{
    typedef void concept0;

    BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_EXTRACT_CONCEPT, ~)

    typedef BOOST_PP_CAT(concept, N) type;
};

#undef BOOST_TYPE_ERASURE_EXTRACT_CONCEPT
#undef N

#endif
