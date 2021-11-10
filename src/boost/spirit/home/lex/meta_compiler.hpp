//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_META_COMPILER_APR_20_2009_0756PM)
#define BOOST_SPIRIT_LEX_META_COMPILER_APR_20_2009_0756PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/meta_compiler.hpp>
#include <boost/spirit/home/lex/domain.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/proto/tags.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit
{
    template <typename T>
    struct use_terminal<lex::domain, T
      , typename enable_if<traits::is_lexer<T> >::type> // enables lexers
      : mpl::true_ {};

    namespace lex
    {
        template <typename T, typename Modifiers, typename Enable = void>
        struct make_primitive // by default, return it as-is
        {
            typedef T result_type;

            template <typename T_>
            T_& operator()(T_& val, unused_type) const
            {
                return val;
            }

            template <typename T_>
            T_ const& operator()(T_ const& val, unused_type) const
            {
                return val;
            }
        };

        template <typename Tag, typename Elements
          , typename Modifiers, typename Enable = void>
        struct make_composite;
    }

    // Lex primitive meta-compiler
    template <>
    struct make_component<lex::domain, proto::tag::terminal>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename lex::make_primitive<
                typename remove_const<typename Elements::car_type>::type,
                typename remove_reference<Modifiers>::type>::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            typedef typename remove_const<typename Elements::car_type>::type term;
            return lex::make_primitive<term, Modifiers>()(elements.car, modifiers);
        }
    };

    // Lex composite meta-compiler
    template <typename Tag>
    struct make_component<lex::domain, Tag>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                lex::make_composite<Tag, Elements
                  , typename remove_reference<Modifiers>::type>::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            return lex::make_composite<Tag, Elements, Modifiers>()(
                elements, modifiers);
        }
    };

}}

#endif
