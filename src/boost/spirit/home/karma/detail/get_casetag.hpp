//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_GET_CASETAG_JANUARY_19_2009_1107AM)
#define BOOST_SPIRIT_KARMA_GET_CASETAG_JANUARY_19_2009_1107AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/identity.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    template <typename Modifiers, bool case_modifier = false>
    struct get_casetag : mpl::identity<unused_type> {};

    template <typename Modifiers>
    struct get_casetag<Modifiers, true>
      : mpl::if_<has_modifier<Modifiers, tag::char_code_base<tag::lower> >
          , tag::lower, tag::upper> {};
}}}}

#endif
