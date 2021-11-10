/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_OCTOBER_16_2008_0254PM)
#define BOOST_SPIRIT_PARSER_OCTOBER_16_2008_0254PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/has_xxx.hpp>
#include <boost/spirit/home/qi/domain.hpp>

namespace boost { namespace spirit { namespace qi
{

    //[parser_base_parser
    template <typename Derived>
    struct parser
    {
        struct parser_id;
        typedef Derived derived_type;
        typedef qi::domain domain;

        // Requirement: p.parse(f, l, context, skip, attr) -> bool
        //
        //  p:          a parser
        //  f, l:       first/last iterator pair
        //  context:    enclosing rule context (can be unused_type)
        //  skip:       skipper (can be unused_type)
        //  attr:       attribute (can be unused_type)

        // Requirement: p.what(context) -> info
        //
        //  p:          a parser
        //  context:    enclosing rule context (can be unused_type)

        // Requirement: P::template attribute<Ctx, Iter>::type
        //
        //  P:          a parser type
        //  Ctx:        A context type (can be unused_type)
        //  Iter:       An iterator type (can be unused_type)

        Derived const& derived() const
        {
            return *static_cast<Derived const*>(this);
        }
    };
    //]

    template <typename Derived>
    struct primitive_parser : parser<Derived>
    {
        struct primitive_parser_id;
    };

    template <typename Derived>
    struct nary_parser : parser<Derived>
    {
        struct nary_parser_id;

        // Requirement: p.elements -> fusion sequence
        //
        // p:   a composite parser

        // Requirement: P::elements_type -> fusion sequence
        //
        // P:   a composite parser type
    };

    template <typename Derived>
    struct unary_parser : parser<Derived>
    {
        struct unary_parser_id;

        // Requirement: p.subject -> subject parser
        //
        // p:   a unary parser

        // Requirement: P::subject_type -> subject parser type
        //
        // P:   a unary parser type
    };

    template <typename Derived>
    struct binary_parser : parser<Derived>
    {
        struct binary_parser_id;

        // Requirement: p.left -> left parser
        //
        // p:   a binary parser

        // Requirement: P::left_type -> left parser type
        //
        // P:   a binary parser type

        // Requirement: p.right -> right parser
        //
        // p:   a binary parser

        // Requirement: P::right_type -> right parser type
        //
        // P:   a binary parser type
    };
}}}

namespace boost { namespace spirit { namespace traits // classification
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(parser_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(primitive_parser_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(nary_parser_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(unary_parser_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(binary_parser_id)
    }

    // parser type identification
    template <typename T>
    struct is_parser : detail::has_parser_id<T> {};

    template <typename T>
    struct is_primitive_parser : detail::has_primitive_parser_id<T> {};

    template <typename T>
    struct is_nary_parser : detail::has_nary_parser_id<T> {};

    template <typename T>
    struct is_unary_parser : detail::has_unary_parser_id<T> {};

    template <typename T>
    struct is_binary_parser : detail::has_binary_parser_id<T> {};

}}}

#endif
