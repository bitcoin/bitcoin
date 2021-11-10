//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DELIMIT_FLAG_DEC_02_2009_1201PM)
#define BOOST_SPIRIT_KARMA_DELIMIT_FLAG_DEC_02_2009_1201PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    BOOST_SCOPED_ENUM_START(delimit_flag) 
    { 
        predelimit,         // force predelimiting in generate_delimited()
        dont_predelimit     // inhibit predelimiting in generate_delimited()
    };
    BOOST_SCOPED_ENUM_END
}}}

#endif

