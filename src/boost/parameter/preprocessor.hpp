// Copyright Daniel Wallin 2006.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_PREPROCESSOR_060206_HPP
#define BOOST_PARAMETER_PREPROCESSOR_060206_HPP

#include <boost/parameter/aux_/preprocessor/impl/forwarding_overloads.hpp>
#include <boost/parameter/aux_/preprocessor/impl/specification.hpp>
#include <boost/preprocessor/cat.hpp>

// Helper macro for BOOST_PARAMETER_CONSTRUCTOR.
#define BOOST_PARAMETER_CONSTRUCTOR_AUX(class_, base, tag_namespace, args)   \
    BOOST_PARAMETER_SPECIFICATION(tag_namespace, ctor, args, 0)              \
        BOOST_PP_CAT(constructor_parameters, __LINE__);                      \
    BOOST_PARAMETER_CONSTRUCTOR_OVERLOADS(class_, base, args)
/**/

#include <boost/parameter/aux_/preprocessor/impl/function_name.hpp>

// Defines the implementation function header.
#define BOOST_PARAMETER_FUNCTION_IMPL_HEAD(name, is_const)                   \
    template <typename Args>                                                 \
    typename BOOST_PARAMETER_FUNCTION_RESULT_NAME(name, is_const)<           \
        Args                                                                 \
    >::type BOOST_PARAMETER_FUNCTION_IMPL_NAME(name, is_const)(              \
        Args const& args                                                     \
    )
/**/

#include <boost/parameter/aux_/preprocessor/impl/parenthesized_return_type.hpp>
#include <boost/parameter/config.hpp>

// Expands to the result metafunction and the parameters specialization.
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_FUNCTION_HEAD(result, name, tag_ns, args, is_const)  \
    template <typename Args>                                                 \
    using BOOST_PARAMETER_FUNCTION_RESULT_NAME(name, is_const)               \
    = typename BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(result);            \
    BOOST_PARAMETER_SPECIFICATION(tag_ns, name, args, is_const)              \
        BOOST_PARAMETER_FUNCTION_SPECIFICATION_NAME(name, is_const);
/**/
#else
#define BOOST_PARAMETER_FUNCTION_HEAD(result, name, tag_ns, args, is_const)  \
    template <typename Args>                                                 \
    struct BOOST_PARAMETER_FUNCTION_RESULT_NAME(name, is_const)              \
      : BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(result)                    \
    {                                                                        \
    };                                                                       \
    BOOST_PARAMETER_SPECIFICATION(tag_ns, name, args, is_const)              \
        BOOST_PARAMETER_FUNCTION_SPECIFICATION_NAME(name, is_const);
/**/
#endif  // BOOST_PARAMETER_CAN_USE_MP11

// Helper macro for BOOST_PARAMETER_BASIC_FUNCTION.
#define BOOST_PARAMETER_BASIC_FUNCTION_AUX(result, name, tag_ns, args)       \
    BOOST_PARAMETER_FUNCTION_HEAD(result, name, tag_ns, args, 0)             \
    BOOST_PARAMETER_FUNCTION_IMPL_HEAD(name, 0);                             \
    BOOST_PARAMETER_FUNCTION_FORWARD_OVERLOADS(name, name, args, 0, 0)       \
    BOOST_PARAMETER_FUNCTION_IMPL_HEAD(name, 0)
/**/

#include <boost/preprocessor/control/expr_if.hpp>

// Helper macro for BOOST_PARAMETER_BASIC_MEMBER_FUNCTION,
// BOOST_PARAMETER_BASIC_CONST_MEMBER_FUNCTION,
// BOOST_PARAMETER_BASIC_FUNCTION_CALL_OPERATOR, and
// BOOST_PARAMETER_BASIC_CONST_FUNCTION_CALL_OPERATOR.
#define BOOST_PARAMETER_BASIC_MEMBER_FUNCTION_AUX(r, n, i, tag_ns, args, c)  \
    BOOST_PARAMETER_FUNCTION_HEAD(r, i, tag_ns, args, c)                     \
    BOOST_PARAMETER_FUNCTION_FORWARD_OVERLOADS(n, i, args, 1, c)             \
    BOOST_PARAMETER_FUNCTION_IMPL_HEAD(i, c) BOOST_PP_EXPR_IF(c, const)
/**/

#include <boost/parameter/aux_/preprocessor/impl/flatten.hpp>

// Expands to a Boost.Parameter-enabled constructor header.  All arguments are
// accessible via args and keywords only.
#define BOOST_PARAMETER_CONSTRUCTOR(class_, base, tag_namespace, args)       \
    BOOST_PARAMETER_CONSTRUCTOR_AUX(                                         \
        class_, base, tag_namespace                                          \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args)                        \
    )
/**/

// Expands to a Boost.Parameter-enabled function header.  All arguments are
// accessible via args and keywords only.
#define BOOST_PARAMETER_BASIC_FUNCTION(result, name, tag_namespace, args)    \
    BOOST_PARAMETER_BASIC_FUNCTION_AUX(                                      \
        result, name, tag_namespace                                          \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args)                        \
    )
/**/

// Expands to a Boost.Parameter-enabled member function header.  All arguments
// are accessible via args and keywords only.
#define BOOST_PARAMETER_BASIC_MEMBER_FUNCTION(result, name, tag_ns, args)    \
    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION_AUX(                               \
        result, name, name, tag_ns                                           \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args), 0                     \
    )
/**/

// Expands to a Boost.Parameter-enabled const-qualified member function
// header.  All arguments are accessible via args and keywords only.
#define BOOST_PARAMETER_BASIC_CONST_MEMBER_FUNCTION(r, name, tag_ns, args)   \
    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION_AUX(                               \
        r, name, name, tag_ns                                                \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args), 1                     \
    )
/**/

// Expands to a Boost.Parameter-enabled function call operator header.  All
// arguments are accessible via args and keywords only.
#define BOOST_PARAMETER_BASIC_FUNCTION_CALL_OPERATOR(result, tag_ns, args)   \
    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION_AUX(                               \
        result, operator(), operator, tag_ns                                 \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args), 0                     \
    )
/**/

// Expands to a Boost.Parameter-enabled const-qualified function call
// operator header.  All arguments are accessible via args and keywords only.
#define BOOST_PARAMETER_BASIC_CONST_FUNCTION_CALL_OPERATOR(r, tag_ns, args)  \
    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION_AUX(                               \
        r, operator(), operator, tag_ns                                      \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(2, 2, 3, args), 1                     \
    )
/**/

#include <boost/parameter/aux_/preprocessor/impl/function_dispatch_layer.hpp>

// Helper macro for BOOST_PARAMETER_FUNCTION.
#define BOOST_PARAMETER_FUNCTION_AUX(result, name, tag_ns, args)             \
    BOOST_PARAMETER_FUNCTION_HEAD(result, name, tag_ns, args, 0)             \
    BOOST_PARAMETER_FUNCTION_IMPL_HEAD(name, 0);                             \
    BOOST_PARAMETER_FUNCTION_FORWARD_OVERLOADS(name, name, args, 0, 0)       \
    BOOST_PARAMETER_FUNCTION_DISPATCH_LAYER(                                 \
        1, (name, BOOST_PARAMETER_FUNCTION_SPLIT_ARGS(args), 0, 0, tag_ns)   \
    )
/**/

// Expands to a Boost.Parameter-enabled function header.  All arguments are
// accessible via args and keywords, as well as by name.
#define BOOST_PARAMETER_FUNCTION(result, name, tag_namespace, args)          \
    BOOST_PARAMETER_FUNCTION_AUX(                                            \
        result, name, tag_namespace                                          \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(3, 2, 3, args)                        \
    )
/**/

#include <boost/preprocessor/control/if.hpp>

// Helper macro for BOOST_PARAMETER_MEMBER_FUNCTION
// BOOST_PARAMETER_CONST_MEMBER_FUNCTION,
// BOOST_PARAMETER_FUNCTION_CALL_OPERATOR, and
// BOOST_PARAMETER_CONST_FUNCTION_CALL_OPERATOR.
#define BOOST_PARAMETER_MEMBER_FUNCTION_AUX(r, name, impl, tag_ns, c, args)  \
    BOOST_PARAMETER_FUNCTION_HEAD(r, impl, tag_ns, args, c)                  \
    BOOST_PARAMETER_FUNCTION_FORWARD_OVERLOADS(name, impl, args, 1, c)       \
    BOOST_PARAMETER_FUNCTION_DISPATCH_LAYER(                                 \
        0, (                                                                 \
            impl                                                             \
          , BOOST_PARAMETER_FUNCTION_SPLIT_ARGS(args)                        \
          , BOOST_PP_IF(                                                     \
                BOOST_PARAMETER_MEMBER_FUNCTION_IS_STATIC(impl)              \
              , 0                                                            \
              , 1                                                            \
            )                                                                \
          , c                                                                \
          , tag_ns                                                           \
        )                                                                    \
    )
/**/

// Expands to a Boost.Parameter-enabled member function header.  All
// arguments are accessible via args and keywords, as well as by name.
#define BOOST_PARAMETER_MEMBER_FUNCTION(result, name, tag_ns, args)          \
    BOOST_PARAMETER_MEMBER_FUNCTION_AUX(                                     \
        result, name, name, tag_ns, 0                                        \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(3, 2, 3, args)                        \
    )
/**/

// Expands to a Boost.Parameter-enabled const-qualified member function
// header.  All arguments are accessible via args and keywords, as well as
// by name.
#define BOOST_PARAMETER_CONST_MEMBER_FUNCTION(result, name, tag_ns, args)    \
    BOOST_PARAMETER_MEMBER_FUNCTION_AUX(                                     \
        result, name, name, tag_ns, 1                                        \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(3, 2, 3, args)                        \
    )
/**/

// Expands to a Boost.Parameter-enabled function call operator header.  All
// arguments are accessible via args and keywords, as well as by name.
#define BOOST_PARAMETER_FUNCTION_CALL_OPERATOR(result, tag_ns, args)         \
    BOOST_PARAMETER_MEMBER_FUNCTION_AUX(                                     \
        result, operator(), operator, tag_ns, 0                              \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(3, 2, 3, args)                        \
    )
/**/

// Expands to a Boost.Parameter-enabled const-qualified function call operator
// header.  All arguments are accessible via args and keywords, as well as
// by name.
#define BOOST_PARAMETER_CONST_FUNCTION_CALL_OPERATOR(result, tag_ns, args)   \
    BOOST_PARAMETER_MEMBER_FUNCTION_AUX(                                     \
        result, operator(), operator, tag_ns, 1                              \
      , BOOST_PARAMETER_AUX_PP_FLATTEN(3, 2, 3, args)                        \
    )
/**/

#endif  // include guard

