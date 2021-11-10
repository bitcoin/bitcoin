//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_DOMAIN_MAR_13_2007_0140PM)
#define BOOST_SPIRIT_LEX_DOMAIN_MAR_13_2007_0140PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/meta_compiler.hpp>
#include <boost/spirit/home/support/info.hpp>

namespace boost { namespace spirit { namespace lex
{
    // lex's domain
    struct domain {};

    // bring in some of spirit parts into spirit::lex
    using spirit::unused;
    using spirit::unused_type;
    using spirit::compile;
    using spirit::info;

}}}

#endif
