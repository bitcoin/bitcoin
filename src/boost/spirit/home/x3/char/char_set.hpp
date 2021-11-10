/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_CHAR_SET_OCT_12_2014_1051AM)
#define BOOST_SPIRIT_X3_CHAR_SET_OCT_12_2014_1051AM

#include <boost/spirit/home/x3/char/char_parser.hpp>
#include <boost/spirit/home/x3/char/detail/cast_char.hpp>
#include <boost/spirit/home/x3/support/traits/string_traits.hpp>
#include <boost/spirit/home/x3/support/utility/utf8.hpp>
#include <boost/spirit/home/x3/support/no_case.hpp>
#include <boost/spirit/home/support/char_set/basic_chset.hpp>

#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    // Parser for a character range
    ///////////////////////////////////////////////////////////////////////////
    template <typename Encoding, typename Attribute = typename Encoding::char_type>
    struct char_range
      : char_parser< char_range<Encoding, Attribute> >
    {

        typedef typename Encoding::char_type char_type;
        typedef Encoding encoding;
        typedef Attribute attribute_type;
        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;


        constexpr char_range(char_type from_, char_type to_)
          : from(from_), to(to_) {}

        template <typename Char, typename Context>
        bool test(Char ch_, Context const& context) const
        {

            char_type ch = char_type(ch_);  // optimize for token based parsing
            return (get_case_compare<encoding>(context)(ch, from) >= 0)
               && (get_case_compare<encoding>(context)(ch , to) <= 0);
        }

        char_type from, to;
    };


    ///////////////////////////////////////////////////////////////////////////
    // Parser for a character set
    ///////////////////////////////////////////////////////////////////////////
    template <typename Encoding, typename Attribute = typename Encoding::char_type>
    struct char_set : char_parser<char_set<Encoding, Attribute>>
    {
        typedef typename Encoding::char_type char_type;
        typedef Encoding encoding;
        typedef Attribute attribute_type;
        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;

        template <typename String>
        char_set(String const& str)
        {
            using spirit::x3::detail::cast_char;

            auto* definition = traits::get_c_string(str);
            auto ch = *definition++;
            while (ch)
            {
                auto next = *definition++;
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

        template <typename Char, typename Context>
        bool test(Char ch_, Context const& context) const
        {
            return get_case_compare<encoding>(context).in_set(ch_, chset);
        }

        support::detail::basic_chset<char_type> chset;
    };

    template <typename Encoding, typename Attribute>
    struct get_info<char_set<Encoding, Attribute>>
    {
        typedef std::string result_type;
        std::string operator()(char_set<Encoding, Attribute> const& /* p */) const
        {
            return "char-set";
        }
    };

    template <typename Encoding, typename Attribute>
    struct get_info<char_range<Encoding, Attribute>>
    {
        typedef std::string result_type;
        std::string operator()(char_range<Encoding, Attribute> const& p) const
        {
            return "char_range \"" + to_utf8(Encoding::toucs4(p.from)) + '-' + to_utf8(Encoding::toucs4(p.to))+ '"';
        }
    };

}}}

#endif
