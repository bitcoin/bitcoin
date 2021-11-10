/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_CHAR_CLASS_APRIL_16_2006_1051AM)
#define BOOST_SPIRIT_X3_CHAR_CLASS_APRIL_16_2006_1051AM

#include <boost/spirit/home/x3/char/char_parser.hpp>
#include <boost/spirit/home/x3/char/detail/cast_char.hpp>
#include <boost/spirit/home/support/char_encoding/standard.hpp>
#include <boost/spirit/home/support/char_encoding/standard_wide.hpp>
#include <boost/spirit/home/support/char_encoding/ascii.hpp>
#include <boost/spirit/home/support/char_encoding/iso8859_1.hpp>
#include <boost/spirit/home/x3/char/char_class_tags.hpp>
namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Encoding>
    struct char_class_base
    {
        typedef typename Encoding::classify_type char_type;

#define BOOST_SPIRIT_X3_CLASSIFY(name)                                          \
        template <typename Char>                                                \
        static bool                                                             \
        is(name##_tag, Char ch)                                                 \
        {                                                                       \
            return Encoding::is##name                                           \
                BOOST_PREVENT_MACRO_SUBSTITUTION                                \
                    (detail::cast_char<char_type>(ch));                         \
        }                                                                       \
        /***/

        BOOST_SPIRIT_X3_CLASSIFY(char)
        BOOST_SPIRIT_X3_CLASSIFY(alnum)
        BOOST_SPIRIT_X3_CLASSIFY(alpha)
        BOOST_SPIRIT_X3_CLASSIFY(digit)
        BOOST_SPIRIT_X3_CLASSIFY(xdigit)
        BOOST_SPIRIT_X3_CLASSIFY(cntrl)
        BOOST_SPIRIT_X3_CLASSIFY(graph)
        BOOST_SPIRIT_X3_CLASSIFY(lower)
        BOOST_SPIRIT_X3_CLASSIFY(print)
        BOOST_SPIRIT_X3_CLASSIFY(punct)
        BOOST_SPIRIT_X3_CLASSIFY(space)
        BOOST_SPIRIT_X3_CLASSIFY(blank)
        BOOST_SPIRIT_X3_CLASSIFY(upper)

#undef BOOST_SPIRIT_X3_CLASSIFY
    };

    template <typename Encoding, typename Tag>
    struct char_class
      : char_parser<char_class<Encoding, Tag>>
    {
        typedef Encoding encoding;
        typedef Tag tag;
        typedef typename Encoding::char_type char_type;
        typedef char_type attribute_type;
        static bool const has_attribute = true;

        template <typename Char, typename Context>
        bool test(Char ch, Context const& context) const
        {
            return encoding::ischar(ch)
                && char_class_base<Encoding>::is(
                    get_case_compare<Encoding>(context).get_char_class_tag(tag()), ch);
        }
    };

#define BOOST_SPIRIT_X3_CHAR_CLASS(encoding, name)                                 \
    typedef char_class<char_encoding::encoding, name##_tag> name##_type;           \
    constexpr name##_type name = name##_type();                                    \
    /***/

#define BOOST_SPIRIT_X3_CHAR_CLASSES(encoding)                                     \
    namespace encoding                                                             \
    {                                                                              \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, alnum)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, alpha)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, digit)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, xdigit)                               \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, cntrl)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, graph)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, lower)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, print)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, punct)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, space)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, blank)                                \
        BOOST_SPIRIT_X3_CHAR_CLASS(encoding, upper)                                \
    }                                                                              \
    /***/

    BOOST_SPIRIT_X3_CHAR_CLASSES(standard)
#ifndef BOOST_SPIRIT_NO_STANDARD_WIDE
    BOOST_SPIRIT_X3_CHAR_CLASSES(standard_wide)
#endif
    BOOST_SPIRIT_X3_CHAR_CLASSES(ascii)
    BOOST_SPIRIT_X3_CHAR_CLASSES(iso8859_1)

#undef BOOST_SPIRIT_X3_CHAR_CLASS
#undef BOOST_SPIRIT_X3_CHAR_CLASSES

    using standard::alnum_type;
    using standard::alpha_type;
    using standard::digit_type;
    using standard::xdigit_type;
    using standard::cntrl_type;
    using standard::graph_type;
    using standard::lower_type;
    using standard::print_type;
    using standard::punct_type;
    using standard::space_type;
    using standard::blank_type;
    using standard::upper_type;

    using standard::alnum;
    using standard::alpha;
    using standard::digit;
    using standard::xdigit;
    using standard::cntrl;
    using standard::graph;
    using standard::lower;
    using standard::print;
    using standard::punct;
    using standard::space;
    using standard::blank;
    using standard::upper;
}}}

#endif
