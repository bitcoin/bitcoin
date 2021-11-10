// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_SPLIT_ARGS_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_SPLIT_ARGS_HPP

#include <boost/preprocessor/tuple/elem.hpp>

// Accessor macros for the split_args tuple.
#define BOOST_PARAMETER_SPLIT_ARG_REQ_COUNT(x) BOOST_PP_TUPLE_ELEM(4, 0, x)
#define BOOST_PARAMETER_SPLIT_ARG_REQ_SEQ(x) BOOST_PP_TUPLE_ELEM(4, 1, x)
#define BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(x) BOOST_PP_TUPLE_ELEM(4, 2, x)
#define BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(x) BOOST_PP_TUPLE_ELEM(4, 3, x)

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/seq/push_back.hpp>

// Helper macros for BOOST_PARAMETER_FUNCTION_SPLIT_ARGS.
#define BOOST_PARAMETER_FUNCTION_SPLIT_ARG_required(s_a, arg)                \
    (                                                                        \
        BOOST_PP_INC(BOOST_PARAMETER_SPLIT_ARG_REQ_COUNT(s_a))               \
      , BOOST_PP_SEQ_PUSH_BACK(BOOST_PARAMETER_SPLIT_ARG_REQ_SEQ(s_a), arg)  \
      , BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(s_a)                             \
      , BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(s_a)                               \
    )
/**/

#define BOOST_PARAMETER_FUNCTION_SPLIT_ARG_deduced_required(split_args, arg) \
    BOOST_PARAMETER_FUNCTION_SPLIT_ARG_required(split_args, arg)
/**/

#define BOOST_PARAMETER_FUNCTION_SPLIT_ARG_optional(s_a, arg)                \
    (                                                                        \
        BOOST_PARAMETER_SPLIT_ARG_REQ_COUNT(s_a)                             \
      , BOOST_PARAMETER_SPLIT_ARG_REQ_SEQ(s_a)                               \
      , BOOST_PP_INC(BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(s_a))               \
      , BOOST_PP_SEQ_PUSH_BACK(BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(s_a), arg)  \
    )
/**/

#define BOOST_PARAMETER_FUNCTION_SPLIT_ARG_deduced_optional(split_args, arg) \
    BOOST_PARAMETER_FUNCTION_SPLIT_ARG_optional(split_args, arg)
/**/

#include <boost/parameter/aux_/preprocessor/impl/argument_specs.hpp>
#include <boost/preprocessor/cat.hpp>

#define BOOST_PARAMETER_FUNCTION_SPLIT_ARG(s, split_args, arg)               \
    BOOST_PP_CAT(                                                            \
        BOOST_PARAMETER_FUNCTION_SPLIT_ARG_                                  \
      , BOOST_PARAMETER_FN_ARG_QUALIFIER(arg)                                \
    )(split_args, arg)
/**/

#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/seq.hpp>

// Expands from the flattened BOOST_PARAMETER_FUNCTION et. al. arg sequence to
// the tuple (required_count, required_args, optional_count, optional_args).
#define BOOST_PARAMETER_FUNCTION_SPLIT_ARGS(args)                            \
    BOOST_PP_SEQ_FOLD_LEFT(                                                  \
        BOOST_PARAMETER_FUNCTION_SPLIT_ARG                                   \
      , (0, BOOST_PP_SEQ_NIL, 0, BOOST_PP_SEQ_NIL)                           \
      , args                                                                 \
    )
/**/

#endif  // include guard

