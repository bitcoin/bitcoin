/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#if !defined(FUSION_TUPLE_EXPAND_10032005_0815)
#define FUSION_TUPLE_EXPAND_10032005_0815

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 \
    <boost/fusion/tuple/detail/tuple_expand.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, FUSION_MAX_VECTOR_SIZE)
#include BOOST_PP_ITERATE()

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

    BOOST_FUSION_GPU_ENABLED
#if N == 1
    explicit
#endif
    tuple(BOOST_PP_ENUM_BINARY_PARAMS(
        N, typename detail::call_param<T, >::type arg))
        : base_type(BOOST_PP_ENUM_PARAMS(N, arg)) {}

    template <BOOST_PP_ENUM_PARAMS(N, typename U)>
    BOOST_FUSION_GPU_ENABLED
    tuple(tuple<BOOST_PP_ENUM_PARAMS(N, U)> const& rhs)
        : base_type(rhs) {}

    template <BOOST_PP_ENUM_PARAMS(N, typename U)>
    BOOST_FUSION_GPU_ENABLED
    tuple& operator=(tuple<BOOST_PP_ENUM_PARAMS(N, U)> const& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

