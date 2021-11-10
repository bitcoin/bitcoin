// Copyright Daniel Wallin 2006.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_DISPATCH_LAYER_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_DISPATCH_LAYER_HPP

#include <boost/preprocessor/cat.hpp>

// Expands to keyword_tag_type for some keyword_tag.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_TYPE(keyword_tag)              \
    BOOST_PP_CAT(keyword_tag, _type)
/**/

// Expands to a template parameter for each dispatch function.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_TEMPLATE_ARG(r, macro, arg)        \
  , typename BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_TYPE(macro(arg))
/**/

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

// Expands to a forwarding parameter for a dispatch function.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_DEFN(r, macro, arg)            \
  , BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_TYPE(macro(arg))&& macro(arg)
/**/

#include <utility>

// Expands to an argument passed from one dispatch function to the next.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_FWD(r, macro, arg)             \
  , ::std::forward<                                                          \
        BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_TYPE(macro(arg))               \
    >(macro(arg))
/**/

#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

// Expands to a forwarding parameter for a dispatch function.  The parameter
// type stores its const-ness.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_DEFN(r, macro, arg)            \
  , BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_TYPE(macro(arg))& macro(arg)
/**/

#include <boost/parameter/aux_/as_lvalue.hpp>

// Expands to an argument passed from one dispatch function to the next.
// Explicit forwarding takes the form of forcing the argument to be an lvalue.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_FWD(r, macro, arg)             \
  , ::boost::parameter::aux::as_lvalue(macro(arg))
/**/

#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING

#include <boost/parameter/aux_/preprocessor/impl/argument_specs.hpp>
#include <boost/parameter/aux_/preprocessor/impl/split_args.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/first_n.hpp>

// Iterates through all required arguments and the first n optional arguments,
// passing each argument to the specified macro.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(macro, n, split_args)   \
    BOOST_PP_SEQ_FOR_EACH(                                                   \
        macro                                                                \
      , BOOST_PARAMETER_FN_ARG_NAME                                          \
      , BOOST_PARAMETER_SPLIT_ARG_REQ_SEQ(split_args)                        \
    )                                                                        \
    BOOST_PP_SEQ_FOR_EACH(                                                   \
        macro                                                                \
      , BOOST_PARAMETER_FN_ARG_NAME                                          \
      , BOOST_PP_SEQ_FIRST_N(                                                \
            n, BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(split_args)                 \
        )                                                                    \
    )
/**/

#include <boost/parameter/aux_/preprocessor/impl/function_dispatch_tuple.hpp>
#include <boost/parameter/aux_/preprocessor/impl/function_name.hpp>
#include <boost/preprocessor/control/if.hpp>

// Produces a name for the dispatch functions.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, n)                         \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(                                                     \
                BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x)                \
              , boost_param_dispatch_const_                                  \
              , boost_param_dispatch_                                        \
            )                                                                \
          , BOOST_PP_CAT(BOOST_PP_CAT(n, boost_), __LINE__)                  \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(                                \
            BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                   \
        )                                                                    \
    )
/**/

// Expands to the template parameter list of the dispatch function with all
// required and first n optional parameters; also extracts the static keyword
// if present.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                     \
    template <                                                               \
        typename ResultType, typename Args                                   \
        BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(                        \
            BOOST_PARAMETER_FUNCTION_DISPATCH_TEMPLATE_ARG                   \
          , n                                                                \
          , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
    > BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(                                \
        BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                       \
    )
/**/

#include <boost/parameter/aux_/use_default_tag.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

// Expands to the result type, name, parenthesized list of all required and
// n optional parameters, and const-ness of the dispatch function; the bit
// value b determines whether or not this dispatch function takes in
// boost::parameter::aux::use_default_tag as its last parameter.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, b1, b2)             \
    ResultType BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, b1)(                \
        ResultType(*)(), Args const& args, long                              \
        BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(                        \
            BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_DEFN                       \
          , n                                                                \
          , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
        BOOST_PP_COMMA_IF(b2)                                                \
        BOOST_PP_EXPR_IF(b2, ::boost::parameter::aux::use_default_tag)       \
    ) BOOST_PP_EXPR_IF(BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x), const)
/**/

// Expands to a forward declaration of the dispatch function that takes in
// all required and the first n optional parameters, but not
// boost::parameter::aux::use_default_tag.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_FWD_DECL_0_Z(z, n, x)              \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                         \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, 0, 0);
/**/

// Expands to a forward declaration of the dispatch function that takes in
// all required parameters, the first n optional parameters, and
// boost::parameter::aux::use_default_tag.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_FWD_DECL_1_Z(z, n, x)              \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                         \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, 0, 1);
/**/

#include <boost/preprocessor/seq/elem.hpp>

// Expands to the default value of the (n + 1)th optional parameter.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_DEFAULT_AUX(n, s_args)             \
    BOOST_PARAMETER_FN_ARG_DEFAULT(                                          \
        BOOST_PP_SEQ_ELEM(n, BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(s_args))      \
    )
/**/

#include <boost/parameter/keyword.hpp>

// Expands to the assignment portion which binds the default value to the
// (n + 1)th optional parameter before composing it with the argument-pack
// parameter passed in to the n-th dispatch function.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_DEFAULT(n, s_args, tag_ns)         \
    ::boost::parameter::keyword<                                             \
        tag_ns::BOOST_PARAMETER_FN_ARG_NAME(                                 \
            BOOST_PP_SEQ_ELEM(n, BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(s_args))  \
        )                                                                    \
    >::instance = BOOST_PARAMETER_FUNCTION_DISPATCH_DEFAULT_AUX(n, s_args)
/**/

#include <boost/parameter/aux_/preprocessor/impl/function_cast.hpp>

// Takes in the arg tuple (name, pred) and the tag namespace.
// Extracts the corresponding required argument from the pack.
// This form enables BOOST_PARAMETER_FUNCTION_DISPATCH_LAYER to use it
// from within BOOST_PP_SEQ_FOR_EACH.
#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
// The boost::parameter::aux::forward wrapper is necessary to transmit the
// target type to the next dispatch function.  Otherwise, the argument will
// retain its original type. -- Cromwell D. Enage
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_CAST_R(r, tag_ns, arg)         \
  , ::boost::parameter::aux::forward<                                        \
        BOOST_PARAMETER_FUNCTION_CAST_T(                                     \
            tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                         \
          , BOOST_PARAMETER_FN_ARG_PRED(arg)                                 \
          , Args                                                             \
        )                                                                    \
      , BOOST_PARAMETER_FUNCTION_CAST_B(                                     \
            tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                         \
          , BOOST_PARAMETER_FN_ARG_PRED(arg)                                 \
          , Args                                                             \
        )                                                                    \
    >(                                                                       \
        args[                                                                \
            ::boost::parameter::keyword<                                     \
                tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                     \
            >::instance                                                      \
        ]                                                                    \
    )
/**/
#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
// The explicit type cast is necessary to transmit the target type to the next
// dispatch function.  Otherwise, the argument will retain its original type.
// -- Cromwell D. Enage
#define BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_CAST_R(r, tag_ns, arg)         \
  , BOOST_PARAMETER_FUNCTION_CAST_T(                                         \
        tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                             \
      , BOOST_PARAMETER_FN_ARG_PRED(arg)                                     \
      , Args                                                                 \
    )(                                                                       \
        args[                                                                \
            ::boost::parameter::keyword<                                     \
                tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                     \
            >::instance                                                      \
        ]                                                                    \
    )
/**/
#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING

// Takes in the arg tuple (name, pred, default) and the tag namespace.
// Extracts the corresponding optional argument from the pack if specified,
// otherwise temporarily passes use_default_tag() to the dispatch functions.
#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
// The boost::parameter::aux::forward wrapper is necessary to transmit the
// target type to the next dispatch function.  Otherwise, the argument will
// retain its original type. -- Cromwell D. Enage
#define BOOST_PARAMETER_FUNCTION_DISPATCH_OPT_ARG_CAST(arg, tag_ns)          \
    ::boost::parameter::aux::forward<                                        \
        BOOST_PARAMETER_FUNCTION_CAST_T(                                     \
            tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                         \
          , BOOST_PARAMETER_FN_ARG_PRED(arg)                                 \
          , Args                                                             \
        )                                                                    \
      , BOOST_PARAMETER_FUNCTION_CAST_B(                                     \
            tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                         \
          , BOOST_PARAMETER_FN_ARG_PRED(arg)                                 \
          , Args                                                             \
        )                                                                    \
    >(                                                                       \
        args[                                                                \
            ::boost::parameter::keyword<                                     \
                tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                     \
            >::instance || ::boost::parameter::aux::use_default_tag()        \
        ]                                                                    \
    )
/**/
#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
#define BOOST_PARAMETER_FUNCTION_DISPATCH_OPT_ARG_CAST(arg, tag_ns)          \
    BOOST_PARAMETER_FUNCTION_CAST_B(                                         \
        args[                                                                \
            ::boost::parameter::keyword<                                     \
                tag_ns::BOOST_PARAMETER_FN_ARG_NAME(arg)                     \
            >::instance || ::boost::parameter::aux::use_default_tag()        \
        ]                                                                    \
      , BOOST_PARAMETER_FN_ARG_PRED(arg)                                     \
      , Args                                                                 \
    )
/**/
#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING

#include <boost/parameter/aux_/preprocessor/nullptr.hpp>

// Expands to three dispatch functions that take in all required parameters
// and the first n optional parameters.  The third dispatch function bears
// the same name as the first but takes in use_default_tag as the last
// parameter.  The second dispatch function bears a different name from the
// other two.
//
// x is a tuple:
//
//   (name, split_args, is_const, tag_namespace)
//
// Where name is the base name of the functions, and split_args is a tuple:
//
//   (required_count, required_args, optional_count, required_args)
//
// The first dispatch function queries args if it has bound the (n + 1)th
// optional parameter to a user-defined argument.  If so, then it forwards
// its own arguments followed by the user-defined argument to the dispatch
// function that takes in all required parameters and the first (n + 1)
// optional parameters, but not use_default_tag.   Otherwise, it forwards
// its own arguments to the third dispatch function.
//
// The third dispatch function appends the default value of the (n + 1)th
// optional parameter to its copy of args.  Then it forwards this copy, all
// required parameters, and the first n (not n + 1) optional parameters to
// the second dispatch function.
//
// The second dispatch function forwards its arguments, then the (n + 1)th
// optional parameter that it extracts from args, to the other-named dispatch
// function that takes in all required parameters and the first (n + 1)
// optional parameters, but not use_default_tag.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_OVERLOAD_Z(z, n, x)                \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                         \
    inline BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, 0, 0)            \
    {                                                                        \
        return BOOST_PP_EXPR_IF(                                             \
            BOOST_PARAMETER_FUNCTION_DISPATCH_IS_MEMBER(x)                   \
          , this->                                                           \
        ) BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, 0)(                      \
            static_cast<ResultType(*)()>(BOOST_PARAMETER_AUX_PP_NULLPTR)     \
          , args                                                             \
          , 0L                                                               \
            BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(                    \
                BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_FWD                    \
              , n                                                            \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)              \
            )                                                                \
          , BOOST_PARAMETER_FUNCTION_DISPATCH_OPT_ARG_CAST(                  \
                BOOST_PP_SEQ_ELEM(                                           \
                    n                                                        \
                  , BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(                       \
                        BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)      \
                    )                                                        \
                )                                                            \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_TAG_NAMESPACE(x)           \
            )                                                                \
        );                                                                   \
    }                                                                        \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                         \
    inline BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, 1, 0)            \
    {                                                                        \
        return BOOST_PP_EXPR_IF(                                             \
            BOOST_PARAMETER_FUNCTION_DISPATCH_IS_MEMBER(x)                   \
          , this->                                                           \
        ) BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, 0)(                      \
            static_cast<ResultType(*)()>(BOOST_PARAMETER_AUX_PP_NULLPTR)     \
          , args                                                             \
          , 0L                                                               \
            BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(                    \
                BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_FWD                    \
              , n                                                            \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)              \
            )                                                                \
          , args[                                                            \
                ::boost::parameter::keyword<                                 \
                    BOOST_PARAMETER_FUNCTION_DISPATCH_TAG_NAMESPACE(x)::     \
                    BOOST_PARAMETER_FN_ARG_NAME(                             \
                        BOOST_PP_SEQ_ELEM(                                   \
                            n                                                \
                          , BOOST_PARAMETER_SPLIT_ARG_OPT_SEQ(               \
                            BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)  \
                            )                                                \
                        )                                                    \
                    )                                                        \
                >::instance                                                  \
            ]                                                                \
        );                                                                   \
    }                                                                        \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(n, x)                         \
    inline BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(n, x, 0, 1)            \
    {                                                                        \
        return BOOST_PP_EXPR_IF(                                             \
            BOOST_PARAMETER_FUNCTION_DISPATCH_IS_MEMBER(x)                   \
          , this->                                                           \
        ) BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, 1)(                      \
            static_cast<ResultType(*)()>(BOOST_PARAMETER_AUX_PP_NULLPTR)     \
          , (args                                                            \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_DEFAULT(                   \
                    n                                                        \
                  , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)          \
                  , BOOST_PARAMETER_FUNCTION_DISPATCH_TAG_NAMESPACE(x)       \
                )                                                            \
            )                                                                \
          , 0L                                                               \
            BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_REPEAT(                    \
                BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_FWD                    \
              , n                                                            \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)              \
            )                                                                \
        );                                                                   \
    }
/**/

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

// x is a tuple:
//
//   (base_name, split_args, is_member, is_const, tag_namespace)
//
// Generates all dispatch functions for the function named base_name.  Each
// dispatch function that takes in n optional parameters passes the default
// value of the (n + 1)th optional parameter to the next dispatch function.
// The last dispatch function is the back-end implementation, so only the
// header is generated: the user is expected to supply the body.
//
// Also generates the front-end implementation function, which uses
// BOOST_PARAMETER_FUNCTION_CAST to extract each argument from the argument
// pack.
#define BOOST_PARAMETER_FUNCTION_DISPATCH_LAYER(fwd_decl, x)                 \
    BOOST_PP_IF(fwd_decl, BOOST_PP_REPEAT_FROM_TO, BOOST_PP_TUPLE_EAT(4))(   \
        0                                                                    \
      , BOOST_PP_INC(                                                        \
            BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(                             \
                BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)              \
            )                                                                \
        )                                                                    \
      , BOOST_PARAMETER_FUNCTION_DISPATCH_FWD_DECL_0_Z                       \
      , x                                                                    \
    )                                                                        \
    BOOST_PP_IF(fwd_decl, BOOST_PP_REPEAT_FROM_TO, BOOST_PP_TUPLE_EAT(4))(   \
        0                                                                    \
      , BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(                                 \
            BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
      , BOOST_PARAMETER_FUNCTION_DISPATCH_FWD_DECL_1_Z                       \
      , x                                                                    \
    )                                                                        \
    template <typename Args> BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(         \
        BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                       \
    ) inline typename BOOST_PARAMETER_FUNCTION_RESULT_NAME(                  \
        BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                       \
      , BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x)                        \
    )<Args>::type BOOST_PARAMETER_FUNCTION_IMPL_NAME(                        \
        BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)                       \
      , BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x)                        \
    )(Args const& args)                                                      \
    BOOST_PP_EXPR_IF(BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x), const)   \
    {                                                                        \
        return BOOST_PP_EXPR_IF(                                             \
            BOOST_PARAMETER_FUNCTION_DISPATCH_IS_MEMBER(x)                   \
          , this->                                                           \
        ) BOOST_PARAMETER_FUNCTION_DISPATCH_NAME(x, 0)(                      \
            static_cast<                                                     \
                typename BOOST_PARAMETER_FUNCTION_RESULT_NAME(               \
                    BOOST_PARAMETER_FUNCTION_DISPATCH_BASE_NAME(x)           \
                  , BOOST_PARAMETER_FUNCTION_DISPATCH_IS_CONST(x)            \
                )<Args>::type(*)()                                           \
            >(BOOST_PARAMETER_AUX_PP_NULLPTR)                                \
          , args                                                             \
          , 0L                                                               \
            BOOST_PP_SEQ_FOR_EACH(                                           \
                BOOST_PARAMETER_FUNCTION_DISPATCH_ARG_CAST_R                 \
              , BOOST_PARAMETER_FUNCTION_DISPATCH_TAG_NAMESPACE(x)           \
              , BOOST_PARAMETER_SPLIT_ARG_REQ_SEQ(                           \
                    BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)          \
                )                                                            \
            )                                                                \
        );                                                                   \
    }                                                                        \
    BOOST_PP_REPEAT_FROM_TO(                                                 \
        0                                                                    \
      , BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(                                 \
            BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
      , BOOST_PARAMETER_FUNCTION_DISPATCH_OVERLOAD_Z                         \
      , x                                                                    \
    )                                                                        \
    BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_TPL(                              \
        BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(                                 \
            BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
      , x                                                                    \
    )                                                                        \
    inline BOOST_PARAMETER_FUNCTION_DISPATCH_HEAD_PRN(                       \
        BOOST_PARAMETER_SPLIT_ARG_OPT_COUNT(                                 \
            BOOST_PARAMETER_FUNCTION_DISPATCH_SPLIT_ARGS(x)                  \
        )                                                                    \
      , x                                                                    \
      , 0                                                                    \
      , 0                                                                    \
    )
/**/

#endif  // include guard

