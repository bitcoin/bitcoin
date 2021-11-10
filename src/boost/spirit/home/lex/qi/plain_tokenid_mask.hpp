//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_PLAIN_TOKENID_MASK_JUN_03_2011_0929PM)
#define BOOST_SPIRIT_LEX_PLAIN_TOKENID_MASK_JUN_03_2011_0929PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/or.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <iterator> // for std::iterator_traits
#include <sstream>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables tokenid_mask(id)
    template <typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<tag::tokenid_mask, fusion::vector1<A0> >
    > : mpl::or_<is_integral<A0>, is_enum<A0> > {};

    // enables *lazy* tokenid_mask(id)
    template <>
    struct use_lazy_terminal<
        qi::domain, tag::tokenid_mask, 1
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::tokenid_mask;
#endif
    using spirit::tokenid_mask_type;

    ///////////////////////////////////////////////////////////////////////////
    // The plain_tokenid represents a simple token defined by the lexer inside
    // a Qi grammar. The difference to plain_token is that it exposes the
    // matched token id instead of the iterator_range of the matched input.
    // Additionally it applies the given mask to the matched token id.
    template <typename Mask>
    struct plain_tokenid_mask
      : primitive_parser<plain_tokenid_mask<Mask> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef Mask type;
        };

        plain_tokenid_mask(Mask const& mask)
          : mask(mask) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);   // always do a pre-skip

            if (first != last) {
                // simply match the token id with the mask this component has
                // been initialized with

                typedef typename
                    std::iterator_traits<Iterator>::value_type
                token_type;
                typedef typename token_type::id_type id_type;

                token_type const& t = *first;
                if ((t.id() & mask) == id_type(mask))
                {
                    spirit::traits::assign_to(t.id(), attr);
                    ++first;
                    return true;
                }
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            std::stringstream ss;
            ss << "tokenid_mask(" << mask << ")";
            return info("tokenid_mask", ss.str());
        }

        Mask mask;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename Mask>
    struct make_primitive<terminal_ex<tag::tokenid_mask, fusion::vector1<Mask> >
      , Modifiers>
    {
        typedef plain_tokenid_mask<Mask> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template<typename Mask, typename Attr, typename Context, typename Iterator>
    struct handles_container<qi::plain_tokenid_mask<Mask>, Attr, Context, Iterator>
      : mpl::true_
    {};
}}}

#endif
