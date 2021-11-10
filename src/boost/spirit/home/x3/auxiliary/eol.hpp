/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_EOL_MARCH_23_2007_0454PM)
#define BOOST_SPIRIT_X3_EOL_MARCH_23_2007_0454PM

#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct eol_parser : parser<eol_parser>
    {
        typedef unused_type attribute_type;
        static bool const has_attribute = false;

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
         , Context const& context, unused_type, Attribute& /*attr*/) const
        {
            x3::skip_over(first, last, context);
            Iterator iter = first;
            bool matched = false;
            if (iter != last && *iter == '\r')  // CR
            {
                matched = true;
                ++iter;
            }
            if (iter != last && *iter == '\n')  // LF
            {
                matched = true;
                ++iter;
            }

            if (matched) first = iter;
            return matched;
        }
    };

    template<>
    struct get_info<eol_parser>
    {
        typedef std::string result_type;
        result_type operator()(eol_parser const &) const { return "eol"; }
    };

    constexpr auto eol = eol_parser{};
}}}

#endif
