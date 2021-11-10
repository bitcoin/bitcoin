// Copyright Cromwell D. Enage 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_COMPOSE_HPP
#define BOOST_PARAMETER_COMPOSE_HPP

#include <boost/parameter/aux_/arg_list.hpp>

namespace boost { namespace parameter {

    inline BOOST_CONSTEXPR ::boost::parameter::aux::empty_arg_list compose()
    {
        return ::boost::parameter::aux::empty_arg_list();
    }
}} // namespace boost::parameter

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

namespace boost { namespace parameter { namespace aux {

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
    template <typename ...TaggedArgs>
    struct compose_arg_list
    {
        using type = ::boost::parameter::aux::flat_like_arg_list<
            ::boost::parameter::aux::flat_like_arg_tuple<
                typename TaggedArgs::base_type::key_type
              , typename TaggedArgs::base_type
            >...
        >;
    };
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
    template <typename TaggedArg0, typename ...TaggedArgs>
    struct compose_arg_list;

    template <typename TaggedArg0>
    struct compose_arg_list<TaggedArg0>
    {
        typedef ::boost::parameter::aux::arg_list<TaggedArg0> type;
    };

    template <typename TaggedArg0, typename ...TaggedArgs>
    struct compose_arg_list
    {
        typedef ::boost::parameter::aux::arg_list<
            TaggedArg0
          , typename ::boost::parameter::aux
            ::compose_arg_list<TaggedArgs...>::type
        > type;
    };
#endif  // BOOST_PARAMETER_CAN_USE_MP11
}}} // namespace boost::parameter::aux

#include <boost/parameter/are_tagged_arguments.hpp>
#include <boost/core/enable_if.hpp>

namespace boost { namespace parameter { namespace result_of {

    template <typename ...TaggedArgs>
    struct compose
      : ::boost::lazy_enable_if<
            ::boost::parameter::are_tagged_arguments<TaggedArgs...>
          , ::boost::parameter::aux::compose_arg_list<TaggedArgs...>
        >
    {
    };

    template <>
    struct compose<>
    {
        typedef ::boost::parameter::aux::empty_arg_list type;
    };
}}} // namespace boost::parameter::result_of

namespace boost { namespace parameter {

    template <typename TaggedArg0, typename ...TaggedArgs>
    inline BOOST_CONSTEXPR typename ::boost::parameter::result_of
    ::compose<TaggedArg0,TaggedArgs...>::type
        compose(TaggedArg0 const& arg0, TaggedArgs const&... args)
    {
        return typename ::boost::parameter::aux
        ::compose_arg_list<TaggedArg0,TaggedArgs...>::type(
            ::boost::parameter::aux::value_type_is_not_void()
          , arg0
          , args...
        );
    }
}} // namespace boost::parameter

#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

#define BOOST_PARAMETER_compose_arg_list_type_suffix(z, n, suffix) suffix

#include <boost/preprocessor/cat.hpp>

#define BOOST_PARAMETER_compose_arg_list_type_prefix(z, n, prefix)           \
    ::boost::parameter::aux::arg_list<BOOST_PP_CAT(prefix, n)
/**/

#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define BOOST_PARAMETER_compose_arg_list_type(z, n, prefix)                  \
    BOOST_PP_CAT(BOOST_PP_ENUM_, z)(                                         \
        n, BOOST_PARAMETER_compose_arg_list_type_prefix, prefix              \
    ) BOOST_PP_CAT(BOOST_PP_REPEAT_, z)(                                     \
        n, BOOST_PARAMETER_compose_arg_list_type_suffix, >                   \
    )
/**/

#include <boost/parameter/aux_/void.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

#if defined(BOOST_NO_SFINAE)
#define BOOST_PARAMETER_compose_arg_list_function_overload(z, n, prefix)     \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename prefix)>                 \
    inline BOOST_CONSTEXPR                                                   \
    BOOST_PARAMETER_compose_arg_list_type(z, n, prefix)                      \
        compose(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, prefix, const& a))       \
    {                                                                        \
        return BOOST_PARAMETER_compose_arg_list_type(z, n, prefix)(          \
            BOOST_PP_ENUM_PARAMS_Z(z, n, a)                                  \
            BOOST_PP_ENUM_TRAILING_PARAMS_Z(                                 \
                z                                                            \
              , BOOST_PP_SUB(BOOST_PARAMETER_COMPOSE_MAX_ARITY, n)           \
              , ::boost::parameter::aux::void_reference() BOOST_PP_INTERCEPT \
            )                                                                \
        );                                                                   \
    }
/**/
#else   // !defined(BOOST_NO_SFINAE)
#include <boost/parameter/are_tagged_arguments.hpp>
#include <boost/core/enable_if.hpp>

namespace boost { namespace parameter { namespace result_of {

    template <
        BOOST_PP_ENUM_BINARY_PARAMS(
            BOOST_PP_INC(BOOST_PARAMETER_COMPOSE_MAX_ARITY)
          , typename TaggedArg
          , = void BOOST_PP_INTERCEPT
        )
    >
    struct compose;

    template <>
    struct compose<>
    {
        typedef ::boost::parameter::aux::empty_arg_list type;
    };
}}} // namespace boost::parameter::result_of

#define BOOST_PARAMETER_compose_arg_list_function_overload(z, n, prefix)     \
namespace boost { namespace parameter { namespace result_of {                \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename prefix)>                 \
    struct compose<BOOST_PP_ENUM_PARAMS_Z(z, n, prefix)>                     \
      : ::boost::enable_if<                                                  \
            ::boost::parameter                                               \
            ::are_tagged_arguments<BOOST_PP_ENUM_PARAMS_Z(z, n, prefix)>     \
          , BOOST_PARAMETER_compose_arg_list_type(z, n, prefix)              \
        >                                                                    \
    {                                                                        \
    };                                                                       \
}}}                                                                          \
namespace boost { namespace parameter {                                      \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename prefix)>                 \
    inline BOOST_CONSTEXPR typename ::boost::parameter::result_of            \
    ::compose<BOOST_PP_ENUM_PARAMS_Z(z, n, prefix)>::type                    \
        compose(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, prefix, const& a))       \
    {                                                                        \
        return BOOST_PARAMETER_compose_arg_list_type(z, n, prefix)(          \
            BOOST_PP_ENUM_PARAMS_Z(z, n, a)                                  \
            BOOST_PP_ENUM_TRAILING_PARAMS_Z(                                 \
                z                                                            \
              , BOOST_PP_SUB(BOOST_PARAMETER_COMPOSE_MAX_ARITY, n)           \
              , ::boost::parameter::aux::void_reference() BOOST_PP_INTERCEPT \
            )                                                                \
        );                                                                   \
    }                                                                        \
}}
/**/
#endif  // BOOST_NO_SFINAE

#include <boost/preprocessor/repetition/repeat_from_to.hpp>

BOOST_PP_REPEAT_FROM_TO(
    1
  , BOOST_PP_INC(BOOST_PARAMETER_COMPOSE_MAX_ARITY)
  , BOOST_PARAMETER_compose_arg_list_function_overload
  , TaggedArg
)

#undef BOOST_PARAMETER_compose_arg_list_function_overload
#undef BOOST_PARAMETER_compose_arg_list_type
#undef BOOST_PARAMETER_compose_arg_list_type_prefix
#undef BOOST_PARAMETER_compose_arg_list_type_suffix

#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING
#endif  // include guard

