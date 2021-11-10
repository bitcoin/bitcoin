// Copyright Eric Niebler 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_NESTED_KEYWORD_HPP
#define BOOST_PARAMETER_NESTED_KEYWORD_HPP

#include <boost/parameter/aux_/name.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_NESTED_KEYWORD_AUX(tag_namespace, q, name, alias)    \
    namespace tag_namespace                                                  \
    {                                                                        \
        template <int Dummy = 0>                                             \
        struct BOOST_PP_CAT(name, _)                                         \
        {                                                                    \
            static BOOST_CONSTEXPR char const* keyword_name()                \
            {                                                                \
                return BOOST_PP_STRINGIZE(name);                             \
            }                                                                \
            using _ = BOOST_PARAMETER_TAG_PLACEHOLDER_TYPE(                  \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            );                                                               \
            using _1 = BOOST_PARAMETER_TAG_PLACEHOLDER_TYPE(                 \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            );                                                               \
            BOOST_PARAMETER_TAG_MP11_PLACEHOLDER_BINDING(                    \
                binding_fn                                                   \
              , BOOST_PP_CAT(name, _)<Dummy>                                 \
            );                                                               \
            BOOST_PARAMETER_TAG_MP11_PLACEHOLDER_VALUE(                      \
                fn                                                           \
              , BOOST_PP_CAT(name, _)<Dummy>                                 \
            );                                                               \
            using qualifier = ::boost::parameter::q;                         \
            static ::boost::parameter::keyword<                              \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            > const& alias;                                                  \
        };                                                                   \
        template <int Dummy>                                                 \
        ::boost::parameter::keyword<                                         \
            BOOST_PP_CAT(name, _)<Dummy>                                     \
        > const& BOOST_PP_CAT(name, _)<Dummy>::alias                         \
            = ::boost::parameter::keyword<                                   \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            >::instance;                                                     \
        typedef BOOST_PP_CAT(name, _)<> name;                                \
    }                                                                        \
    namespace                                                                \
    {                                                                        \
        ::boost::parameter::keyword<tag_namespace::name> const& name         \
            = ::boost::parameter::keyword<tag_namespace::name>::instance;    \
    }
/**/
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_NESTED_KEYWORD_AUX(tag_namespace, q, name, alias)    \
    namespace tag_namespace                                                  \
    {                                                                        \
        template <int Dummy = 0>                                             \
        struct BOOST_PP_CAT(name, _)                                         \
        {                                                                    \
            static BOOST_CONSTEXPR char const* keyword_name()                \
            {                                                                \
                return BOOST_PP_STRINGIZE(name);                             \
            }                                                                \
            typedef BOOST_PARAMETER_TAG_PLACEHOLDER_TYPE(                    \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            ) _;                                                             \
            typedef BOOST_PARAMETER_TAG_PLACEHOLDER_TYPE(                    \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            ) _1;                                                            \
            typedef ::boost::parameter::q qualifier;                         \
            static ::boost::parameter::keyword<                              \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            > const& alias;                                                  \
        };                                                                   \
        template <int Dummy>                                                 \
        ::boost::parameter::keyword<                                         \
            BOOST_PP_CAT(name, _)<Dummy>                                     \
        > const& BOOST_PP_CAT(name, _)<Dummy>::alias                         \
            = ::boost::parameter::keyword<                                   \
                BOOST_PP_CAT(name, _)<Dummy>                                 \
            >::instance;                                                     \
        typedef BOOST_PP_CAT(name, _)<> name;                                \
    }                                                                        \
    namespace                                                                \
    {                                                                        \
        ::boost::parameter::keyword<tag_namespace::name> const& name         \
            = ::boost::parameter::keyword<tag_namespace::name>::instance;    \
    }
/**/
#endif  // BOOST_PARAMETER_CAN_USE_MP11

#include <boost/parameter/aux_/preprocessor/qualifier.hpp>

#define BOOST_PARAMETER_NESTED_KEYWORD(tag_namespace, name, alias)           \
    BOOST_PARAMETER_NESTED_KEYWORD_AUX(                                      \
        tag_namespace                                                        \
      , BOOST_PARAMETER_GET_QUALIFIER(name)                                  \
      , BOOST_PARAMETER_UNQUALIFIED(name)                                    \
      , alias                                                                \
    )
/**/

#endif  // include guard

