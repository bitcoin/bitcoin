/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_INT_APR_17_2006_0830AM)
#define BOOST_SPIRIT_X3_INT_APR_17_2006_0830AM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/support/numeric_utils/extract_int.hpp>
#include <cstdint>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct int_parser : parser<int_parser<T, Radix, MinDigits, MaxDigits>>
    {
        // check template parameter 'Radix' for validity
        static_assert(
            (Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16),
            "Error Unsupported Radix");

        typedef T attribute_type;
        static bool const has_attribute = true;

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr) const
        {
            typedef extract_int<T, Radix, MinDigits, MaxDigits> extract;
            x3::skip_over(first, last, context);
            return extract::call(first, last, attr);
        }
    };

#define BOOST_SPIRIT_X3_INT_PARSER(int_type, name)                              \
    typedef int_parser<int_type> name##type;                                    \
    constexpr name##type name = {};                                             \
    /***/

    BOOST_SPIRIT_X3_INT_PARSER(long, long_)
    BOOST_SPIRIT_X3_INT_PARSER(short, short_)
    BOOST_SPIRIT_X3_INT_PARSER(int, int_)
    BOOST_SPIRIT_X3_INT_PARSER(long long, long_long)

    BOOST_SPIRIT_X3_INT_PARSER(int8_t, int8)
    BOOST_SPIRIT_X3_INT_PARSER(int16_t, int16)
    BOOST_SPIRIT_X3_INT_PARSER(int32_t, int32)
    BOOST_SPIRIT_X3_INT_PARSER(int64_t, int64)

#undef BOOST_SPIRIT_X3_INT_PARSER

}}}

#endif
