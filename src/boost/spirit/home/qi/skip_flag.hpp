/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_SKIP_FLAG_DEC_02_2009_0412PM)
#define BOOST_SPIRIT_SKIP_FLAG_DEC_02_2009_0412PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>

namespace boost { namespace spirit { namespace qi 
{
    ///////////////////////////////////////////////////////////////////////////
    BOOST_SCOPED_ENUM_START(skip_flag) 
    { 
        postskip,           // force post-skipping in phrase_parse()
        dont_postskip       // inhibit post-skipping in phrase_parse()
    };
    BOOST_SCOPED_ENUM_END

}}}

#endif

