/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_LITERAL_CHAR_APRIL_16_2006_1051AM)
#define BOOST_SPIRIT_X3_LITERAL_CHAR_APRIL_16_2006_1051AM

#include <boost/spirit/home/x3/char/char_parser.hpp>
#include <boost/spirit/home/x3/support/utility/utf8.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Encoding, typename Attribute = typename Encoding::char_type>
    struct literal_char : char_parser<literal_char<Encoding, Attribute>>
    {
        typedef typename Encoding::char_type char_type;
        typedef Encoding encoding;
        typedef Attribute attribute_type;
        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;

        template <typename Char>
        constexpr literal_char(Char ch)
          : ch(static_cast<char_type>(ch)) {}

        template <typename Char, typename Context>
        bool test(Char ch_, Context const& context) const
        {
            return get_case_compare<encoding>(context)(ch, char_type(ch_)) == 0;
        }

        char_type ch;
    };

    template <typename Encoding, typename Attribute>
    struct get_info<literal_char<Encoding, Attribute>>
    {
        typedef std::string result_type;
        std::string operator()(literal_char<Encoding, Attribute> const& p) const
        {
            return '\'' + to_utf8(Encoding::toucs4(p.ch)) + '\'';
        }
    };
}}}

#endif
