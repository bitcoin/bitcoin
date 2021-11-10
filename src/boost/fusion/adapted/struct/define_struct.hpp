/*=============================================================================
    Copyright (c) 2010-2011 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DEFINE_STRUCT_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DEFINE_STRUCT_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/detail/define_struct.hpp>
#include <boost/preprocessor/cat.hpp>

#define BOOST_FUSION_DEFINE_TPL_STRUCT(                                         \
    TEMPLATE_PARAMS_SEQ, NAMESPACE_SEQ, NAME, ATTRIBUTES)                       \
                                                                                \
    BOOST_FUSION_DEFINE_TPL_STRUCT_IMPL(                                        \
        TEMPLATE_PARAMS_SEQ,                                                    \
        (0)NAMESPACE_SEQ,                                                       \
        NAME,                                                                   \
        BOOST_PP_CAT(BOOST_FUSION_DEFINE_STRUCT_FILLER_0(0,0)ATTRIBUTES,_END),  \
        2)                                                                      \
                                                                                \
    BOOST_FUSION_ADAPT_TPL_STRUCT(                                              \
        TEMPLATE_PARAMS_SEQ,                                                    \
        (BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION((0)NAMESPACE_SEQ) NAME)\
            TEMPLATE_PARAMS_SEQ,                                                \
        ATTRIBUTES)

#define BOOST_FUSION_DEFINE_STRUCT(NAMESPACE_SEQ, NAME, ATTRIBUTES)             \
    BOOST_FUSION_DEFINE_STRUCT_IMPL(                                            \
        (0)NAMESPACE_SEQ,                                                       \
        NAME,                                                                   \
        BOOST_PP_CAT(BOOST_FUSION_DEFINE_STRUCT_FILLER_0(0,0)ATTRIBUTES,_END),  \
        2)                                                                      \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT(                                                  \
        BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION((0)NAMESPACE_SEQ) NAME, \
        ATTRIBUTES)

#endif
