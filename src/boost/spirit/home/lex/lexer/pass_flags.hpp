//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_PASS_FLAGS_JUN_09_2009_0840PM)
#define BOOST_SPIRIT_LEX_PASS_FLAGS_JUN_09_2009_0840PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>

namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    BOOST_SCOPED_ENUM_START(pass_flags) 
    { 
        pass_fail = 0,        // make the current match fail in retrospective
        pass_normal = 1,      // continue normal token matching, that's the default 
        pass_ignore = 2       // ignore the current token and start matching the next
    };
    BOOST_SCOPED_ENUM_END

}}}

#endif
