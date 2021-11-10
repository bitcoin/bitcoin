// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_BINARY_SEQ_TO_ARGS_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_BINARY_SEQ_TO_ARGS_HPP

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/cat.hpp>

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0_1(n, prefix_seq)          \
    BOOST_PP_CAT(BOOST_PP_SEQ_ELEM(0, prefix_seq), n) const&
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1_1(n, prefix_seq)          \
    BOOST_PP_CAT(BOOST_PP_SEQ_ELEM(0, prefix_seq), n)&
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0_2(n, prefix_seq)          \
    BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0_1(n, prefix_seq)              \
    BOOST_PP_CAT(BOOST_PP_SEQ_ELEM(1, prefix_seq), n)
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1_2(n, prefix_seq)          \
    BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1_1(n, prefix_seq)              \
    BOOST_PP_CAT(BOOST_PP_SEQ_ELEM(1, prefix_seq), n)
/**/

#include <boost/preprocessor/seq/size.hpp>

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0(prefix_seq)               \
    BOOST_PP_CAT(                                                            \
        BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0_                          \
      , BOOST_PP_SEQ_SIZE(prefix_seq)                                        \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1(prefix_seq)               \
    BOOST_PP_CAT(                                                            \
        BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1_                          \
      , BOOST_PP_SEQ_SIZE(prefix_seq)                                        \
    )
/**/

#include <boost/parameter/aux_/preprocessor/convert_binary_seq.hpp>

// This macro converts the specified Boost.Preprocessor sequence of 1s and 0s
// into a formal function parameter list.
//
// Example:
// BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARGS((1)(0)(1)(0), (P)(p))
// expands to
// P0 & p0, P1 const& p1, P2 & p2, P3 const& p3
#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARGS(binary_seq, prefix_seq)    \
    BOOST_PARAMETER_AUX_PP_CONVERT_BINARY_SEQ(                               \
        binary_seq                                                           \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_0(prefix_seq)               \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_TO_ARG_1(prefix_seq)               \
      , prefix_seq                                                           \
    )
/**/

#endif  // include guard

