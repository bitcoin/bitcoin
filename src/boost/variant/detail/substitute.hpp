
#if !defined(BOOST_PP_IS_ITERATING)

///// header body

//-----------------------------------------------------------------------------
// boost variant/detail/substitute.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_SUBSTITUTE_HPP
#define BOOST_VARIANT_DETAIL_SUBSTITUTE_HPP

#include <boost/mpl/aux_/config/ctps.hpp>

#include <boost/variant/detail/substitute_fwd.hpp>
#include <boost/variant/variant_fwd.hpp> // for BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES
#include <boost/mpl/aux_/lambda_arity_param.hpp>
#include <boost/mpl/aux_/preprocessor/params.hpp>
#include <boost/mpl/aux_/preprocessor/repeat.hpp>
#include <boost/mpl/int_fwd.hpp>
#include <boost/mpl/limits/arity.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/iterate.hpp>

namespace boost {
namespace detail { namespace variant {

#if !defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE)

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction substitute
//
// Substitutes one type for another in the given type expression.
//

//
// primary template
//
template <
      typename T, typename Dest, typename Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(
          typename Arity /* = ... (see substitute_fwd.hpp) */
        )
    >
struct substitute
{
    typedef T type;
};

//
// tag substitution specializations
//

#define BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG(CV_) \
    template <typename Dest, typename Source> \
    struct substitute< \
          CV_ Source \
        , Dest \
        , Source \
          BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>) \
        > \
    { \
        typedef CV_ Dest type; \
    }; \
    /**/

BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG( BOOST_PP_EMPTY() )
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG(const)
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG(volatile)
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG(const volatile)

#undef BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_SUBSTITUTE_TAG

//
// pointer specializations
//
#define BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER(CV_) \
    template <typename T, typename Dest, typename Source> \
    struct substitute< \
          T * CV_ \
        , Dest \
        , Source \
          BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>) \
        > \
    { \
        typedef typename substitute< \
              T, Dest, Source \
            >::type * CV_ type; \
    }; \
    /**/

BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER( BOOST_PP_EMPTY() )
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER(const)
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER(volatile)
BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER(const volatile)

#undef BOOST_VARIANT_AUX_ENABLE_RECURSIVE_IMPL_HANDLE_POINTER

//
// reference specializations
//
template <typename T, typename Dest, typename Source>
struct substitute<
      T&
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>)
    >
{
    typedef typename substitute<
          T, Dest, Source
        >::type & type;
};

//
// template expression (i.e., F<...>) specializations
//

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
template <
      template <typename...> class F
    , typename... Ts
    , typename Dest
    , typename Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(typename Arity)
    >
struct substitute<
      F<Ts...>
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(Arity)
    >
{
    typedef F<typename substitute<
          Ts, Dest, Source
        >::type...> type;
};

//
// function specializations
//
template <
      typename R
    , typename... A
    , typename Dest
    , typename Source
    >
struct substitute<
      R (*)(A...)
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>)
    >
{
private:
    typedef typename substitute< R, Dest, Source >::type r;

public:
    typedef r (*type)(typename substitute<
          A, Dest, Source
        >::type...);
};
#else

#define BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF_IMPL(N) \
    typedef typename substitute< \
          BOOST_PP_CAT(U,N), Dest, Source \
        >::type BOOST_PP_CAT(u,N); \
    /**/

#define BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF(z, N, _) \
    BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF_IMPL( BOOST_PP_INC(N) ) \
    /**/

#define BOOST_PP_ITERATION_LIMITS (0,BOOST_MPL_LIMIT_METAFUNCTION_ARITY)
#define BOOST_PP_FILENAME_1 <boost/variant/detail/substitute.hpp>
#include BOOST_PP_ITERATE()

#undef BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF_IMPL
#undef BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF

#endif // !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
#endif // !defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE)

}} // namespace detail::variant
} // namespace boost

#endif // BOOST_VARIANT_DETAIL_SUBSTITUTE_HPP

///// iteration, depth == 1

#elif BOOST_PP_ITERATION_DEPTH() == 1
#define i BOOST_PP_FRAME_ITERATION(1)

#if i > 0

//
// template specializations
//
template <
      template < BOOST_MPL_PP_PARAMS(i,typename P) > class T
    , BOOST_MPL_PP_PARAMS(i,typename U)
    , typename Dest
    , typename Source
    >
struct substitute<
      T< BOOST_MPL_PP_PARAMS(i,U) >
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<( i )>)
    >
{
private:
    BOOST_MPL_PP_REPEAT(i, BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF, _)

public:
    typedef T< BOOST_MPL_PP_PARAMS(i,u) > type;
};

//
// function specializations
//
template <
      typename R
    , BOOST_MPL_PP_PARAMS(i,typename U)
    , typename Dest
    , typename Source
    >
struct substitute<
      R (*)( BOOST_MPL_PP_PARAMS(i,U) )
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>)
    >
{
private:
    typedef typename substitute< R, Dest, Source >::type r;
    BOOST_MPL_PP_REPEAT(i, BOOST_VARIANT_AUX_SUBSTITUTE_TYPEDEF, _)

public:
    typedef r (*type)( BOOST_MPL_PP_PARAMS(i,u) );
};

#elif i == 0

//
// zero-arg function specialization
//
template <
      typename R, typename Dest, typename Source
    >
struct substitute<
      R (*)( void )
    , Dest
    , Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(mpl::int_<-1>)
    >
{
private:
    typedef typename substitute< R, Dest, Source >::type r;

public:
    typedef r (*type)( void );
};

#endif // i

#undef i
#endif // BOOST_PP_IS_ITERATING
