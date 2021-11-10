//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2009 Francois Barel
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_PARAMETERIZED_AUGUST_09_2009_0601AM)
#define BOOST_SPIRIT_KARMA_PARAMETERIZED_AUGUST_09_2009_0601AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/ref.hpp>

#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/generator.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // parameterized_nonterminal: generator representing the invocation of a
    // nonterminal, passing inherited attributes
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Params>
    struct parameterized_nonterminal
      : generator<parameterized_nonterminal<Subject, Params> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        parameterized_nonterminal(Subject const& subject, Params const& params)
          : ref(subject), params(params)
        {
        }

        template <typename Context, typename Unused>
        struct attribute
            // Forward to subject.
          : Subject::template attribute<Context, Unused> {};

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& delim, Attribute const& attr) const
        {
            // Forward to subject, passing the additional
            // params argument to generate.
            return ref.get().generate(sink, context, delim, attr, params);
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
    struct handles_container<karma::parameterized_nonterminal<Subject, Params>
          , Attribute, Context, Iterator>
      : handles_container<typename remove_const<Subject>::type
        , Attribute, Context, Iterator> 
    {};
}}}

#endif
