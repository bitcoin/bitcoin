//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_STRICT_RELAXED_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_STRICT_RELAXED_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::strict>  // enables strict[]
      : mpl::true_ {};

    template <>
    struct use_directive<karma::domain, tag::relaxed> // enables relaxed[]
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct is_modifier_directive<karma::domain, tag::strict>
      : mpl::true_ {};

    template <>
    struct is_modifier_directive<karma::domain, tag::relaxed>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // Don't add tag::strict or tag::relaxed if there is already one of those 
    // in the modifier list
    template <typename Current>
    struct compound_modifier<Current, tag::strict
          , typename enable_if<has_modifier<Current, tag::relaxed> >::type>
      : Current
    {
        compound_modifier()
          : Current() {}

        compound_modifier(Current const& current, tag::strict const&)
          : Current(current) {}
    };

    template <typename Current>
    struct compound_modifier<Current, tag::relaxed
          , typename enable_if<has_modifier<Current, tag::strict> >::type>
      : Current
    {
        compound_modifier()
          : Current() {}

        compound_modifier(Current const& current, tag::relaxed const&)
          : Current(current) {}
    };

    namespace karma
    {
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
        using boost::spirit::strict;
        using boost::spirit::relaxed;
#endif
        using boost::spirit::strict_type;
        using boost::spirit::relaxed_type;
    }
}}

#endif
