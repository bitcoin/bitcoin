//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_GENERATOR_BINDER_APR_17_2009_0952PM)
#define BOOST_SPIRIT_GENERATOR_BINDER_APR_17_2009_0952PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/fusion/include/at.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    // generator_binder for plain rules
    template <typename Generator, typename Auto>
    struct generator_binder
    {
        generator_binder(Generator const& g)
          : g(g) {}

        template <typename OutputIterator, typename Delimiter, typename Context>
        bool call(OutputIterator& sink, Context& context
          , Delimiter const& delim, mpl::true_) const
        {
            // If DeducedAuto is false (semantic actions is present), the 
            // component's attribute is unused.
            return g.generate(sink, context, delim, unused);
        }

        template <typename OutputIterator, typename Delimiter, typename Context>
        bool call(OutputIterator& sink, Context& context
          , Delimiter const& delim, mpl::false_) const
        {
            // If DeducedAuto is true (no semantic action), we pass the rule's 
            // attribute on to the component.
            return g.generate(sink, context, delim
                , fusion::at_c<0>(context.attributes));
        }

        template <typename OutputIterator, typename Delimiter, typename Context>
        bool operator()(OutputIterator& sink, Context& context
          , Delimiter const& delim) const
        {
            // If Auto is false, we need to deduce whether to apply auto rule
            typedef typename traits::has_semantic_action<Generator>::type auto_rule;
            return call(sink, context, delim, auto_rule());
        }

        Generator g;
    };

    // generator_binder for auto rules
    template <typename Generator>
    struct generator_binder<Generator, mpl::true_>
    {
        generator_binder(Generator const& g)
          : g(g) {}

        template <typename OutputIterator, typename Delimiter, typename Context>
        bool operator()(OutputIterator& sink, Context& context
          , Delimiter const& delim) const
        {
            // If Auto is true, the component's attribute is unused.
            return g.generate(sink, context, delim
                , fusion::at_c<0>(context.attributes));
        }

        Generator g;
    };

    template <typename Auto, typename Generator>
    inline generator_binder<Generator, Auto>
    bind_generator(Generator const& g)
    {
        return generator_binder<Generator, Auto>(g);
    }

}}}}

#endif
