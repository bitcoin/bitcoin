//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c)      2010 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_CHAR_FEB_21_2007_0543PM)
#define BOOST_SPIRIT_KARMA_CHAR_FEB_21_2007_0543PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/char_set/basic_chset.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/char/char_generator.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/enable_lit.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding>
    struct use_terminal<karma::domain
      , tag::char_code<tag::char_, CharEncoding>        // enables char_
    > : mpl::true_ {};

    template <typename CharEncoding, typename A0>
    struct use_terminal<karma::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>    // enables char_('x'), char_("x")
          , fusion::vector1<A0>
        >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> > // enables lit('x')
          , typename enable_if<traits::is_char<A0> >::type>
      : mpl::true_ {};

    template <typename CharEncoding, typename A0, typename A1>
    struct use_terminal<karma::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>    // enables char_('a','z')
          , fusion::vector2<A0, A1>
        >
    > : mpl::true_ {};

    template <typename CharEncoding>                    // enables *lazy* char_('x'), char_("x")
    struct use_lazy_terminal<
        karma::domain
      , tag::char_code<tag::char_, CharEncoding>
      , 1 // arity
    > : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, char>            // enables 'x'
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, char[2]>         // enables "x"
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, wchar_t>         // enables L'x'
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, wchar_t[2]>      // enables L"x"
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::lit;    // lit('x') is equivalent to 'x'
#endif
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  any_char
    //      generates a single character from the associated attribute
    //
    //      Note: this generator has to have an associated attribute
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, typename Tag>
    struct any_char
      : char_generator<any_char<CharEncoding, Tag>, CharEncoding, Tag>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef char_type type;
        };

        // any_char has an attached parameter
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context&) const
        {
            ch = CharParam(attr);
            return true;
        }

        // any_char has no attribute attached, it needs to have been
        // initialized from a direct literal
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam&, Context&) const
        {
            // It is not possible (doesn't make sense) to use char_ without
            // providing any attribute, as the generator doesn't 'know' what
            // character to output. The following assertion fires if this
            // situation is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(CharParam, char_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("any-char");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  literal_char
    //      generates a single character given by a literal it was initialized
    //      from
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding, typename Tag, bool no_attribute>
    struct literal_char
      : char_generator<literal_char<CharEncoding, Tag, no_attribute>
          , CharEncoding, Tag>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        literal_char(char_type ch)
          : ch (spirit::char_class::convert<char_encoding>::to(Tag(), ch)) 
        {}

        template <typename Context, typename Unused>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, char_type>
        {};

        // A char_('x') which additionally has an associated attribute emits
        // its immediate literal only if it matches the attribute, otherwise
        // it fails.
        // any_char has an attached parameter
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch_, Context&) const
        {
            // fail if attribute isn't matched my immediate literal
            ch_ = static_cast<char_type>(attr);
            return attr == ch;
        }

        // A char_('x') without any associated attribute just emits its 
        // immediate literal
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam& ch_, Context&) const
        {
            ch_ = ch;
            return true;
        }

        template <typename Context>
        info what(Context const& /*context*/) const
        {
            return info("literal-char", char_encoding::toucs4(ch));
        }

        char_type ch;
    };

    ///////////////////////////////////////////////////////////////////////////
    // char range generator
    template <typename CharEncoding, typename Tag>
    struct char_range
      : char_generator<char_range<CharEncoding, Tag>, CharEncoding, Tag>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        char_range(char_type from, char_type to)
          : from(spirit::char_class::convert<char_encoding>::to(Tag(), from))
          , to(spirit::char_class::convert<char_encoding>::to(Tag(), to)) 
        {}

        // A char_('a', 'z') which has an associated attribute emits it only if 
        // it matches the character range, otherwise it fails.
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context&) const
        {
            // fail if attribute doesn't belong to character range
            ch = attr;
            return (from <= char_type(attr)) && (char_type(attr) <= to);
        }

        // A char_('a', 'z') without any associated attribute fails compiling
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam&, Context&) const
        {
            // It is not possible (doesn't make sense) to use char_ generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(CharParam
              , char_range_not_usable_without_attribute, ());
            return false;
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

    ///////////////////////////////////////////////////////////////////////////
    // character set generator
    template <typename CharEncoding, typename Tag, bool no_attribute>
    struct char_set
      : char_generator<char_set<CharEncoding, Tag, no_attribute>
          , CharEncoding, Tag>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename Context, typename Unused>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, char_type>
        {};

        template <typename String>
        char_set(String const& str)
        {
            typedef typename traits::char_type_of<String>::type in_type;

            BOOST_SPIRIT_ASSERT_MSG((
                (sizeof(char_type) == sizeof(in_type))
            ), cannot_convert_string, (String));

            typedef spirit::char_class::convert<char_encoding> convert_type;

            char_type const* definition =
                (char_type const*)traits::get_c_string(str);
            char_type ch = convert_type::to(Tag(), *definition++);
            while (ch)
            {
                char_type next = convert_type::to(Tag(), *definition++);
                if (next == '-')
                {
                    next = convert_type::to(Tag(), *definition++);
                    if (next == 0)
                    {
                        chset.set(ch);
                        chset.set('-');
                        break;
                    }
                    chset.set(ch, next);
                }
                else
                {
                    chset.set(ch);
                }
                ch = next;
            }
        }

        // A char_("a-z") which has an associated attribute emits it only if 
        // it matches the character set, otherwise it fails.
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context&) const
        {
            // fail if attribute doesn't belong to character set
            ch = attr;
            return chset.test(char_type(attr));
        }

        // A char_("a-z") without any associated attribute fails compiling
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam&, Context&) const
        {
            // It is not possible (doesn't make sense) to use char_ generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(CharParam
               , char_set_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("char-set");
        }

        support::detail::basic_chset<char_type> chset;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Modifiers, typename Encoding>
        struct basic_literal
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_char<
                typename spirit::detail::get_encoding_with_case<
                    Modifiers, Encoding, lower || upper>::type
              , typename get_casetag<Modifiers, lower || upper>::type
              , true>
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

    // literals: 'x', "x"
    template <typename Modifiers>
    struct make_primitive<char, Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard> {};

    template <typename Modifiers>
    struct make_primitive<char const(&)[2], Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard> {};

    // literals: L'x', L"x"
    template <typename Modifiers>
    struct make_primitive<wchar_t, Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard_wide> {};

    template <typename Modifiers>
    struct make_primitive<wchar_t const(&)[2], Modifiers>
      : detail::basic_literal<Modifiers, char_encoding::standard_wide> {};

    // char_
    template <typename CharEncoding, typename Modifiers>
    struct make_primitive<tag::char_code<tag::char_, CharEncoding>, Modifiers>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef any_char<
            typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename CharEncoding, typename Modifiers, typename A0
          , bool no_attribute>
        struct make_char_direct
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type encoding;
            typedef typename detail::get_casetag<
                Modifiers, lower || upper>::type tag;

            typedef typename mpl::if_<
                traits::is_string<A0>
              , char_set<encoding, tag, no_attribute>
              , literal_char<encoding, tag, no_attribute>
            >::type result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                return result_type(fusion::at_c<0>(term.args));
            }
        };
    }

    // char_(...), lit(...)
    template <typename CharEncoding, typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<
                tag::char_code<tag::char_, CharEncoding>
              , fusion::vector1<A0> >
          , Modifiers>
      : detail::make_char_direct<CharEncoding, Modifiers, A0, false>
    {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::lit, fusion::vector1<A0> >
          , Modifiers
          , typename enable_if<traits::is_char<A0> >::type>
      : detail::make_char_direct<
            typename traits::char_encoding_from_char<
                typename traits::char_type_of<A0>::type>::type
          , Modifiers, A0, true>
    {};

    ///////////////////////////////////////////////////////////////////////////
    // char_("x")
    template <typename CharEncoding, typename Modifiers, typename Char>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<Char(&)[2]> > // For single char strings
      , Modifiers>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef literal_char<
            typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
          , false
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args)[0]);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // char_('a', 'z')
    template <typename CharEncoding, typename Modifiers, typename A0, typename A1>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector2<A0, A1>
        >
      , Modifiers>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef char_range<
            typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args)
              , fusion::at_c<1>(term.args));
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
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef char_range<
            typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args)[0]
              , fusion::at_c<1>(term.args)[0]);
        }
    };
}}}   // namespace boost::spirit::karma

#endif
