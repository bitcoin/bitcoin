//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_QI_NONTERMINAL_FWD_DEC_24_2010_1105PM)
#define BOOST_SPIRIT_QI_NONTERMINAL_FWD_DEC_24_2010_1105PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>

namespace boost { namespace spirit { namespace qi
{
    // forward declaration only
    template <
        typename Iterator, typename T1 = unused_type
      , typename T2 = unused_type, typename T3 = unused_type
      , typename T4 = unused_type>
    struct rule;

    template <
        typename Iterator, typename T1 = unused_type
      , typename T2 = unused_type, typename T3 = unused_type
      , typename T4 = unused_type> 
    struct grammar;
}}}

#endif
