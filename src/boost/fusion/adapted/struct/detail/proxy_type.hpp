/*=============================================================================
    Copyright (c) 2009-2010 Hartmut Kaiser
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_DETAIL_STRUCT_PROXY_TYPE_HPP
#define BOOST_FUSION_ADAPTED_DETAIL_STRUCT_PROXY_TYPE_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/adapted/struct/detail/namespace.hpp>

#define BOOST_FUSION_PROXY_PREFIX() obj.

#define BOOST_FUSION_ADAPT_STRUCT_DEFINE_PROXY_TYPE_IMPL(                       \
    WRAPPED_TYPE,NAMESPACE_SEQ,NAME)                                            \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DEFINITION_BEGIN(NAMESPACE_SEQ)         \
                                                                                \
    struct NAME                                                                 \
    {                                                                           \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        NAME(WRAPPED_TYPE& in_obj)                                              \
          : obj(in_obj)                                                         \
        {}                                                                      \
                                                                                \
        WRAPPED_TYPE& obj;                                                      \
                                                                                \
        BOOST_DELETED_FUNCTION(NAME& operator= (NAME const&))                   \
    };                                                                          \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DEFINITION_END(NAMESPACE_SEQ)

#define BOOST_FUSION_ADAPT_STRUCT_DEFINE_PROXY_TYPE(                            \
    WRAPPED_TYPE, NAMESPACE_SEQ, NAME)                                          \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_DEFINE_PROXY_TYPE_IMPL(                           \
        WRAPPED_TYPE, (0)NAMESPACE_SEQ, NAME)

#endif
