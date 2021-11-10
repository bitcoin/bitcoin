//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_AUTO_NOV_29_2009_0339PM)
#define BOOST_SPIRIT_KARMA_AUTO_NOV_29_2009_0339PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/auto/create_generator.hpp>
#include <boost/mpl/bool.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, tag::auto_>     // enables auto_
      : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                   // enables auto_(...)
      , terminal_ex<tag::auto_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <>                                         // enables auto_(f)
    struct use_lazy_terminal<
        karma::domain, tag::auto_, 1   /*arity*/
    > : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::auto_;
#endif
    using spirit::auto_type;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct auto_generator
      : generator<auto_generator<Modifiers> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef spirit::basic_hold_any<char> type;
        };

        auto_generator(Modifiers const& modifiers)
          : modifiers_(modifiers) {}

        // auto_generator has an attached attribute
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr) const
        {
            return compile<karma::domain>(create_generator<Attribute>(), modifiers_)
                      .generate(sink, context, d, attr);
        }

        // this auto_generator has no attribute attached, it needs to have been
        // initialized from a value/variable
        template <typename OutputIterator, typename Context
          , typename Delimiter>
        static bool
        generate(OutputIterator&, Context&, Delimiter const&, unused_type)
        {
            // It is not possible (doesn't make sense) to use auto_ generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator, auto_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("auto_");
        }

        Modifiers modifiers_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Modifiers>
    struct lit_auto_generator
      : generator<lit_auto_generator<T, Modifiers> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        lit_auto_generator(typename add_reference<T>::type t, Modifiers const& modifiers)
          : t_(t)
          , generator_(compile<karma::domain>(create_generator<T>(), modifiers))
        {}

        // auto_generator has an attached attribute
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const&) const
        {
            return generator_.generate(sink, context, d, t_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("auto_");
        }

        typedef typename spirit::result_of::create_generator<T>::type
            generator_type;

        typedef typename spirit::result_of::compile<
            karma::domain, generator_type, Modifiers>::type generator_impl_type;

        T t_;
        generator_impl_type generator_;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(lit_auto_generator& operator= (lit_auto_generator const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // auto_
    template <typename Modifiers>
    struct make_primitive<tag::auto_, Modifiers>
    {
        typedef auto_generator<Modifiers> result_type;

        result_type operator()(unused_type, Modifiers const& modifiers) const
        {
            return result_type(modifiers);
        }
    };

    // auto_(...)
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::auto_, fusion::vector1<A0> >, Modifiers>
    {
        typedef typename add_const<A0>::type const_attribute;

        typedef lit_auto_generator<const_attribute, Modifiers> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Modifiers const& modifiers) const
        {
            return result_type(fusion::at_c<0>(term.args), modifiers);
        }
    };

}}}

#endif
