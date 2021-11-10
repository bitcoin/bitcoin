//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DETAIL_ALTERNATIVE_FUNCTION_HPP
#define BOOST_SPIRIT_KARMA_DETAIL_ALTERNATIVE_FUNCTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/directive/buffer.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/utree/utree_traits_fwd.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/variant.hpp>
#include <boost/detail/workaround.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    //  execute a generator if the given Attribute type is compatible
    ///////////////////////////////////////////////////////////////////////////

    //  this gets instantiated if the Attribute type is _not_ compatible with
    //  the generator
    template <typename Component, typename Attribute, typename Expected
      , typename Enable = void>
    struct alternative_generate
    {
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        call(Component const&, OutputIterator&, Context&, Delimiter const&
          , Attribute const&, bool& failed)
        {
            failed = true;
            return false;
        }
    };

    template <typename Component>
    struct alternative_generate<Component, unused_type, unused_type>
    {
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        call(Component const& component, OutputIterator& sink, Context& ctx
          , Delimiter const& d, unused_type, bool&)
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            component; // suppresses warning: C4100: 'component' : unreferenced formal parameter
#endif
            // return true if any of the generators succeed
            return component.generate(sink, ctx, d, unused);
        }
    };

    //  this gets instantiated if there is no Attribute given for the
    //  alternative generator
    template <typename Component, typename Expected>
    struct alternative_generate<Component, unused_type, Expected>
      : alternative_generate<Component, unused_type, unused_type> {};

    //  this gets instantiated if the generator does not expect to receive an
    //  Attribute (the generator is self contained).
    template <typename Component, typename Attribute>
    struct alternative_generate<Component, Attribute, unused_type>
      : alternative_generate<Component, unused_type, unused_type> {};

    //  this gets instantiated if the Attribute type is compatible to the
    //  generator
    template <typename Component, typename Attribute, typename Expected>
    struct alternative_generate<Component, Attribute, Expected
      , typename enable_if<
            traits::compute_compatible_component<Expected, Attribute, karma::domain> >::type>
    {
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        call(Component const& component, OutputIterator& sink
          , Context& ctx, Delimiter const& d, Attribute const& attr, bool&)
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            component; // suppresses warning: C4100: 'component' : unreferenced formal parameter
#endif
            return call(component, sink, ctx, d, attr
              , spirit::traits::not_is_variant_or_variant_in_optional<Attribute, karma::domain>());
        }

        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        call(Component const& component, OutputIterator& sink
          , Context& ctx, Delimiter const& d, Attribute const& attr, mpl::true_)
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            component; // suppresses warning: C4100: 'component' : unreferenced formal parameter
#endif
            return component.generate(sink, ctx, d, attr);
        }

        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        call(Component const& component, OutputIterator& sink
          , Context& ctx, Delimiter const& d, Attribute const& attr, mpl::false_)
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            component; // suppresses warning: C4100: 'component' : unreferenced formal parameter
#endif
            typedef
                traits::compute_compatible_component<Expected, Attribute, domain>
            component_type;

            // if we got passed an empty optional, just fail generation
            if (!traits::has_optional_value(attr))
                return false;

            // make sure, the content of the passed variant matches our
            // expectations
            typename traits::optional_attribute<Attribute>::type attr_ = 
                traits::optional_value(attr);
            if (!component_type::is_compatible(spirit::traits::which(attr_)))
                return false;

            // returns true if any of the generators succeed
            typedef typename component_type::compatible_type compatible_type;
            return component.generate(sink, ctx, d
              , boost::get<compatible_type>(attr_));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  alternative_generate_function: a functor supplied to fusion::any which
    //  will be executed for every generator in a given alternative generator
    //  expression
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Context, typename Delimiter,
        typename Attribute, typename Strict>
    struct alternative_generate_function
    {
        alternative_generate_function(OutputIterator& sink_, Context& ctx_
              , Delimiter const& d, Attribute const& attr_)
          : sink(sink_), ctx(ctx_), delim(d), attr(attr_) {}

        template <typename Component>
        bool operator()(Component const& component)
        {
            typedef
                typename traits::attribute_of<Component, Context>::type
            expected_type;
            typedef
                alternative_generate<Component, Attribute, expected_type>
            generate;

            // wrap the given output iterator avoid output as long as one
            // component fails
            detail::enable_buffering<OutputIterator> buffering(sink);
            bool r = false;
            bool failed = false;    // will be ignored
            {
                detail::disable_counting<OutputIterator> nocounting(sink);
                r = generate::call(component, sink, ctx, delim, attr, failed);
            }
            if (r) 
                buffering.buffer_copy();
            return r;
        }

        // avoid double buffering
        template <typename Component>
        bool operator()(buffer_directive<Component> const& component)
        {
            typedef typename 
                traits::attribute_of<Component, Context>::type
            expected_type;
            typedef alternative_generate<
                buffer_directive<Component>, Attribute, expected_type>
            generate;

            bool failed = false;    // will be ignored
            return generate::call(component, sink, ctx, delim, attr, failed);
        }

        OutputIterator& sink;
        Context& ctx;
        Delimiter const& delim;
        Attribute const& attr;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(alternative_generate_function& operator= (alternative_generate_function const&))
    };

    // specialization for strict alternatives
    template <typename OutputIterator, typename Context, typename Delimiter,
        typename Attribute>
    struct alternative_generate_function<
        OutputIterator, Context, Delimiter, Attribute, mpl::true_>
    {
        alternative_generate_function(OutputIterator& sink_, Context& ctx_
              , Delimiter const& d, Attribute const& attr_)
          : sink(sink_), ctx(ctx_), delim(d), attr(attr_), failed(false) {}

        template <typename Component>
        bool operator()(Component const& component)
        {
            typedef
                typename traits::attribute_of<Component, Context>::type
            expected_type;
            typedef
                alternative_generate<Component, Attribute, expected_type>
            generate;

            if (failed)
                return false;     // give up when already failed

            // wrap the given output iterator avoid output as long as one
            // component fails
            detail::enable_buffering<OutputIterator> buffering(sink);
            bool r = false;
            {
                detail::disable_counting<OutputIterator> nocounting(sink);
                r = generate::call(component, sink, ctx, delim, attr, failed);
            }
            if (r && !failed) 
            {
                buffering.buffer_copy();
                return true;
            }
            return false;
        }

        OutputIterator& sink;
        Context& ctx;
        Delimiter const& delim;
        Attribute const& attr;
        bool failed;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(alternative_generate_function& operator= (alternative_generate_function const&))
    };
}}}}

#endif
