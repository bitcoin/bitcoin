/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_STANDARD_WIDE_NOVEMBER_10_2006_0913AM)
#define BOOST_SPIRIT_STANDARD_WIDE_NOVEMBER_10_2006_0913AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <cwctype>
#include <string>

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>

#include <boost/type_traits/make_unsigned.hpp>

namespace boost { namespace spirit { namespace traits
{
    template <std::size_t N>
    struct wchar_t_size
    {
        BOOST_SPIRIT_ASSERT_MSG(N == 1 || N == 2 || N == 4,
            not_supported_size_of_wchar_t, ());
    };

    template <> struct wchar_t_size<1> { enum { mask = 0xff }; };
    template <> struct wchar_t_size<2> { enum { mask = 0xffff }; };
    template <> struct wchar_t_size<4> { enum { mask = 0xffffffff }; };

}}}

namespace boost { namespace spirit { namespace char_encoding
{
    ///////////////////////////////////////////////////////////////////////////
    //  Test characters for specified conditions (using std wchar_t functions)
    ///////////////////////////////////////////////////////////////////////////

    struct standard_wide
    {
        typedef wchar_t char_type;
        typedef wchar_t classify_type;

        template <typename Char>
        static typename std::char_traits<Char>::int_type
        to_int_type(Char ch)
        {
            return std::char_traits<Char>::to_int_type(ch);
        }

        template <typename Char>
        static Char
        to_char_type(typename std::char_traits<Char>::int_type ch)
        {
            return std::char_traits<Char>::to_char_type(ch);
        }

        static bool
        ischar(int ch)
        {
            // we have to watch out for sign extensions (casting is there to
            // silence certain compilers complaining about signed/unsigned
            // mismatch)
            return (
                std::size_t(0) ==
                    std::size_t(ch & ~traits::wchar_t_size<sizeof(wchar_t)>::mask) ||
                std::size_t(~0) ==
                    std::size_t(ch | traits::wchar_t_size<sizeof(wchar_t)>::mask)
            ) != 0;     // any wchar_t, but no other bits set
        }

        static bool
        isalnum(wchar_t ch)
        {
            using namespace std;
            return iswalnum(to_int_type(ch)) != 0;
        }

        static bool
        isalpha(wchar_t ch)
        {
            using namespace std;
            return iswalpha(to_int_type(ch)) != 0;
        }

        static bool
        iscntrl(wchar_t ch)
        {
            using namespace std;
            return iswcntrl(to_int_type(ch)) != 0;
        }

        static bool
        isdigit(wchar_t ch)
        {
            using namespace std;
            return iswdigit(to_int_type(ch)) != 0;
        }

        static bool
        isgraph(wchar_t ch)
        {
            using namespace std;
            return iswgraph(to_int_type(ch)) != 0;
        }

        static bool
        islower(wchar_t ch)
        {
            using namespace std;
            return iswlower(to_int_type(ch)) != 0;
        }

        static bool
        isprint(wchar_t ch)
        {
            using namespace std;
            return iswprint(to_int_type(ch)) != 0;
        }

        static bool
        ispunct(wchar_t ch)
        {
            using namespace std;
            return iswpunct(to_int_type(ch)) != 0;
        }

        static bool
        isspace(wchar_t ch)
        {
            using namespace std;
            return iswspace(to_int_type(ch)) != 0;
        }

        static bool
        isupper(wchar_t ch)
        {
            using namespace std;
            return iswupper(to_int_type(ch)) != 0;
        }

        static bool
        isxdigit(wchar_t ch)
        {
            using namespace std;
            return iswxdigit(to_int_type(ch)) != 0;
        }

        static bool
        isblank BOOST_PREVENT_MACRO_SUBSTITUTION (wchar_t ch)
        {
            return (ch == L' ' || ch == L'\t');
        }

        ///////////////////////////////////////////////////////////////////////
        //  Simple character conversions
        ///////////////////////////////////////////////////////////////////////

        static wchar_t
        tolower(wchar_t ch)
        {
            using namespace std;
            return isupper(ch) ?
                to_char_type<wchar_t>(towlower(to_int_type(ch))) : ch;
        }

        static wchar_t
        toupper(wchar_t ch)
        {
            using namespace std;
            return islower(ch) ?
                to_char_type<wchar_t>(towupper(to_int_type(ch))) : ch;
        }

        static ::boost::uint32_t
        toucs4(wchar_t ch)
        {
            return static_cast<make_unsigned<wchar_t>::type>(ch);
        }
    };
}}}

#endif
