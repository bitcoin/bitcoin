/*=============================================================================
    Copyright (c) 2012 Nathan Ridge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DEFINE_STRUCT_INLINE_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DEFINE_STRUCT_INLINE_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/detail/define_struct_inline.hpp>

#define BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE(                                  \
    TEMPLATE_PARAMS_SEQ, NAME, ATTRIBUTES)                                      \
                                                                                \
    BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE_IMPL(                                 \
        TEMPLATE_PARAMS_SEQ,                                                    \
        NAME,                                                                   \
        ATTRIBUTES)

#define BOOST_FUSION_DEFINE_STRUCT_INLINE(NAME, ATTRIBUTES)                     \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_IMPL(NAME, ATTRIBUTES)                    \

#endif
