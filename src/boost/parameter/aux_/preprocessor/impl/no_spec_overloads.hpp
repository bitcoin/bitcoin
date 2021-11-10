// Copyright Cromwell D. Enage 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_NO_SPEC_OVERLOADS_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_NO_SPEC_OVERLOADS_HPP

#include <boost/parameter/aux_/preprocessor/impl/function_name.hpp>

// Defines the no-spec implementation function header.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_HEAD(name, is_const)           \
    template <typename ResultType, typename Args>                            \
    BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(name) ResultType                  \
        BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_NAME(                          \
            name, is_const                                                   \
        )(ResultType(*)(), Args const& args)
/**/

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

#include <boost/parameter/aux_/preprocessor/impl/parenthesized_return_type.hpp>

// Expands to the result metafunction for the specified no-spec function.
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_HEAD(result, name, is_const)        \
    template <typename TaggedArg0, typename ...TaggedArgs>                   \
    using BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(name, is_const)       \
    = typename BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(result);
/**/
#else
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_HEAD(result, name, is_const)        \
    template <typename TaggedArg0, typename ...TaggedArgs>                   \
    struct BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(name, is_const)      \
      : BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(result)                    \
    {                                                                        \
    };
/**/
#endif  // BOOST_PARAMETER_CAN_USE_MP11

#include <boost/parameter/compose.hpp>
#include <boost/parameter/are_tagged_arguments.hpp>
#include <boost/parameter/aux_/preprocessor/impl/parenthesized_type.hpp>
#include <boost/core/enable_if.hpp>

// Exapnds to a variadic constructor that is enabled if and only if all its
// arguments are tagged arguments.  The enclosing class must inherit from the
// specified base class, which in turn must implement a constructor that takes
// in the argument pack that this one passes on.
#define BOOST_PARAMETER_NO_SPEC_CONSTRUCTOR(class_, base)                    \
    template <                                                               \
        typename TaggedArg0                                                  \
      , typename ...TaggedArgs                                               \
      , typename = typename ::boost::enable_if<                              \
            ::boost::parameter                                               \
            ::are_tagged_arguments<TaggedArg0,TaggedArgs...>                 \
        >::type                                                              \
    > inline explicit                                                        \
    class_(TaggedArg0 const& arg0, TaggedArgs const&... args)                \
      : BOOST_PARAMETER_PARENTHESIZED_TYPE(base)(                            \
            ::boost::parameter::compose(arg0, args...)                       \
        )                                                                    \
    {                                                                        \
    }
/**/

// Exapnds to a variadic constructor that is enabled if and only if all its
// arguments are tagged arguments.  The specified function must be able to
// take in the argument pack that this constructor passes on.
#define BOOST_PARAMETER_NO_SPEC_NO_BASE_CONSTRUCTOR(class_, func)            \
    template <                                                               \
        typename TaggedArg0                                                  \
      , typename ...TaggedArgs                                               \
      , typename = typename ::boost::enable_if<                              \
            ::boost::parameter                                               \
            ::are_tagged_arguments<TaggedArg0,TaggedArgs...>                 \
        >::type                                                              \
    > inline explicit                                                        \
    class_(TaggedArg0 const& arg0, TaggedArgs const&... args)                \
    {                                                                        \
        func(::boost::parameter::compose(arg0, args...));                    \
    }
/**/

#include <boost/parameter/aux_/preprocessor/nullptr.hpp>
#include <boost/preprocessor/control/expr_if.hpp>

// Exapnds to a variadic function that is enabled if and only if
// all its arguments are tagged arguments.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_OVERLOAD(name, impl, is_m, c)       \
    template <typename TaggedArg0, typename ...TaggedArgs>                   \
    BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(impl)                             \
    inline typename ::boost::lazy_enable_if<                                 \
        ::boost::parameter                                                   \
        ::are_tagged_arguments<TaggedArg0,TaggedArgs...>                     \
      , BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(                        \
            impl, c                                                          \
        )<TaggedArg0,TaggedArgs...>                                          \
    >::type BOOST_PARAMETER_MEMBER_FUNCTION_NAME(name)                       \
    (TaggedArg0 const& arg0, TaggedArgs const&... args)                      \
    BOOST_PP_EXPR_IF(c, const)                                               \
    {                                                                        \
        return BOOST_PP_EXPR_IF(is_m, this->)                                \
        BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_NAME(impl, c)(                 \
            static_cast<                                                     \
                typename BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(       \
                    impl, c                                                  \
                )<TaggedArg0,TaggedArgs...>::type(*)()                       \
            >(BOOST_PARAMETER_AUX_PP_NULLPTR)                                \
          , ::boost::parameter::compose(arg0, args...)                       \
        );                                                                   \
    }
/**/

#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

#include <boost/parameter/aux_/void.hpp>
#include <boost/parameter/aux_/preprocessor/impl/parenthesized_return_type.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

// Expands to the result metafunction for the specified no-spec function.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_HEAD(result, name, is_const)        \
    template <                                                               \
        BOOST_PP_ENUM_BINARY_PARAMS(                                         \
            BOOST_PARAMETER_COMPOSE_MAX_ARITY                                \
          , typename TaggedArg                                               \
          , = ::boost::parameter::void_ BOOST_PP_INTERCEPT                   \
        )                                                                    \
    >                                                                        \
    struct BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(name, is_const)      \
      : BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(result)                    \
    {                                                                        \
    };
/**/

#include <boost/parameter/compose.hpp>
#include <boost/parameter/aux_/preprocessor/impl/parenthesized_type.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#if defined(BOOST_NO_SFINAE)

// Exapnds to a tagged-argument constructor overload that passes the argument
// pack to the base class delegate constructor.
#define BOOST_PARAMETER_NO_SPEC_CONSTRUCTOR_OVERLOAD_Z(z, n, data)           \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PP_EXPR_IF(BOOST_PP_EQUAL(n, 1), explicit) inline                  \
    BOOST_PP_TUPLE_ELEM(2, 0, data)(                                         \
        BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& arg)           \
    ) : BOOST_PARAMETER_PARENTHESIZED_TYPE(BOOST_PP_TUPLE_ELEM(2, 1, data))( \
            ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))   \
        )                                                                    \
    {                                                                        \
    }
/**/

// Exapnds to a tagged-argument constructor overload that passes the argument
// pack to the delegate function.
#define BOOST_PARAMETER_NO_SPEC_NO_BASE_CONSTRUCTOR_OVERLOAD_Z(z, n, data)   \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PP_EXPR_IF(BOOST_PP_EQUAL(n, 1), explicit) inline                  \
    BOOST_PP_TUPLE_ELEM(2, 0, data)(                                         \
        BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& a)             \
    )                                                                        \
    {                                                                        \
        BOOST_PP_TUPLE_ELEM(2, 1, data)(                                     \
            ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, a))     \
        );                                                                   \
    }
/**/

#include <boost/parameter/aux_/preprocessor/nullptr.hpp>

// Exapnds to a tagged-argument function overload.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_OVERLOAD_Z(z, n, data)              \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(BOOST_PP_TUPLE_ELEM(4, 1, data))  \
    inline typename BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(            \
        BOOST_PP_TUPLE_ELEM(4, 1, data)                                      \
      , BOOST_PP_TUPLE_ELEM(4, 3, data)                                      \
    )<BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)>::type                         \
        BOOST_PARAMETER_MEMBER_FUNCTION_NAME(                                \
            BOOST_PP_TUPLE_ELEM(4, 0, data)                                  \
        )(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& arg))        \
        BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(4, 3, data), const)             \
    {                                                                        \
        return BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(4, 2, data), this->)     \
        BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_NAME(                          \
            BOOST_PP_TUPLE_ELEM(4, 1, data)                                  \
          , BOOST_PP_TUPLE_ELEM(4, 3, data)                                  \
        )(                                                                   \
            static_cast<                                                     \
                typename BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(       \
                    BOOST_PP_TUPLE_ELEM(4, 1, data)                          \
                  , BOOST_PP_TUPLE_ELEM(4, 3, data)                          \
                )<BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)>::type(*)()        \
            >(BOOST_PARAMETER_AUX_PP_NULLPTR)                                \
          , ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))   \
        );                                                                   \
    }
/**/

#else   // !defined(BOOST_NO_SFINAE)

#include <boost/parameter/are_tagged_arguments.hpp>
#include <boost/parameter/aux_/preprocessor/nullptr.hpp>
#include <boost/core/enable_if.hpp>

// Exapnds to a tagged-argument constructor overload that passes the argument
// pack to the base class delegate constructor.  This constructor is enabled
// if and only if all its arguments are tagged arguments.
#define BOOST_PARAMETER_NO_SPEC_CONSTRUCTOR_OVERLOAD_Z(z, n, data)           \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PP_EXPR_IF(BOOST_PP_EQUAL(n, 1), explicit) inline                  \
    BOOST_PP_TUPLE_ELEM(2, 0, data)(                                         \
        BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& arg)           \
      , typename ::boost::enable_if<                                         \
            ::boost::parameter::are_tagged_arguments<                        \
                BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)                      \
            >                                                                \
        >::type* = BOOST_PARAMETER_AUX_PP_NULLPTR                            \
    ) : BOOST_PARAMETER_PARENTHESIZED_TYPE(BOOST_PP_TUPLE_ELEM(2, 1, data))( \
            ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))   \
        )                                                                    \
    {                                                                        \
    }
/**/

// Exapnds to a tagged-argument constructor overload that passes the argument
// pack to the delegate function.  This constructor is enabled if and only if
// all its arguments are tagged arguments.
#define BOOST_PARAMETER_NO_SPEC_NO_BASE_CONSTRUCTOR_OVERLOAD_Z(z, n, data)   \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PP_EXPR_IF(BOOST_PP_EQUAL(n, 1), explicit) inline                  \
    BOOST_PP_TUPLE_ELEM(2, 0, data)(                                         \
        BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& a)             \
      , typename ::boost::enable_if<                                         \
            ::boost::parameter::are_tagged_arguments<                        \
                BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)                      \
            >                                                                \
        >::type* = BOOST_PARAMETER_AUX_PP_NULLPTR                            \
    )                                                                        \
    {                                                                        \
        BOOST_PP_TUPLE_ELEM(2, 1, data)(                                     \
            ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, a))     \
        );                                                                   \
    }
/**/

// Exapnds to a function overload that is enabled if and only if
// all its arguments are tagged arguments.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_OVERLOAD_Z(z, n, data)              \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename TaggedArg)>              \
    BOOST_PARAMETER_MEMBER_FUNCTION_STATIC(BOOST_PP_TUPLE_ELEM(4, 1, data))  \
    inline typename ::boost::lazy_enable_if<                                 \
        ::boost::parameter                                                   \
        ::are_tagged_arguments<BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)>      \
      , BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(                        \
            BOOST_PP_TUPLE_ELEM(4, 1, data)                                  \
          , BOOST_PP_TUPLE_ELEM(4, 3, data)                                  \
        )<BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)>                           \
    >::type BOOST_PARAMETER_MEMBER_FUNCTION_NAME(                            \
        BOOST_PP_TUPLE_ELEM(4, 0, data)                                      \
    )(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, TaggedArg, const& arg))            \
    BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(4, 3, data), const)                 \
    {                                                                        \
        return BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(4, 2, data), this->)     \
        BOOST_PARAMETER_NO_SPEC_FUNCTION_IMPL_NAME(                          \
            BOOST_PP_TUPLE_ELEM(4, 1, data)                                  \
          , BOOST_PP_TUPLE_ELEM(4, 3, data)                                  \
        )(                                                                   \
            static_cast<                                                     \
                typename BOOST_PARAMETER_NO_SPEC_FUNCTION_RESULT_NAME(       \
                    BOOST_PP_TUPLE_ELEM(4, 1, data)                          \
                  , BOOST_PP_TUPLE_ELEM(4, 3, data)                          \
                )<BOOST_PP_ENUM_PARAMS_Z(z, n, TaggedArg)>::type(*)()        \
            >(BOOST_PARAMETER_AUX_PP_NULLPTR)                                \
          , ::boost::parameter::compose(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))   \
        );                                                                   \
    }
/**/

#endif  // BOOST_NO_SFINAE

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

// Emulates a variadic constructor that is enabled if and only if all its
// arguments are tagged arguments.  The enclosing class must inherit from the
// specified base class, which in turn must implement a constructor that takes
// in the argument pack that this one passes on.
#define BOOST_PARAMETER_NO_SPEC_CONSTRUCTOR(class_, base)                    \
    BOOST_PP_REPEAT_FROM_TO(                                                 \
        1                                                                    \
      , BOOST_PP_INC(BOOST_PARAMETER_COMPOSE_MAX_ARITY)                      \
      , BOOST_PARAMETER_NO_SPEC_CONSTRUCTOR_OVERLOAD_Z                       \
      , (class_, base)                                                       \
    )
/**/

// Emulates a variadic constructor that is enabled if and only if all its
// arguments are tagged arguments.  The specified function must be able to
// take in the argument pack that this constructor passes on.
#define BOOST_PARAMETER_NO_SPEC_NO_BASE_CONSTRUCTOR(class_, func)            \
    BOOST_PP_REPEAT_FROM_TO(                                                 \
        1                                                                    \
      , BOOST_PP_INC(BOOST_PARAMETER_COMPOSE_MAX_ARITY)                      \
      , BOOST_PARAMETER_NO_SPEC_NO_BASE_CONSTRUCTOR_OVERLOAD_Z               \
      , (class_, func)                                                       \
    )
/**/

// Emulates a variadic function that is enabled if and only if
// all its arguments are tagged arguments.
#define BOOST_PARAMETER_NO_SPEC_FUNCTION_OVERLOAD(name, impl, is_m, c)       \
    BOOST_PP_REPEAT_FROM_TO(                                                 \
        1                                                                    \
      , BOOST_PP_INC(BOOST_PARAMETER_COMPOSE_MAX_ARITY)                      \
      , BOOST_PARAMETER_NO_SPEC_FUNCTION_OVERLOAD_Z                          \
      , (name, impl, is_m, c)                                                \
    )
/**/

#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING
#endif  // include guard

