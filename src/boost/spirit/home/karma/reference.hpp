//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_REFERENCE_APR_17_2009_1057PM)
#define BOOST_SPIRIT_KARMA_REFERENCE_APR_17_2009_1057PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/ref.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // reference is a generator that references another generator (its Subject)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct reference : generator<reference<Subject> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        typedef Subject subject_type;

        reference(Subject& subject)
          : ref(subject) {}

        template <typename Context, typename Unused>
        struct attribute : Subject::template attribute<Context, Unused> {};

        // Default overload, used whenever the attribute is not unused and not
        // used from an aliased rule.
        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& delim, Attribute const& attr) const
        {
            return ref.get().generate(sink, context, delim, attr);
        }

        // This overload gets called from an aliased rule only, we take the 
        // attribute from the context provided from the wrapper rule.
        template <typename OutputIterator, typename Context
          , typename Delimiter>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& delim, unused_type) const
        {
            return ref.get().generate(sink, context, delim, context.attributes);
        }

        // This overload is used whenever no attribute is given and it is used
        // not from an aliased rule.
        template <typename OutputIterator, typename Delimiter>
        bool generate(OutputIterator& sink, unused_type
          , Delimiter const& delim, unused_type) const
        {
            return ref.get().generate(sink, unused, delim, unused);
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
    struct handles_container<karma::reference<Subject>, Attribute
      , Context, Iterator>
      : handles_container<typename remove_const<Subject>::type, Attribute
        , Context, Iterator> 
    {};
}}}

#endif
