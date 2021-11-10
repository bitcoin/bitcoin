//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_CHAR_CLASS_AUG_10_2009_0720AM)
#define BOOST_SPIRIT_KARMA_CHAR_CLASS_AUG_10_2009_0720AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/char/char_generator.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit 
{ 
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    // enables alnum, alpha, graph, etc.
    template <typename CharClass, typename CharEncoding>
    struct use_terminal<karma::domain
        , tag::char_code<CharClass, CharEncoding> >
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    // hoist the char classification namespaces into karma sub-namespaces of 
    // the same name
    namespace ascii { using namespace boost::spirit::ascii; }
    namespace iso8859_1 { using namespace boost::spirit::iso8859_1; }
    namespace standard { using namespace boost::spirit::standard; }
    namespace standard_wide { using namespace boost::spirit::standard_wide; }
#if defined(BOOST_SPIRIT_UNICODE)
    namespace unicode { using namespace boost::spirit::unicode; }
#endif

    // Import the standard namespace into the karma namespace. This allows 
    // for default handling of all character/string related operations if not 
    // prefixed with a character set namespace.
    using namespace boost::spirit::standard;

    // Import encoding
    using spirit::encoding;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  char_class
    //      generates a single character if it matches the given character 
    //      class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Tag, typename CharEncoding, typename CharClass>
    struct char_class
      : char_generator<
            char_class<Tag, CharEncoding, CharClass>
          , CharEncoding, CharClass>
    {
        typedef typename Tag::char_encoding char_encoding;
        typedef typename char_encoding::char_type char_type;
        typedef typename Tag::char_class classification;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef char_type type;
        };

        // char_class needs an attached attribute
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context&) const
        {
            ch = attr;

            using spirit::char_class::classify;
            return classify<char_encoding>::is(classification(), attr);
        }

        // char_class shouldn't be used without any associated attribute
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam&, Context&) const
        {
            // It is not possible (doesn't make sense) to use char_ generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(CharParam
              , char_class_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            typedef spirit::char_class::what<char_encoding> what_;
            return info(what_::is(classification()));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  space
    //      generates a single character from the associated parameter
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding>
    struct any_space
      : char_generator<any_space<CharEncoding>, CharEncoding, tag::space>
    {
        typedef typename CharEncoding::char_type char_type;
        typedef CharEncoding char_encoding;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef char_type type;
        };

        // any_space has an attached parameter
        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context&) const
        {
            ch = CharParam(attr);

            using spirit::char_class::classify;
            return classify<char_encoding>::is(tag::space(), attr);
        }

        // any_space has no attribute attached, use single space character
        template <typename CharParam, typename Context>
        bool test(unused_type, CharParam& ch, Context&) const
        {
            ch = ' ';
            return true;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("space");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    namespace detail
    {
        template <typename Tag, bool lower = false, bool upper = false>
        struct make_char_class : mpl::identity<Tag> {};

        template <>
        struct make_char_class<tag::alpha, true, false> 
          : mpl::identity<tag::lower> {};

        template <>
        struct make_char_class<tag::alpha, false, true> 
          : mpl::identity<tag::upper> {};

        template <>
        struct make_char_class<tag::alnum, true, false> 
          : mpl::identity<tag::lowernum> {};

        template <>
        struct make_char_class<tag::alnum, false, true> 
          : mpl::identity<tag::uppernum> {};
    }

    // enables alnum, alpha, graph, etc.
    template <typename CharClass, typename CharEncoding, typename Modifiers>
    struct make_primitive<tag::char_code<CharClass, CharEncoding>, Modifiers>
    {
        static bool const lower = 
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper = 
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef tag::char_code<
            typename detail::make_char_class<CharClass, lower, upper>::type
          , CharEncoding>
        tag_type;

        typedef char_class<
            tag_type
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, CharEncoding, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    // space is special
    template <typename CharEncoding, typename Modifiers>
    struct make_primitive<tag::char_code<tag::space, CharEncoding>, Modifiers>
    {
        typedef any_space<CharEncoding> result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

}}}  // namespace boost::spirit::karma

#endif // !defined(BOOST_SPIRIT_KARMA_CHAR_FEB_21_2007_0543PM)
