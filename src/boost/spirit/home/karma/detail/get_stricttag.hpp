//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_GET_STRICTTAG_APR_22_2010_1007AM)
#define BOOST_SPIRIT_KARMA_GET_STRICTTAG_APR_22_2010_1007AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/or.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    // the default mode for Karma is 'relaxed'
    template <
        typename Modifiers
      , typename StrictModifier = typename mpl::or_<
            has_modifier<Modifiers, tag::strict>
          , has_modifier<Modifiers, tag::relaxed> >::type>
    struct get_stricttag : mpl::false_ {};

    // strict mode is enforced only when tag::strict is on the modifiers list
    template <typename Modifiers>
    struct get_stricttag<Modifiers, mpl::true_>
      : has_modifier<Modifiers, tag::strict> {};
}}}}

#endif
