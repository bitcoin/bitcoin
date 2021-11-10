//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_ENCODING_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_ENCODING_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables encoding
    template <typename CharEncoding>
    struct use_directive<
            karma::domain, tag::char_code<tag::encoding, CharEncoding> > 
      : mpl::true_ {};

    template <typename CharEncoding>
    struct is_modifier_directive<
            karma::domain, tag::char_code<tag::encoding, CharEncoding> >
      : mpl::true_ {};
}}

#endif
