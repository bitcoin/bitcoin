/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_CHAR_CLASS_APRIL_16_2006_1051AM)
#define BOOST_SPIRIT_CHAR_CLASS_APRIL_16_2006_1051AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/char/char_parser.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/mpl/eval_if.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    // enables alnum, alpha, graph, etc.
    template <typename CharClass, typename CharEncoding>
    struct use_terminal<qi::domain, tag::char_code<CharClass, CharEncoding> >
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    // hoist the char classification namespaces into qi sub-namespaces of the
    // same name
    namespace ascii { using namespace boost::spirit::ascii; }
    namespace iso8859_1 { using namespace boost::spirit::iso8859_1; }
    namespace standard { using namespace boost::spirit::standard; }
    namespace standard_wide { using namespace boost::spirit::standard_wide; }
#if defined(BOOST_SPIRIT_UNICODE)
    namespace unicode { using namespace boost::spirit::unicode; }
#endif

    // Import the standard namespace into the qi namespace. This allows
    // for default handling of all character/string related operations if not
    // prefixed with a character set namespace.
    using namespace boost::spirit::standard;

    // Import encoding
    using spirit::encoding;

    ///////////////////////////////////////////////////////////////////////////
    // Generic char classification parser (for alnum, alpha, graph, etc.)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Tag>
    struct char_class
      : char_parser<char_class<Tag>, typename Tag::char_encoding::char_type>
    {
        typedef typename Tag::char_encoding char_encoding;
        typedef typename Tag::char_class classification;

        template <typename CharParam, typename Context>
        bool test(CharParam ch, Context&) const
        {
            using spirit::char_class::classify;
            return traits::ischar<CharParam, char_encoding>::call(ch) &&
                   classify<char_encoding>::is(classification(), ch);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            typedef spirit::char_class::what<char_encoding> what_;
            return info(what_::is(classification()));
        }
    };

    namespace detail
    {
        template <typename Tag, bool no_case = false>
        struct make_char_class : mpl::identity<Tag> {};

        template <>
        struct make_char_class<tag::lower, true> : mpl::identity<tag::alpha> {};

        template <>
        struct make_char_class<tag::upper, true> : mpl::identity<tag::alpha> {};
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharClass, typename CharEncoding, typename Modifiers>
    struct make_primitive<tag::char_code<CharClass, CharEncoding>, Modifiers>
    {
        static bool const no_case =
            has_modifier<Modifiers, tag::char_code_base<tag::no_case> >::value;

        typedef typename
            spirit::detail::get_encoding<Modifiers, CharEncoding>::type
        char_encoding;

        typedef tag::char_code<
            typename detail::make_char_class<CharClass, no_case>::type
          , char_encoding>
        tag;

        typedef char_class<tag> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };
}}}

#endif
