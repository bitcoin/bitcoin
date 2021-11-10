/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_LIT_APR_18_2006_1125PM)
#define BOOST_SPIRIT_LIT_APR_18_2006_1125PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/detail/string_parse.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/detail/enable_lit.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/utility/enable_if.hpp>
#include <string>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct use_terminal<qi::domain, T
      , typename enable_if<traits::is_string<T> >::type> // enables strings
      : mpl::true_ {};

    template <typename CharEncoding, typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<
            tag::char_code<tag::string, CharEncoding>   // enables string(str)
          , fusion::vector1<A0> >
    > : traits::is_string<A0> {};

    template <typename CharEncoding>                    // enables string(f)
    struct use_lazy_terminal<
        qi::domain
      , tag::char_code<tag::string, CharEncoding>
      , 1 /*arity*/
    > : mpl::true_ {};

    // enables lit(...)
    template <typename A0>
    struct use_terminal<qi::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> >
          , typename enable_if<traits::is_string<A0> >::type>
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::lit;
#endif
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    // Parse for literal strings
    ///////////////////////////////////////////////////////////////////////////
    template <typename String, bool no_attribute>
    struct literal_string
      : primitive_parser<literal_string<String, no_attribute> >
    {
        typedef typename
            remove_const<typename traits::char_type_of<String>::type>::type
        char_type;
        typedef std::basic_string<char_type> string_type;

        literal_string(typename add_reference<String>::type str_)
          : str(str_)
        {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename mpl::if_c<
                no_attribute, unused_type, string_type>::type
            type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper, Attribute& attr_) const
        {
            qi::skip_over(first, last, skipper);
            return detail::string_parse(str, first, last, attr_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("literal-string", str);
        }

        String str;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(literal_string& operator= (literal_string const&))
    };

    template <typename String, bool no_attribute>
    struct no_case_literal_string
      : primitive_parser<no_case_literal_string<String, no_attribute> >
    {
        typedef typename
            remove_const<typename traits::char_type_of<String>::type>::type
        char_type;
        typedef std::basic_string<char_type> string_type;

        template <typename CharEncoding>
        no_case_literal_string(char_type const* in, CharEncoding encoding)
          : str_lo(in)
          , str_hi(in)
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            encoding; // suppresses warning: C4100: 'encoding' : unreferenced formal parameter
#endif
            typename string_type::iterator loi = str_lo.begin();
            typename string_type::iterator hii = str_hi.begin();

            for (; loi != str_lo.end(); ++loi, ++hii, ++in)
            {
                typedef typename CharEncoding::char_type encoded_char_type;

                *loi = static_cast<char_type>(encoding.tolower(encoded_char_type(*loi)));
                *hii = static_cast<char_type>(encoding.toupper(encoded_char_type(*hii)));
            }
        }

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename mpl::if_c<
                no_attribute, unused_type, string_type>::type
            type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper, Attribute& attr_) const
        {
            qi::skip_over(first, last, skipper);
            return detail::string_parse(str_lo, str_hi, first, last, attr_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("no-case-literal-string", str_lo);
        }

        string_type str_lo, str_hi;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Modifiers>
    struct make_primitive<T, Modifiers
      , typename enable_if<traits::is_string<T> >::type>
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;

        typedef typename add_const<T>::type const_string;
        typedef typename mpl::if_<
            no_case
          , no_case_literal_string<const_string, true>
          , literal_string<const_string, true> >::type
        result_type;

        result_type operator()(
            typename add_reference<const_string>::type str, unused_type) const
        {
            return op(str, no_case());
        }

        template <typename String>
        result_type op(String const& str, mpl::false_) const
        {
            return result_type(str);
        }

        template <typename String>
        result_type op(String const& str, mpl::true_) const
        {
            typename spirit::detail::get_encoding<Modifiers,
                spirit::char_encoding::standard>::type encoding;
            return result_type(traits::get_c_string(str), encoding);
        }
    };

    // lit("...")
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::lit, fusion::vector1<A0> >
      , Modifiers
      , typename enable_if<traits::is_string<A0> >::type>
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;

        typedef typename add_const<A0>::type const_string;
        typedef typename mpl::if_<
            no_case
          , no_case_literal_string<const_string, true>
          , literal_string<const_string, true> >::type
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return op(fusion::at_c<0>(term.args), no_case());
        }

        template <typename String>
        result_type op(String const& str, mpl::false_) const
        {
            return result_type(str);
        }

        template <typename String>
        result_type op(String const& str, mpl::true_) const
        {
            typedef typename traits::char_encoding_from_char<
                typename traits::char_type_of<A0>::type>::type encoding_type;
            typename spirit::detail::get_encoding<Modifiers,
                encoding_type>::type encoding;
            return result_type(traits::get_c_string(str), encoding);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // string("...")
    template <typename CharEncoding, typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector1<A0> >
      , Modifiers>
    {
        typedef CharEncoding encoding;
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;

        typedef typename add_const<A0>::type const_string;
        typedef typename mpl::if_<
            no_case
          , no_case_literal_string<const_string, false>
          , literal_string<const_string, false> >::type
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return op(fusion::at_c<0>(term.args), no_case());
        }

        template <typename String>
        result_type op(String const& str, mpl::false_) const
        {
            return result_type(str);
        }

        template <typename String>
        result_type op(String const& str, mpl::true_) const
        {
            return result_type(traits::get_c_string(str), encoding());
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename String, bool no_attribute, typename Attribute
      ,typename Context, typename Iterator>
    struct handles_container<qi::literal_string<String, no_attribute>
      , Attribute, Context, Iterator>
      : mpl::true_ {};

    template <typename String, bool no_attribute, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<qi::no_case_literal_string<String, no_attribute>
      , Attribute, Context, Iterator>
      : mpl::true_ {};
}}}

#endif
