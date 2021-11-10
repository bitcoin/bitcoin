/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_UC_TYPES_NOVEMBER_23_2008_0840PM)
#define BOOST_SPIRIT_X3_UC_TYPES_NOVEMBER_23_2008_0840PM

#include <boost/cstdint.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <string>

namespace boost { namespace spirit { namespace x3
{
    typedef ::boost::uint32_t ucs4_char;
    typedef char utf8_char;
    typedef std::basic_string<ucs4_char> ucs4_string;
    typedef std::basic_string<utf8_char> utf8_string;

    template <typename Char>
    inline utf8_string to_utf8(Char value)
    {
        // always store as UTF8
        utf8_string result;
        typedef std::back_insert_iterator<utf8_string> insert_iter;
        insert_iter out_iter(result);
        utf8_output_iterator<insert_iter> utf8_iter(out_iter);
        typedef typename make_unsigned<Char>::type UChar;
        *utf8_iter = (UChar)value;
        return result;
    }

    template <typename Char>
    inline utf8_string to_utf8(Char const* str)
    {
        // always store as UTF8
        utf8_string result;
        typedef std::back_insert_iterator<utf8_string> insert_iter;
        insert_iter out_iter(result);
        utf8_output_iterator<insert_iter> utf8_iter(out_iter);
        typedef typename make_unsigned<Char>::type UChar;
        while (*str)
            *utf8_iter++ = (UChar)*str++;
        return result;
    }

    template <typename Char, typename Traits, typename Allocator>
    inline utf8_string
    to_utf8(std::basic_string<Char, Traits, Allocator> const& str)
    {
        // always store as UTF8
        utf8_string result;
        typedef std::back_insert_iterator<utf8_string> insert_iter;
        insert_iter out_iter(result);
        utf8_output_iterator<insert_iter> utf8_iter(out_iter);
        typedef typename make_unsigned<Char>::type UChar;
        for (Char ch : str)
        {
            *utf8_iter++ = (UChar)ch;
        }
        return result;
    }

    // Assume wchar_t content is UTF-16 on MSVC, or mingw/wineg++ with -fshort-wchar
#if defined(_MSC_VER) || defined(__SIZEOF_WCHAR_T__) && __SIZEOF_WCHAR_T__ == 2
    inline utf8_string to_utf8(wchar_t value)
    {
        utf8_string result;
        typedef std::back_insert_iterator<utf8_string> insert_iter;
        insert_iter out_iter(result);
        utf8_output_iterator<insert_iter> utf8_iter(out_iter);

        u16_to_u32_iterator<wchar_t const*, ucs4_char> ucs4_iter(&value);
        *utf8_iter++ = *ucs4_iter;

        return result;
    }

    inline utf8_string to_utf8(wchar_t const* str)
    {
        utf8_string result;
        typedef std::back_insert_iterator<utf8_string> insert_iter;
        insert_iter out_iter(result);
        utf8_output_iterator<insert_iter> utf8_iter(out_iter);

        u16_to_u32_iterator<wchar_t const*, ucs4_char> ucs4_iter(str);
        for (ucs4_char c; (c = *ucs4_iter) != ucs4_char(); ++ucs4_iter) {
            *utf8_iter++ = c;
        }

        return result;
    }

    template <typename Traits, typename Allocator>
    inline utf8_string
    to_utf8(std::basic_string<wchar_t, Traits, Allocator> const& str)
    {
        return to_utf8(str.c_str());
    }
#endif
}}}

#endif
