//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEXER_TYPE_APR_20_2009_0759PM)
#define BOOST_SPIRIT_LEXER_TYPE_APR_20_2009_0759PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/has_xxx.hpp>
#include <boost/spirit/home/lex/domain.hpp>

namespace boost { namespace spirit { namespace lex
{
    template <typename Derived>
    struct lexer_type
    {
        struct lexer_id;
        typedef Derived derived_type;
        typedef lex::domain domain;

        // Requirement: l.collect(def, state, targetstate) -> void
        //
        //  l:           a lexer component
        //  def:         token definition container
        //  state:       lexer state this token definition needs to be added to
        //  targetstate: an optional lexer state the lexer should be switched 
        //               into after matching this token 

        Derived const& derived() const
        {
            return *static_cast<Derived const*>(this);
        }
    };

    template <typename Derived>
    struct primitive_lexer : lexer_type<Derived>
    {
        struct primitive_lexer_id;
    };

    template <typename Derived>
    struct unary_lexer : lexer_type<Derived>
    {
        struct unary_lexer_id;

        // Requirement: l.subject -> subject lexer component
        //
        // l:   a unary lexer component

        // Requirement: L::subject_type -> subject lexer component type
        //
        // L:   a unary lexer component type
    };

    template <typename Derived>
    struct nary_lexer : lexer_type<Derived>
    {
        struct nary_lexer_id;

        // Requirement: l.elements -> fusion sequence
        //
        // l:   a composite lexer component

        // Requirement: L::elements_type -> fusion sequence
        //
        // L:   a composite lexer component type
    };

}}}

namespace boost { namespace spirit { namespace traits // classification
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(lexer_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(primitive_lexer_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(unary_lexer_id)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(nary_lexer_id)
    }

    template <typename T>
    struct is_lexer : detail::has_lexer_id<T> {};

    template <typename T>
    struct is_primitive_lexer : detail::has_primitive_lexer_id<T> {};

    template <typename T>
    struct is_unary_lexer : detail::has_unary_lexer_id<T> {};

    template <typename T>
    struct is_nary_lexer : detail::has_nary_lexer_id<T> {};

}}}

#endif
