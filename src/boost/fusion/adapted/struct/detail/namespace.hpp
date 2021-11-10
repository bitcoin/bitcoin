/*=============================================================================
    Copyright (c) 2009-2010 Hartmut Kaiser
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_DETAIL_STRUCT_NAMESPACE_HPP
#define BOOST_FUSION_ADAPTED_DETAIL_STRUCT_NAMESPACE_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_BEGIN_I(R,DATA,ELEM)                \
    namespace ELEM {
#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_END_I(Z,I,DATA) }
#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION_I(Z,I,ELEM) ELEM::

#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DEFINITION_BEGIN(NAMESPACE_SEQ)     \
    BOOST_PP_IF(                                                                \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(NAMESPACE_SEQ)),                         \
        BOOST_PP_SEQ_FOR_EACH_R,                                                \
        BOOST_PP_TUPLE_EAT(4))(                                                 \
            1,                                                                  \
            BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_BEGIN_I,                        \
            _,                                                                  \
            BOOST_PP_SEQ_TAIL(NAMESPACE_SEQ))

#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DEFINITION_END(NAMESPACE_SEQ)       \
    BOOST_PP_REPEAT_1(                                                          \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(NAMESPACE_SEQ)),                         \
        BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_END_I,                              \
        _)

#define BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION(NAMESPACE_SEQ)          \
    BOOST_PP_IF(                                                                \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(NAMESPACE_SEQ)),                         \
        BOOST_PP_SEQ_FOR_EACH_R,                                                \
        BOOST_PP_TUPLE_EAT(4))(                                                 \
            1,                                                                  \
            BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION_I,                  \
            _,                                                                  \
            BOOST_PP_SEQ_TAIL(NAMESPACE_SEQ))

#endif
