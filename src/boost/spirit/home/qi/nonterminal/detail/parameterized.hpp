/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2009 Francois Barel

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_PARAMETERIZED_AUGUST_09_2009_0539AM)
#define BOOST_SPIRIT_PARAMETERIZED_AUGUST_09_2009_0539AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/ref.hpp>

#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/qi/parser.hpp>

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // parameterized_nonterminal: parser representing the invocation of a
    // nonterminal, passing inherited attributes
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Params>
    struct parameterized_nonterminal
      : parser<parameterized_nonterminal<Subject, Params> >
    {
        parameterized_nonterminal(Subject const& subject, Params const& params_)
          : ref(subject), params(params_)
        {
        }

        template <typename Context, typename Iterator>
        struct attribute
            // Forward to subject.
          : Subject::template attribute<Context, Iterator> {};

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // Forward to subject, passing the additional
            // params argument to parse.
            return ref.get().parse(first, last, context, skipper, attr_, params);
        }

        template <typename Context>
        info what(Context& context) const
        {
            // Forward to subject.
            return ref.get().what(context);
        }

        boost::reference_wrapper<Subject const> ref;
        Params params;
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Params, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<qi::parameterized_nonterminal<Subject, Params>
          , Attribute, Context, Iterator>
      : handles_container<typename remove_const<Subject>::type
        , Attribute, Context, Iterator> 
    {};
}}}

#endif
