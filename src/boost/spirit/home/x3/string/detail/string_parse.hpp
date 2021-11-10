/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_STRING_PARSE_APR_18_2006_1125PM)
#define BOOST_SPIRIT_X3_STRING_PARSE_APR_18_2006_1125PM

#include <boost/spirit/home/x3/support/traits/move_to.hpp>

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    template <typename Char, typename Iterator, typename Attribute, typename CaseCompareFunc>
    inline bool string_parse(
        Char const* str
      , Iterator& first, Iterator const& last, Attribute& attr, CaseCompareFunc const& compare) 
    {
        Iterator i = first;
        Char ch = *str;

        for (; !!ch; ++i)
        {
            if (i == last || (compare(ch, *i) != 0))
                return false;
            ch = *++str;
        }

        x3::traits::move_to(first, i, attr);
        first = i;
        return true;
    }

    template <typename String, typename Iterator, typename Attribute, typename CaseCompareFunc>
    inline bool string_parse(
        String const& str
      , Iterator& first, Iterator const& last, Attribute& attr, CaseCompareFunc const& compare)
    {
        Iterator i = first;
        typename String::const_iterator stri = str.begin();
        typename String::const_iterator str_last = str.end();

        for (; stri != str_last; ++stri, ++i)
            if (i == last || (compare(*stri, *i) != 0))
                return false;
        x3::traits::move_to(first, i, attr);
        first = i;
        return true;
    }

    template <typename Char, typename Iterator, typename Attribute>
    inline bool string_parse(
        Char const* uc_i, Char const* lc_i
      , Iterator& first, Iterator const& last, Attribute& attr)
    {
        Iterator i = first;

        for (; *uc_i && *lc_i; ++uc_i, ++lc_i, ++i)
            if (i == last || ((*uc_i != *i) && (*lc_i != *i)))
                return false;
        x3::traits::move_to(first, i, attr);
        first = i;
        return true;
    }

    template <typename String, typename Iterator, typename Attribute>
    inline bool string_parse(
        String const& ucstr, String const& lcstr
      , Iterator& first, Iterator const& last, Attribute& attr)
    {
        typename String::const_iterator uc_i = ucstr.begin();
        typename String::const_iterator uc_last = ucstr.end();
        typename String::const_iterator lc_i = lcstr.begin();
        Iterator i = first;

        for (; uc_i != uc_last; ++uc_i, ++lc_i, ++i)
            if (i == last || ((*uc_i != *i) && (*lc_i != *i)))
                return false;
        x3::traits::move_to(first, i, attr);
        first = i;
        return true;
    }
}}}}

#endif
