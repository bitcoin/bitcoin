//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DEBUG_HANDLER_STATE_APR_21_2010_0736PM)
#define BOOST_SPIRIT_KARMA_DEBUG_HANDLER_STATE_APR_21_2010_0736PM

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost { namespace spirit { namespace karma
{
    enum debug_handler_state
    {
        pre_generate
      , successful_generate
      , failed_generate
    };
}}}

#endif
