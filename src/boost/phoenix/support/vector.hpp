#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4510) // default constructor could not be generated
#pragma warning(disable: 4610) // can never be instantiated - user defined constructor required
#endif
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)

#ifndef BOOST_PHOENIX_SUPPORT_VECTOR_HPP
#define BOOST_PHOENIX_SUPPORT_VECTOR_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>

#define BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL BOOST_FUSION_ADAPT_TPL_STRUCT

#include <boost/phoenix/support/preprocessed/vector.hpp>

#endif

#else

#if !BOOST_PHOENIX_IS_ITERATING

#ifndef BOOST_PHOENIX_SUPPORT_VECTOR_HPP
#define BOOST_PHOENIX_SUPPORT_VECTOR_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>

#define BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL BOOST_FUSION_ADAPT_TPL_STRUCT

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/vector_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif
/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

#define M0(Z, N, D)                                                             \
    typedef BOOST_PP_CAT(A, N) BOOST_PP_CAT(member_type, N);                    \
    BOOST_PP_CAT(A, N) BOOST_PP_CAT(a, N);                                      \
/**/
#define M1(Z, N, D)                                                             \
    (BOOST_PP_CAT(A, N))                                                        \
/**/
#define M2(Z, N, D)                                                             \
    (BOOST_PP_CAT(T, N))                                                        \
/**/
#define M3(Z, N, D)                                                             \
    (BOOST_PP_CAT(A, N), BOOST_PP_CAT(a, N))                                    \
/**/

namespace boost { namespace phoenix
{
    template <typename Dummy = void>
    struct vector0
    {
        typedef mpl::int_<0> size_type;
        static const int size_value = 0;
    };

    template <int> struct vector_chooser;

    template <>
    struct vector_chooser<0>
    {
        template <typename Dummy = void>
        struct apply
        {
            typedef vector0<> type;
        };
    };
}}


#define BOOST_PHOENIX_ITERATION_PARAMS                                          \
        (3, (1, BOOST_PP_INC(BOOST_PHOENIX_LIMIT),                              \
        <boost/phoenix/support/vector.hpp>))
#include BOOST_PHOENIX_ITERATE()

#undef M0
#undef M1
#undef M2
#undef M3

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif

#else

namespace boost { namespace phoenix
{
    template <BOOST_PHOENIX_typename_A>
    struct BOOST_PP_CAT(vector, BOOST_PHOENIX_ITERATION)
    {
        BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M0, _)
        
        typedef mpl::int_<BOOST_PHOENIX_ITERATION> size_type;
        static const int size_value = BOOST_PHOENIX_ITERATION;

        typedef
            BOOST_PP_CAT(vector, BOOST_PP_DEC(BOOST_PHOENIX_ITERATION))<BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PHOENIX_ITERATION, A)>
            args_type;

        args_type args() const
        {
            args_type r = {BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PHOENIX_ITERATION, a)};
            return r;
        }
    };

    template <>
    struct vector_chooser<BOOST_PHOENIX_ITERATION>
    {
        template <BOOST_PHOENIX_typename_A>
        struct apply
        {
            typedef BOOST_PP_CAT(vector, BOOST_PHOENIX_ITERATION)<BOOST_PHOENIX_A> type;
        };
    };
}}

#define BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM0                                \
    BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M1, _)                             \
/**/
#define BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM1                                \
   (                                                                            \
        BOOST_PP_CAT(                                                           \
            boost::phoenix::vector                                              \
          , BOOST_PHOENIX_ITERATION                                             \
        )                                                                       \
    )                                                                           \
    BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M1, _)                             \
/**/
#define BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM2                                \
    BOOST_PP_REPEAT(BOOST_PHOENIX_ITERATION, M3, _)                             \
/**/

BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM0
  , BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM1
  , BOOST_PHOENIX_SUPPORT_VECTOR_ADAT_PARAM2
)

#endif

#endif

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
