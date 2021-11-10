//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_QI_AUTO_NOV_29_2009_0336PM)
#define BOOST_SPIRIT_QI_AUTO_NOV_29_2009_0336PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/auto/create_parser.hpp>
#include <boost/mpl/bool.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<qi::domain, tag::auto_>     // enables auto_
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::auto_;
#endif
    using spirit::auto_type;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct auto_parser
      : parser<auto_parser<Modifiers> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef spirit::hold_any type;
        };

        auto_parser(Modifiers const& modifiers)
          : modifiers_(modifiers) {}

        template <typename Iterator, typename Context, typename Skipper
          , typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, Attribute& attr) const
        {
            return compile<qi::domain>(create_parser<Attribute>(), modifiers_)
                      .parse(first, last, context, skipper, attr);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("auto_");
        }

        Modifiers modifiers_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::auto_, Modifiers>
    {
        typedef auto_parser<Modifiers> result_type;

        result_type operator()(unused_type, Modifiers const& modifiers) const
        {
            return result_type(modifiers);
        }
    };
}}}

#endif
