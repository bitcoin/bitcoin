//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_STRING_COMPARE_AUG_08_2009_0756PM)
#define BOOST_SPIRIT_KARMA_STRING_COMPARE_AUG_08_2009_0756PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    template <typename Char>
    bool string_compare(Char const* attr, Char const* lit)
    {
        Char ch_attr = *attr;
        Char ch_lit = *lit;

        while (!!ch_lit && !!ch_attr)
        {
            if (ch_attr != ch_lit)
                return false;

            ch_attr = *++attr;
            ch_lit = *++lit;
        }

        return !ch_lit && !ch_attr;
    }

    template <typename Char>
    bool string_compare(Char const* attr, Char const* lit, unused_type, unused_type)
    {
        return string_compare(attr, lit);
    }

    template <typename Char>
    bool string_compare(unused_type, Char const*, unused_type, unused_type)
    {
        return true;
    }

    template <typename Char, typename CharEncoding, typename Tag>
    bool string_compare(Char const* attr, Char const* lit, CharEncoding, Tag)
    {
        Char ch_attr = *attr;
        Char ch_lit = spirit::char_class::convert<CharEncoding>::to(Tag(), *lit);

        while (!!ch_lit && !!ch_attr)
        {
            if (ch_attr != ch_lit)
                return false;

            ch_attr = *++attr;
            ch_lit = spirit::char_class::convert<CharEncoding>::to(Tag(), *++lit);
        }

        return !ch_lit && !ch_attr;
    }

    template <typename Char, typename CharEncoding, typename Tag>
    bool string_compare(unused_type, Char const*, CharEncoding, Tag)
    {
        return true;
    }

}}}}

#endif
