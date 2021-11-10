//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_EPS_APRIL_21_2007_0246PM)
#define BOOST_SPIRIT_KARMA_EPS_APRIL_21_2007_0246PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/fusion/include/at.hpp>

namespace boost { namespace spirit 
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables eps
    template <>
    struct use_terminal<karma::domain, tag::eps>
      : mpl::true_ {};

    // enables eps(bool-condition)
    template <typename A0>
    struct use_terminal<karma::domain
        , terminal_ex<tag::eps, fusion::vector1<A0> > > 
      : is_convertible<A0, bool> {};

    // enables lazy eps(f)
    template <>
    struct use_lazy_terminal<karma::domain, tag::eps, 1>
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using boost::spirit::eps;
#endif
    using boost::spirit::eps_type;

    struct eps_generator : primitive_generator<eps_generator>
    {
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        static bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , Attribute const& /*attr*/)
        {
            return karma::delimit_out(sink, d); // always do post-delimiting
        }

        template <typename Context>
        info what(Context const& /*context*/) const
        {
            return info("eps");
        }
    };

    struct semantic_predicate : primitive_generator<semantic_predicate>
    {
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        semantic_predicate(bool predicate)
          : predicate_(predicate) 
        {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , Attribute const& /*attr*/) const
        {
            // only do post-delimiting when predicate is true
            return predicate_ && karma::delimit_out(sink, d);
        }

        template <typename Context>
        info what(Context const& /*context*/) const
        {
            return info("semantic-predicate");
        }

        bool predicate_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::eps, Modifiers>
    {
        typedef eps_generator result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::eps, fusion::vector1<A0> >
      , Modifiers>
    {
        typedef semantic_predicate result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

}}}

#endif
