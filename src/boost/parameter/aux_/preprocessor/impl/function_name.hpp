// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_NAME_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_NAME_HPP

#define BOOST_PARAMETER_MEMBER_FUNCTION_CHECK_STATIC_static ()
/**/

#include <boost/parameter/aux_/preprocessor/is_nullary.hpp>
#include <boost/preprocessor/cat.hpp>

#define BOOST_PARAMETER_MEMBER_FUNCTION_IS_STATIC(name)                      \
    BOOST_PARAMETER_IS_NULLARY(                                              \
        BOOST_PP_CAT(BOOST_PARAMETER_MEMBER_FUNCTION_CHECK_STATIC_, name)    \
    )
/**/

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/config.hpp>

#if defined(BOOST_MSVC)

// Workaround for MSVC preprocessor.
//
// When stripping static from "static f", msvc will produce " f".  The leading
// whitespace doesn't go away when pasting the token with something else, so
// this thing is a hack to strip the whitespace.
#define BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_static (
/**/

#define BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_AUX(name)               \
    BOOST_PP_CAT(BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_, name))
/**/

#define BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC(name)                   \
    BOOST_PP_SEQ_HEAD(                                                       \
        BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_AUX(name)               \
    )
/**/

#else

#define BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_static
/**/

#define BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC(name)                   \
    BOOST_PP_CAT(BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC_, name)
/**/

#endif  // MSVC workarounds needed

#include <boost/preprocessor/control/expr_if.hpp>

#define BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(name)                         \
    BOOST_PP_EXPR_IF(                                                        \
        BOOST_PARAMETER_MEMBER_FUNCTION_IS_STATIC(name), static              \
    )
/**/

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

#define BOOST_PARAMETER_MEMBER_FUNCTION_NAME(name)                           \
    BOOST_PP_IF(                                                             \
        BOOST_PARAMETER_MEMBER_FUNCTION_IS_STATIC(name)                      \
      , BOOST_PARAMETER_MEMBER_FUNCTION_STRIP_STATIC                         \
      , name BOOST_PP_TUPLE_EAT(1)                                           \
    )(name)
/**/

// Produces a name for a parameter specification for the function named base.
#define BOOST_PARAMETER_FUNCTION_SPECIFICATION_NAME(base, is_const)          \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(                                                     \
                is_const                                                     \
              , boost_param_parameters_const_                                \
              , boost_param_parameters_                                      \
            )                                                                \
          , __LINE__                                                         \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(base)                           \
    )
/**/

// Produces a name for a result type metafunction for the no-spec function
// named base.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(base, is_const)         \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(                                                     \
                is_const                                                     \
              , boost_param_no_spec_result_const_                            \
              , boost_param_no_spec_result_                                  \
            )                                                                \
          , __LINE__                                                         \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(base)                           \
    )
/**/

// Produces a name for a result type metafunction for the function named base.
#define BOOST_PARAMETER_FUNCTION_RESULT_NAME(base, is_const)                 \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(                                                     \
                is_const                                                     \
              , boost_param_result_const_                                    \
              , boost_param_result_                                          \
            )                                                                \
          , __LINE__                                                         \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(base)                           \
    )
/**/

// Produces a name for the implementation function to which the no-spec
// function named base forwards its result type and argument pack.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_NAME(base, is_const)           \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(                                                     \
                is_const                                                     \
              , boost_param_no_spec_impl_const                               \
              , boost_param_no_spec_impl                                     \
            )                                                                \
          , __LINE__                                                         \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(base)                           \
    )
/**/

// Can't do boost_param_impl_ ## basee
// because base might start with an underscore.
// daniel: what? how is that relevant? the reason for using CAT()
// is to make sure base is expanded. i'm not sure we need to here,
// but it's more stable to do it.
#define BOOST_PARAMETER_FUNCTION_IMPL_NAME(base, is_const)                   \
    BOOST_PP_CAT(                                                            \
        BOOST_PP_CAT(                                                        \
            BOOST_PP_IF(is_const, boost_param_impl_const, boost_param_impl)  \
          , __LINE__                                                         \
        )                                                                    \
      , BOOST_PARAMETER_MEMBER_FUNCTION_NAME(base)                           \
    )
/**/

#endif  // include guard

