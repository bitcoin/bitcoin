/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_META_COMPILER_OCTOBER_16_2008_0347PM)
#define BOOST_SPIRIT_META_COMPILER_OCTOBER_16_2008_0347PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/meta_compiler.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/proto/tags.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/at.hpp>

namespace boost { namespace spirit
{
    template <typename T>
    struct use_terminal<qi::domain, T
      , typename enable_if<traits::is_parser<T> >::type> // enables parsers
      : mpl::true_ {};

    namespace qi
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

        template <typename Directive, typename Body
          , typename Modifiers, typename Enable = void>
        struct make_directive
        {
            typedef Body result_type;
            result_type operator()(unused_type, Body const& body, unused_type) const
            {
                return body; // By default, a directive simply returns its subject
            }
        };
    }

    // Qi primitive meta-compiler
    template <>
    struct make_component<qi::domain, proto::tag::terminal>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename qi::make_primitive<
                typename remove_const<typename Elements::car_type>::type,
                typename remove_reference<Modifiers>::type>::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            typedef typename remove_const<typename Elements::car_type>::type term;
            return qi::make_primitive<term, Modifiers>()(elements.car, modifiers);
        }
    };

    // Qi composite meta-compiler
    template <typename Tag>
    struct make_component<qi::domain, Tag>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                qi::make_composite<Tag, Elements,
                typename remove_reference<Modifiers>::type>::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            return qi::make_composite<Tag, Elements, Modifiers>()(
                elements, modifiers);
        }
    };

    // Qi function meta-compiler
    template <>
    struct make_component<qi::domain, proto::tag::function>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                qi::make_composite<
                    typename remove_const<typename Elements::car_type>::type,
                    typename Elements::cdr_type,
                    typename remove_reference<Modifiers>::type
                >::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            return qi::make_composite<
                typename remove_const<typename Elements::car_type>::type,
                typename Elements::cdr_type,
                Modifiers>()(elements.cdr, modifiers);
        }
    };

    // Qi directive meta-compiler
    template <>
    struct make_component<qi::domain, tag::directive>
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Elements, typename Modifiers>
        struct result<This(Elements, Modifiers)>
        {
            typedef typename
                qi::make_directive<
                    typename remove_const<typename Elements::car_type>::type,
                    typename remove_const<typename Elements::cdr_type::car_type>::type,
                    typename remove_reference<Modifiers>::type
                >::result_type
            type;
        };

        template <typename Elements, typename Modifiers>
        typename result<make_component(Elements, Modifiers)>::type
        operator()(Elements const& elements, Modifiers const& modifiers) const
        {
            return qi::make_directive<
                typename remove_const<typename Elements::car_type>::type,
                typename remove_const<typename Elements::cdr_type::car_type>::type,
                Modifiers>()(elements.car, elements.cdr.car, modifiers);
        }
    };

}}

#endif
