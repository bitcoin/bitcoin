
#if !defined(BOOST_PP_IS_ITERATING)

///// header body

#ifndef BOOST_MPL_UNPACK_ARGS_HPP_INCLUDED
#define BOOST_MPL_UNPACK_ARGS_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#if !defined(BOOST_MPL_PREPROCESSING_MODE)
#   include <boost/mpl/apply.hpp>
#   include <boost/mpl/at.hpp>
#   include <boost/mpl/size.hpp>
#   include <boost/mpl/aux_/nttp_decl.hpp>
#   include <boost/mpl/aux_/lambda_spec.hpp>
#endif

#include <boost/mpl/aux_/config/use_preprocessed.hpp>

#if !defined(BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS) \
    && !defined(BOOST_MPL_PREPROCESSING_MODE)

#   define BOOST_MPL_PREPROCESSED_HEADER unpack_args.hpp
#   include <boost/mpl/aux_/include_preprocessed.hpp>

#else

#   include <boost/mpl/limits/arity.hpp>
#   include <boost/mpl/aux_/preprocessor/repeat.hpp>
#   include <boost/mpl/aux_/config/ctps.hpp>
#   include <boost/mpl/aux_/config/forwarding.hpp>
#   include <boost/preprocessor/iterate.hpp>
#   include <boost/preprocessor/cat.hpp>


namespace boost { namespace mpl {

// local macros, #undef-ined at the end of the header

#   define AUX778076_UNPACK(unused, i, Args) \
    , typename at_c<Args,i>::type \
    /**/

#   define AUX778076_UNPACKED_ARGS(n, Args) \
    BOOST_MPL_PP_REPEAT(n, AUX778076_UNPACK, Args) \
    /**/

namespace aux {

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
template< int size, typename F, typename Args >
struct unpack_args_impl;
#else
template< BOOST_MPL_AUX_NTTP_DECL(int, size) > struct unpack_args_impl
{
    template< typename F, typename Args > struct apply;
};
#endif

#define BOOST_PP_ITERATION_PARAMS_1 \
    (3,(0, BOOST_MPL_LIMIT_METAFUNCTION_ARITY, <boost/mpl/unpack_args.hpp>))
#include BOOST_PP_ITERATE()

}

template<
      typename F
    >
struct unpack_args
{
    template< typename Args > struct apply
#if !defined(BOOST_MPL_CFG_NO_NESTED_FORWARDING)
#   if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
        : aux::unpack_args_impl< size<Args>::value,F,Args >
#   else
        : aux::unpack_args_impl< size<Args>::value >
            ::template apply< F,Args >
#   endif
    {
#else // BOOST_MPL_CFG_NO_NESTED_FORWARDING
    {
        typedef typename aux::unpack_args_impl< 
              size<Args>::value
            , F
            , Args
            >::type type;
#endif
    };
};

BOOST_MPL_AUX_PASS_THROUGH_LAMBDA_SPEC(1, unpack_args)

#   undef AUX778076_UNPACK
#   undef AUX778076_UNPACKED_ARGS

}}

#endif // BOOST_MPL_CFG_USE_PREPROCESSED_HEADERS
#endif // BOOST_MPL_UNPACK_ARGS_HPP_INCLUDED

///// iteration, depth == 1

// For gcc 4.4 compatability, we must include the
// BOOST_PP_ITERATION_DEPTH test inside an #else clause.
#else // BOOST_PP_IS_ITERATING
#if BOOST_PP_ITERATION_DEPTH() == 1

#   define i_ BOOST_PP_FRAME_ITERATION(1)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

template< typename F, typename Args >
struct unpack_args_impl<i_,F,Args>
    : BOOST_PP_CAT(apply,i_)<
          F
        AUX778076_UNPACKED_ARGS(i_, Args)
        >
{
};

#else

template<> struct unpack_args_impl<i_>
{
    template< typename F, typename Args > struct apply
        : BOOST_PP_CAT(apply,i_)<
              F
            AUX778076_UNPACKED_ARGS(i_, Args)
            >
    {
    };
};

#endif

#   undef i_

#endif // BOOST_PP_ITERATION_DEPTH()
#endif // BOOST_PP_IS_ITERATING
