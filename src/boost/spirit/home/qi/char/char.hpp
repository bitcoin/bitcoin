/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_CHAR_APRIL_16_2006_1051AM)
#define BOOST_SPIRIT_CHAR_APRIL_16_2006_1051AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/char_set/basic_chset.hpp>
#include <boost/spirit/home/qi/char/char_parser.hpp>
#include <boost/spirit/home/qi/char/char_class.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/detail/enable_lit.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <string>

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding>
    struct use_terminal<qi::domain
      , terminal<
            tag::char_code<tag::char_, CharEncoding>    // enables char_
        >
    > : mpl::true_ {};

    template <typename CharEncoding, typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>    // enables char_('x'), char_("x")
          , fusion::vector1<A0>                         // and char_("a-z0-9")
        >
    > : mpl::true_ {};

    template <typename CharEncoding, typename A0, typename A1>
    struct use_terminal<qi::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>    // enables char_('a','z')
          , fusion::vector2<A0, A1>
        >
    > : mpl::true_ {};

    template <typename CharEncoding>                    // enables *lazy* char_('x'), char_("x")
    struct use_lazy_terminal<                           // and char_("a-z0-9")
        qi::domain
      , tag::char_code<tag::char_, CharEncoding>
      , 1 // arity
    > : mpl::true_ {};

    template <typename CharEncoding>                    // enables *lazy* char_('a','z')
    struct use_lazy_terminal<
        qi::domain
      , tag::char_code<tag::char_, CharEncoding>
      , 2 // arity
    > : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, char>               // enables 'x'
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, char[2]>            // enables "x"
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, wchar_t>            // enables wchar_t
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, wchar_t[2]>         // enables L"x"
      : mpl::true_ {};

    // enables lit(...)
    template <typename A0>
    struct use_terminal<qi::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> >
          , typename enable_if<traits::is_char<A0> >::type>
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::lit; // lit('x') is equivalent to 'x'
#endif
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    // Parser for a single character
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, bool no_attribute, bool no_case = false>
    struct literal_char
      : char_parser<
            literal_char<CharEncoding, no_attribute, false>
          , typename CharEncoding::char_type
          , typename mpl::if_c<no_attribute, unused_type
              , typename CharEncoding::char_type>::type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename Char>
        literal_char(Char ch_)
          : ch(static_cast<char_type>(ch_)) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename mpl::if_c<
                no_attribute, unused_type, char_type>::type
            type;
        };

        template <typename CharParam, typename Context>
        bool test(CharParam ch_, Context&) const
        {
            return traits::ischar<CharParam, char_encoding>::call(ch_) &&
                   ch == char_type(ch_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("literal-char", char_encoding::toucs4(ch));
        }

        char_type ch;
    };

    template <typename CharEncoding, bool no_attribute>
    struct literal_char<CharEncoding, no_attribute, true> // case insensitive
      : char_parser<
            literal_char<CharEncoding, no_attribute, true>
          , typename mpl::if_c<no_attribute, unused_type
              , typename CharEncoding::char_type>::type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        literal_char(char_type ch)
          : lo(static_cast<char_type>(char_encoding::tolower(ch)))
          , hi(static_cast<char_type>(char_encoding::toupper(ch))) {}

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename mpl::if_c<
                no_attribute, unused_type, char_type>::type
            type;
        };

        template <typename CharParam, typename Context>
        bool test(CharParam ch_, Context&) const
        {
            if (!traits::ischar<CharParam, char_encoding>::call(ch_))
                return false;

            char_type ch = char_type(ch_);  // optimize for token based parsing
            return this->lo == ch || this->hi == ch;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("no-case-literal-char", char_encoding::toucs4(lo));
        }

        char_type lo, hi;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser for a character range
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, bool no_case = false>
    struct char_range
      : char_parser<char_range<CharEncoding, false>, typename CharEncoding::char_type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        char_range(char_type from_, char_type to_)
          : from(from_), to(to_) {}

        template <typename CharParam, typename Context>
        bool test(CharParam ch_, Context&) const
        {
            if (!traits::ischar<CharParam, char_encoding>::call(ch_))
                return false;

            char_type ch = char_type(ch_);  // optimize for token based parsing
            return !(ch < from) && !(to < ch);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            info result("char-range", char_encoding::toucs4(from));
            boost::get<std::string>(result.value) += '-';
            boost::get<std::string>(result.value) += to_utf8(char_encoding::toucs4(to));
            return result;
        }

        char_type from, to;
    };

    template <typename CharEncoding>
    struct char_range<CharEncoding, true> // case insensitive
      : char_parser<char_range<CharEncoding, true>, typename CharEncoding::char_type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        char_range(char_type from, char_type to)
          : from_lo(static_cast<char_type>(char_encoding::tolower(from)))
          , to_lo(static_cast<char_type>(char_encoding::tolower(to)))
          , from_hi(static_cast<char_type>(char_encoding::toupper(from)))
          , to_hi(static_cast<char_type>(char_encoding::toupper(to)))
        {}

        template <typename CharParam, typename Context>
        bool test(CharParam ch_, Context&) const
        {
            if (!traits::ischar<CharParam, char_encoding>::call(ch_))
                return false;

            char_type ch = char_type(ch_);  // optimize for token based parsing
            return (!(ch < from_lo) && !(to_lo < ch))
                || (!(ch < from_hi) && !(to_hi < ch))
            ;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            info result("no-case-char-range", char_encoding::toucs4(from_lo));
            boost::get<std::string>(result.value) += '-';
            boost::get<std::string>(result.value) += to_utf8(char_encoding::toucs4(to_lo));
            return result;
        }

        char_type from_lo, to_lo, from_hi, to_hi;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser for a character set
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, bool no_attribute, bool no_case = false>
    struct char_set
      : char_parser<char_set<CharEncoding, no_attribute, false>
          , typename mpl::if_c<no_attribute, unused_type
              , typename CharEncoding::char_type>::type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename String>
        char_set(String const& str)
        {
            using spirit::detail::cast_char;

            typedef typename
                remove_const<
                    typename traits::char_type_of<String>::type
                >::type
            in_type;

            BOOST_SPIRIT_ASSERT_MSG((
                (sizeof(char_type) >= sizeof(in_type))
            ), cannot_convert_string, (String));

            in_type const* definition =
                (in_type const*)traits::get_c_string(str);
            in_type ch = *definition++;
            while (ch)
            {
                in_type next = *definition++;
                if (next == '-')
                {
                    next = *definition++;
                    if (next == 0)
                    {
                        chset.set(cast_char<char_type>(ch));
                        chset.set('-');
                        break;
                    }
                    chset.set(
                        cast_char<char_type>(ch),
                        cast_char<char_type>(next)
                    );
                }
                else
                {
                    chset.set(cast_char<char_type>(ch));
                }
                ch = next;
            }
        }

        template <typename CharParam, typename Context>
        bool test(CharParam ch, Context&) const
        {
            return traits::ischar<CharParam, char_encoding>::call(ch) &&
                   chset.test(char_type(ch));
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("char-set");
        }

        support::detail::basic_chset<char_type> chset;
    };

    template <typename CharEncoding, bool no_attribute>
    struct char_set<CharEncoding, no_attribute, true> // case insensitive
      : char_parser<char_set<CharEncoding, no_attribute, true>
          , typename mpl::if_c<no_attribute, unused_type
              , typename CharEncoding::char_type>::type>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename String>
        char_set(String const& str)
        {
            typedef typename traits::char_type_of<String>::type in_type;

            BOOST_SPIRIT_ASSERT_MSG((
                (sizeof(char_type) == sizeof(in_type))
            ), cannot_convert_string, (String));

            char_type const* definition =
                (char_type const*)traits::get_c_string(str);
            char_type ch = *definition++;
            while (ch)
            {
                char_type next = *definition++;
                if (next == '-')
                {
                    next = *definition++;
                    if (next == 0)
                    {
                        chset.set(static_cast<char_type>(CharEncoding::tolower(ch)));
                        chset.set(static_cast<char_type>(CharEncoding::toupper(ch)));
                        chset.set('-');
                        break;
                    }
                    chset.set(static_cast<char_type>(CharEncoding::tolower(ch))
                      , static_cast<char_type>(CharEncoding::tolower(next)));
                    chset.set(static_cast<char_type>(CharEncoding::toupper(ch))
                      , static_cast<char_type>(CharEncoding::toupper(next)));
                }
                else
                {
                    chset.set(static_cast<char_type>(CharEncoding::tolower(ch)));
                    chset.set(static_cast<char_type>(CharEncoding::toupper(ch)));
                }
                ch = next;
            }
        }

        template <typename CharParam, typename Context>
        bool test(CharParam ch, Context&) const
        {
            return traits::ischar<CharParam, char_encoding>::call(ch) &&
                   chset.test(char_type(ch));
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("no-case-char-set");
        }

        support::detail::basic_chset<char_type> chset;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Modifiers, typename Encoding>
        struct basic_literal
        {
            static bool const no_case =
                has_modifier<
                    Modifiers
                  , tag::char_code_base<tag::no_case>
                >::value;

            static bool const no_attr =
                !has_modifier<
                    Modifiers
                  , tag::lazy_eval
                >::value;

            typedef literal_char<
                typename spirit::detail::get_encoding_with_case<
                    Modifiers, Encoding, no_case>::type
              , no_attr
              , no_case>
            result_type;

            template <typename Char>
            result_type operator()(Char ch, unused_type) const
            {
                return result_type(ch);
            }

            template <typename Char>
            result_type operator()(Char const* str, unused_type) const
            {
                return result_type(str[0]);
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<char, Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard> {};

    template <typename Modifiers>
    struct make_primitive<char const(&)[2], Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard> {};

    template <typename Modifiers>
    struct make_primitive<wchar_t, Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard_wide> {};

    template <typename Modifiers>
    struct make_primitive<wchar_t const(&)[2], Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard_wide> {};

    template <typename CharEncoding, typename Modifiers>
    struct make_primitive<
        terminal<tag::char_code<tag::char_, CharEncoding> >, Modifiers>
    {
        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef tag::char_code<tag::char_, char_encoding> tag;
        typedef char_class<tag> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // char_('x')
    template <typename CharEncoding, typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<A0> >
      , Modifiers>
    {
        static bool const no_case =
            has_modifier<Modifiers, tag::char_code_base<tag::no_case> >::value;

        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef typename
            mpl::if_<
                traits::is_string<A0>
              , char_set<char_encoding, false, no_case>
              , literal_char<char_encoding, false, no_case>
            >::type
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    // lit('x')
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::lit, fusion::vector1<A0> >
      , Modifiers
      , typename enable_if<traits::is_char<A0> >::type>
    {
        static bool const no_case =
            has_modifier<
                Modifiers
              , tag::char_code_base<tag::no_case>
            >::value;

        typedef typename traits::char_encoding_from_char<
                typename traits::char_type_of<A0>::type>::type encoding;

        typedef literal_char<
            typename spirit::detail::get_encoding_with_case<
                Modifiers, encoding, no_case>::type
          , true, no_case>
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, typename Modifiers, typename Char>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<Char(&)[2]> // For single char strings
        >
      , Modifiers>
    {
        static bool const no_case =
            has_modifier<Modifiers, tag::char_code_base<tag::no_case> >::value;

        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef literal_char<char_encoding, false, no_case> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args)[0]);
        }
    };

    template <typename CharEncoding, typename Modifiers, typename A0, typename A1>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<A0, A1>
        >
      , Modifiers>
    {
        static bool const no_case =
            has_modifier<Modifiers, tag::char_code_base<tag::no_case> >::value;

        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef char_range<char_encoding, no_case> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)
              , fusion::at_c<1>(term.args)
            );
        }
    };

    template <typename CharEncoding, typename Modifiers, typename Char>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<Char(&)[2], Char(&)[2]> // For single char strings
        >
      , Modifiers>
    {
        static bool const no_case =
            has_modifier<Modifiers, tag::char_code_base<tag::no_case> >::value;

        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef char_range<char_encoding, no_case> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(term.args)[0]
              , fusion::at_c<1>(term.args)[0]
            );
        }
    };
}}}

#endif
