/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UNICODE_1_JANUARY_12_2010_0728PM)
#define BOOST_SPIRIT_UNICODE_1_JANUARY_12_2010_0728PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/cstdint.hpp>
#include <boost/spirit/home/support/char_encoding/unicode/query.hpp>

namespace boost { namespace spirit { namespace char_encoding
{
    ///////////////////////////////////////////////////////////////////////////
    //  Test characters for specified conditions (using iso8859-1)
    ///////////////////////////////////////////////////////////////////////////
    struct unicode
    {
#ifdef BOOST_NO_CXX11_CHAR32_T
        typedef ::boost::uint32_t char_type;
#else
        typedef char32_t char_type;
#endif
        typedef ::boost::uint32_t classify_type;

    ///////////////////////////////////////////////////////////////////////////
    //  Posix stuff
    ///////////////////////////////////////////////////////////////////////////
        static bool
        isascii_(char_type ch)
        {
            return 0 == (ch & ~0x7f);
        }

        static bool
        ischar(char_type ch)
        {
            // unicode code points in the range 0x00 to 0x10FFFF
            return ch <= 0x10FFFF;
        }

        static bool
        isalnum(char_type ch)
        {
            return ucd::is_alphanumeric(ch);
        }

        static bool
        isalpha(char_type ch)
        {
            return ucd::is_alphabetic(ch);
        }

        static bool
        isdigit(char_type ch)
        {
            return ucd::is_decimal_number(ch);
        }

        static bool
        isxdigit(char_type ch)
        {
            return ucd::is_hex_digit(ch);
        }

        static bool
        iscntrl(char_type ch)
        {
            return ucd::is_control(ch);
        }

        static bool
        isgraph(char_type ch)
        {
            return ucd::is_graph(ch);
        }

        static bool
        islower(char_type ch)
        {
            return ucd::is_lowercase(ch);
        }

        static bool
        isprint(char_type ch)
        {
            return ucd::is_print(ch);
        }

        static bool
        ispunct(char_type ch)
        {
            return ucd::is_punctuation(ch);
        }

        static bool
        isspace(char_type ch)
        {
            return ucd::is_white_space(ch);
        }

        static bool
        isblank BOOST_PREVENT_MACRO_SUBSTITUTION (char_type ch)
        {
            return ucd::is_blank(ch);
        }

        static bool
        isupper(char_type ch)
        {
            return ucd::is_uppercase(ch);
        }

    ///////////////////////////////////////////////////////////////////////////
    //  Simple character conversions
    ///////////////////////////////////////////////////////////////////////////

        static char_type
        tolower(char_type ch)
        {
            return ucd::to_lowercase(ch);
        }

        static char_type
        toupper(char_type ch)
        {
            return ucd::to_uppercase(ch);
        }

        static ::boost::uint32_t
        toucs4(char_type ch)
        {
            return ch;
        }

    ///////////////////////////////////////////////////////////////////////////
    //  Major Categories
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_MAJOR_CATEGORY(name)                                       \
        static bool                                                             \
        is_##name(char_type ch)                                                 \
        {                                                                       \
            return ucd::get_major_category(ch) == ucd::properties::name;        \
        }                                                                       \
        /***/

        BOOST_SPIRIT_MAJOR_CATEGORY(letter)
        BOOST_SPIRIT_MAJOR_CATEGORY(mark)
        BOOST_SPIRIT_MAJOR_CATEGORY(number)
        BOOST_SPIRIT_MAJOR_CATEGORY(separator)
        BOOST_SPIRIT_MAJOR_CATEGORY(other)
        BOOST_SPIRIT_MAJOR_CATEGORY(punctuation)
        BOOST_SPIRIT_MAJOR_CATEGORY(symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  General Categories
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_CATEGORY(name)                                             \
        static bool                                                             \
        is_##name(char_type ch)                                                 \
        {                                                                       \
            return ucd::get_category(ch) == ucd::properties::name;              \
        }                                                                       \
        /***/

        BOOST_SPIRIT_CATEGORY(uppercase_letter)
        BOOST_SPIRIT_CATEGORY(lowercase_letter)
        BOOST_SPIRIT_CATEGORY(titlecase_letter)
        BOOST_SPIRIT_CATEGORY(modifier_letter)
        BOOST_SPIRIT_CATEGORY(other_letter)

        BOOST_SPIRIT_CATEGORY(nonspacing_mark)
        BOOST_SPIRIT_CATEGORY(enclosing_mark)
        BOOST_SPIRIT_CATEGORY(spacing_mark)

        BOOST_SPIRIT_CATEGORY(decimal_number)
        BOOST_SPIRIT_CATEGORY(letter_number)
        BOOST_SPIRIT_CATEGORY(other_number)

        BOOST_SPIRIT_CATEGORY(space_separator)
        BOOST_SPIRIT_CATEGORY(line_separator)
        BOOST_SPIRIT_CATEGORY(paragraph_separator)

        BOOST_SPIRIT_CATEGORY(control)
        BOOST_SPIRIT_CATEGORY(format)
        BOOST_SPIRIT_CATEGORY(private_use)
        BOOST_SPIRIT_CATEGORY(surrogate)
        BOOST_SPIRIT_CATEGORY(unassigned)

        BOOST_SPIRIT_CATEGORY(dash_punctuation)
        BOOST_SPIRIT_CATEGORY(open_punctuation)
        BOOST_SPIRIT_CATEGORY(close_punctuation)
        BOOST_SPIRIT_CATEGORY(connector_punctuation)
        BOOST_SPIRIT_CATEGORY(other_punctuation)
        BOOST_SPIRIT_CATEGORY(initial_punctuation)
        BOOST_SPIRIT_CATEGORY(final_punctuation)

        BOOST_SPIRIT_CATEGORY(math_symbol)
        BOOST_SPIRIT_CATEGORY(currency_symbol)
        BOOST_SPIRIT_CATEGORY(modifier_symbol)
        BOOST_SPIRIT_CATEGORY(other_symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  Derived Categories
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_DERIVED_CATEGORY(name)                                     \
        static bool                                                             \
        is_##name(char_type ch)                                                 \
        {                                                                       \
            return ucd::is_##name(ch);                                          \
        }                                                                       \
        /***/

        BOOST_SPIRIT_DERIVED_CATEGORY(alphabetic)
        BOOST_SPIRIT_DERIVED_CATEGORY(uppercase)
        BOOST_SPIRIT_DERIVED_CATEGORY(lowercase)
        BOOST_SPIRIT_DERIVED_CATEGORY(white_space)
        BOOST_SPIRIT_DERIVED_CATEGORY(hex_digit)
        BOOST_SPIRIT_DERIVED_CATEGORY(noncharacter_code_point)
        BOOST_SPIRIT_DERIVED_CATEGORY(default_ignorable_code_point)

    ///////////////////////////////////////////////////////////////////////////
    //  Scripts
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_SCRIPT(name)                                               \
        static bool                                                             \
        is_##name(char_type ch)                                                 \
        {                                                                       \
            return ucd::get_script(ch) == ucd::properties::name;                \
        }                                                                       \
        /***/

        BOOST_SPIRIT_SCRIPT(arabic)
        BOOST_SPIRIT_SCRIPT(imperial_aramaic)
        BOOST_SPIRIT_SCRIPT(armenian)
        BOOST_SPIRIT_SCRIPT(avestan)
        BOOST_SPIRIT_SCRIPT(balinese)
        BOOST_SPIRIT_SCRIPT(bamum)
        BOOST_SPIRIT_SCRIPT(bengali)
        BOOST_SPIRIT_SCRIPT(bopomofo)
        BOOST_SPIRIT_SCRIPT(braille)
        BOOST_SPIRIT_SCRIPT(buginese)
        BOOST_SPIRIT_SCRIPT(buhid)
        BOOST_SPIRIT_SCRIPT(canadian_aboriginal)
        BOOST_SPIRIT_SCRIPT(carian)
        BOOST_SPIRIT_SCRIPT(cham)
        BOOST_SPIRIT_SCRIPT(cherokee)
        BOOST_SPIRIT_SCRIPT(coptic)
        BOOST_SPIRIT_SCRIPT(cypriot)
        BOOST_SPIRIT_SCRIPT(cyrillic)
        BOOST_SPIRIT_SCRIPT(devanagari)
        BOOST_SPIRIT_SCRIPT(deseret)
        BOOST_SPIRIT_SCRIPT(egyptian_hieroglyphs)
        BOOST_SPIRIT_SCRIPT(ethiopic)
        BOOST_SPIRIT_SCRIPT(georgian)
        BOOST_SPIRIT_SCRIPT(glagolitic)
        BOOST_SPIRIT_SCRIPT(gothic)
        BOOST_SPIRIT_SCRIPT(greek)
        BOOST_SPIRIT_SCRIPT(gujarati)
        BOOST_SPIRIT_SCRIPT(gurmukhi)
        BOOST_SPIRIT_SCRIPT(hangul)
        BOOST_SPIRIT_SCRIPT(han)
        BOOST_SPIRIT_SCRIPT(hanunoo)
        BOOST_SPIRIT_SCRIPT(hebrew)
        BOOST_SPIRIT_SCRIPT(hiragana)
        BOOST_SPIRIT_SCRIPT(katakana_or_hiragana)
        BOOST_SPIRIT_SCRIPT(old_italic)
        BOOST_SPIRIT_SCRIPT(javanese)
        BOOST_SPIRIT_SCRIPT(kayah_li)
        BOOST_SPIRIT_SCRIPT(katakana)
        BOOST_SPIRIT_SCRIPT(kharoshthi)
        BOOST_SPIRIT_SCRIPT(khmer)
        BOOST_SPIRIT_SCRIPT(kannada)
        BOOST_SPIRIT_SCRIPT(kaithi)
        BOOST_SPIRIT_SCRIPT(tai_tham)
        BOOST_SPIRIT_SCRIPT(lao)
        BOOST_SPIRIT_SCRIPT(latin)
        BOOST_SPIRIT_SCRIPT(lepcha)
        BOOST_SPIRIT_SCRIPT(limbu)
        BOOST_SPIRIT_SCRIPT(linear_b)
        BOOST_SPIRIT_SCRIPT(lisu)
        BOOST_SPIRIT_SCRIPT(lycian)
        BOOST_SPIRIT_SCRIPT(lydian)
        BOOST_SPIRIT_SCRIPT(malayalam)
        BOOST_SPIRIT_SCRIPT(mongolian)
        BOOST_SPIRIT_SCRIPT(meetei_mayek)
        BOOST_SPIRIT_SCRIPT(myanmar)
        BOOST_SPIRIT_SCRIPT(nko)
        BOOST_SPIRIT_SCRIPT(ogham)
        BOOST_SPIRIT_SCRIPT(ol_chiki)
        BOOST_SPIRIT_SCRIPT(old_turkic)
        BOOST_SPIRIT_SCRIPT(oriya)
        BOOST_SPIRIT_SCRIPT(osmanya)
        BOOST_SPIRIT_SCRIPT(phags_pa)
        BOOST_SPIRIT_SCRIPT(inscriptional_pahlavi)
        BOOST_SPIRIT_SCRIPT(phoenician)
        BOOST_SPIRIT_SCRIPT(inscriptional_parthian)
        BOOST_SPIRIT_SCRIPT(rejang)
        BOOST_SPIRIT_SCRIPT(runic)
        BOOST_SPIRIT_SCRIPT(samaritan)
        BOOST_SPIRIT_SCRIPT(old_south_arabian)
        BOOST_SPIRIT_SCRIPT(saurashtra)
        BOOST_SPIRIT_SCRIPT(shavian)
        BOOST_SPIRIT_SCRIPT(sinhala)
        BOOST_SPIRIT_SCRIPT(sundanese)
        BOOST_SPIRIT_SCRIPT(syloti_nagri)
        BOOST_SPIRIT_SCRIPT(syriac)
        BOOST_SPIRIT_SCRIPT(tagbanwa)
        BOOST_SPIRIT_SCRIPT(tai_le)
        BOOST_SPIRIT_SCRIPT(new_tai_lue)
        BOOST_SPIRIT_SCRIPT(tamil)
        BOOST_SPIRIT_SCRIPT(tai_viet)
        BOOST_SPIRIT_SCRIPT(telugu)
        BOOST_SPIRIT_SCRIPT(tifinagh)
        BOOST_SPIRIT_SCRIPT(tagalog)
        BOOST_SPIRIT_SCRIPT(thaana)
        BOOST_SPIRIT_SCRIPT(thai)
        BOOST_SPIRIT_SCRIPT(tibetan)
        BOOST_SPIRIT_SCRIPT(ugaritic)
        BOOST_SPIRIT_SCRIPT(vai)
        BOOST_SPIRIT_SCRIPT(old_persian)
        BOOST_SPIRIT_SCRIPT(cuneiform)
        BOOST_SPIRIT_SCRIPT(yi)
        BOOST_SPIRIT_SCRIPT(inherited)
        BOOST_SPIRIT_SCRIPT(common)
        BOOST_SPIRIT_SCRIPT(unknown)

#undef BOOST_SPIRIT_MAJOR_CATEGORY
#undef BOOST_SPIRIT_CATEGORY
#undef BOOST_SPIRIT_DERIVED_CATEGORY
#undef BOOST_SPIRIT_SCRIPT

    };

}}}

#endif

