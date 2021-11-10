/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_REFERENCE_OCTOBER_31_2008_1218AM)
#define BOOST_SPIRIT_REFERENCE_OCTOBER_31_2008_1218AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/ref.hpp>

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // reference is a parser that references another parser (its Subject)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct reference : parser<reference<Subject> >
    {
        typedef Subject subject_type;

        reference(Subject& subject)
          : ref(subject) {}

        template <typename Context, typename Iterator>
        struct attribute : Subject::template attribute<Context, Iterator> {};

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            return ref.get().parse(first, last, context, skipper, attr_);
        }

        template <typename Context>
        info what(Context& context) const
        {
            // the reference is transparent (does not add any info)
            return ref.get().what(context);
        }

        boost::reference_wrapper<Subject> ref;
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<qi::reference<Subject>, Attribute, Context
      , Iterator>
      : handles_container<typename remove_const<Subject>::type
        , Attribute, Context, Iterator> 
    {};
}}}

#endif
