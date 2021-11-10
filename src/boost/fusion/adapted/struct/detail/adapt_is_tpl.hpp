/*=============================================================================
    Copyright (c) 2013-2014 Damien Buhl

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_IS_TPL_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_IS_TPL_HPP

#include <boost/preprocessor/seq/seq.hpp>

#define BOOST_FUSION_ADAPT_IS_TPL(TEMPLATE_PARAMS_SEQ)                          \
    BOOST_PP_SEQ_HEAD(TEMPLATE_PARAMS_SEQ)

#endif
