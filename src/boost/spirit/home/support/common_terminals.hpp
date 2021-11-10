/*=============================================================================
  Copyright (c) 2001-2011 Joel de Guzman
  http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_COMMON_PLACEHOLDERS_OCTOBER_16_2008_0102PM
#define BOOST_SPIRIT_COMMON_PLACEHOLDERS_OCTOBER_16_2008_0102PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/terminal.hpp>
#include <boost/spirit/home/support/char_encoding/standard.hpp>
#include <boost/spirit/home/support/char_encoding/standard_wide.hpp>
#include <boost/spirit/home/support/char_encoding/ascii.hpp>
#include <boost/spirit/home/support/char_encoding/iso8859_1.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/proto/traits.hpp>

#if defined(BOOST_SPIRIT_UNICODE)
# include <boost/spirit/home/support/char_encoding/unicode.hpp>
#endif

namespace boost { namespace spirit
{
    typedef mpl::vector<
            spirit::char_encoding::ascii
          , spirit::char_encoding::iso8859_1
          , spirit::char_encoding::standard
          , spirit::char_encoding::standard_wide
#if defined(BOOST_SPIRIT_UNICODE)
          , spirit::char_encoding::unicode
#endif
        >
    char_encodings;

    template <typename T>
    struct is_char_encoding : mpl::false_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::ascii> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::iso8859_1> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::standard> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::standard_wide> : mpl::true_ {};

#if defined(BOOST_SPIRIT_UNICODE)
    template <>
    struct is_char_encoding<spirit::char_encoding::unicode> : mpl::true_ {};
#endif

    template <typename Encoding>
    struct encoding
        : proto::terminal<tag::char_code<tag::encoding, Encoding> >::type
    {};

    // Our basic terminals
    BOOST_SPIRIT_DEFINE_TERMINALS_NAME(
        ( verbatim, verbatim_type )
        ( no_delimit, no_delimit_type )
        ( lexeme, lexeme_type )
        ( no_skip, no_skip_type )
        ( omit, omit_type )
        ( raw, raw_type )
        ( as_string, as_string_type )
        ( as_wstring, as_wstring_type )
        ( inf, inf_type )
        ( eol, eol_type )
        ( eoi, eoi_type )
        ( buffer, buffer_type )
        ( true_, true_type )
        ( false_, false_type )
        ( matches, matches_type )
        ( hold, hold_type )
        ( strict, strict_type )
        ( relaxed, relaxed_type )
        ( duplicate, duplicate_type )
        ( expect, expect_type )
    )

    // Our extended terminals
    BOOST_SPIRIT_DEFINE_TERMINALS_NAME_EX(
        ( lit, lit_type )
        ( bin, bin_type )
        ( oct, oct_type )
        ( hex, hex_type )
        ( bool_, bool_type )
        ( ushort_, ushort_type )
        ( ulong_, ulong_type )
        ( uint_, uint_type )
        ( short_, short_type )
        ( long_, long_type )
        ( int_, int_type )
        ( ulong_long, ulong_long_type )
        ( long_long, long_long_type )
        ( float_, float_type )
        ( double_, double_type )
        ( long_double, long_double_type )
        ( repeat, repeat_type )
        ( eps, eps_type )
        ( pad, pad_type )
        ( byte_, byte_type )
        ( word, word_type )
        ( big_word, big_word_type )
        ( little_word, little_word_type )
        ( dword, dword_type )
        ( big_dword, big_dword_type )
        ( little_dword, little_dword_type )
        ( qword, qword_type )
        ( big_qword, big_qword_type )
        ( little_qword, little_qword_type )
        ( bin_float, bin_float_type )
        ( big_bin_float, big_bin_float_type )
        ( little_bin_float, little_bin_float_type )
        ( bin_double, bin_double_type )
        ( big_bin_double, big_bin_double_type )
        ( little_bin_double, little_bin_double_type )
        ( skip, skip_type )
        ( delimit, delimit_type )
        ( stream, stream_type )
        ( wstream, wstream_type )
        ( left_align, left_align_type )
        ( right_align, right_align_type )
        ( center, center_type )
        ( maxwidth, maxwidth_type )
        ( set_state, set_state_type )
        ( in_state, in_state_type )
        ( token, token_type )
        ( tokenid, tokenid_type )
        ( raw_token, raw_token_type )
        ( tokenid_mask, tokenid_mask_type )
        ( attr, attr_type )
        ( columns, columns_type )
        ( auto_, auto_type )
    )

    // special tags (used mainly for stateful tag types)
    namespace tag
    {
        struct attr_cast { BOOST_SPIRIT_IS_TAG() };
        struct as { BOOST_SPIRIT_IS_TAG() };
    }
}}

///////////////////////////////////////////////////////////////////////////////
// Here we place the character-set sensitive placeholders. We have one set
// each for ascii, iso8859_1, standard and standard_wide and unicode. These
// placeholders are placed in its char-set namespace. For example, there exist
// a placeholder spirit::ascii::alnum for ascii versions of alnum.

#define BOOST_SPIRIT_TAG_CHAR_SPEC(charset)                                     \
    typedef tag::char_code<tag::char_, charset> char_;                          \
    typedef tag::char_code<tag::string, charset> string;                        \
    /***/

#ifdef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#define BOOST_SPIRIT_CHAR_SPEC(charset)                                         \
    typedef spirit::terminal<tag::charset::char_> char_type;                    \
    typedef spirit::terminal<tag::charset::string> string_type;                 \
    /***/

#else

#define BOOST_SPIRIT_CHAR_SPEC(charset)                                         \
    typedef spirit::terminal<tag::charset::char_> char_type;                    \
    char_type const char_ = char_type();                                        \
                                                                                \
    inline void silence_unused_warnings_##char_() { (void) char_; }             \
                                                                                \
    typedef spirit::terminal<tag::charset::string> string_type;                 \
    string_type const string = string_type();                                   \
                                                                                \
    inline void silence_unused_warnings_##string() { (void) string; }           \
    /***/

#endif

#ifdef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#define BOOST_SPIRIT_CHAR_CODE(name, charset)                                   \
    typedef proto::terminal<tag::char_code<tag::name, charset> >::type          \
        name##_type;                                                            \
    /***/

#else

#define BOOST_SPIRIT_CHAR_CODE(name, charset)                                   \
    typedef proto::terminal<tag::char_code<tag::name, charset> >::type          \
        name##_type;                                                            \
    name##_type const name = name##_type();                                     \
                                                                                \
    inline void silence_unused_warnings_##name() { (void) name; }               \
    /***/


#endif

#define BOOST_SPIRIT_DEFINE_CHAR_CODES(charset)                                 \
    namespace boost { namespace spirit { namespace tag { namespace charset      \
    {                                                                           \
        BOOST_SPIRIT_TAG_CHAR_SPEC(spirit::char_encoding::charset)              \
    }}}}                                                                        \
    namespace boost { namespace spirit { namespace charset                      \
    {                                                                           \
        BOOST_SPIRIT_CHAR_SPEC(charset)                                         \
                                                                                \
        BOOST_SPIRIT_CHAR_CODE(alnum, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(alpha, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(blank, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(cntrl, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(digit, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(graph, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(print, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(punct, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(space, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(xdigit, spirit::char_encoding::charset)          \
                                                                                \
        BOOST_SPIRIT_CHAR_CODE(no_case, spirit::char_encoding::charset)         \
        BOOST_SPIRIT_CHAR_CODE(lower, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(upper, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(lowernum, spirit::char_encoding::charset)        \
        BOOST_SPIRIT_CHAR_CODE(uppernum, spirit::char_encoding::charset)        \
    }}}                                                                         \
    /***/

BOOST_SPIRIT_DEFINE_CHAR_CODES(ascii)
BOOST_SPIRIT_DEFINE_CHAR_CODES(iso8859_1)
BOOST_SPIRIT_DEFINE_CHAR_CODES(standard)
BOOST_SPIRIT_DEFINE_CHAR_CODES(standard_wide)

namespace boost { namespace spirit { namespace traits
{
    template <typename Char>
    struct char_encoding_from_char;

    template <>
    struct char_encoding_from_char<char>
      : mpl::identity<spirit::char_encoding::standard>
    {};

    template <>
    struct char_encoding_from_char<wchar_t>
      : mpl::identity<spirit::char_encoding::standard_wide>
    {};

    template <typename T>
    struct char_encoding_from_char<T const>
      : char_encoding_from_char<T>
    {};
}}}

#if defined(BOOST_SPIRIT_UNICODE)
BOOST_SPIRIT_DEFINE_CHAR_CODES(unicode)

    namespace boost { namespace spirit { namespace tag { namespace unicode
    {
        BOOST_SPIRIT_TAG_CHAR_SPEC(spirit::char_encoding::unicode)
    }}}}

    namespace boost { namespace spirit { namespace unicode
    {
#define BOOST_SPIRIT_UNICODE_CHAR_CODE(name)                                    \
    BOOST_SPIRIT_CHAR_CODE(name, spirit::char_encoding::unicode)                \

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Major Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode General Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(uppercase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lowercase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(titlecase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(modifier_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_letter)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(nonspacing_mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(enclosing_mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(spacing_mark)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(decimal_number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(letter_number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_number)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(space_separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(line_separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(paragraph_separator)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(control)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(format)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(private_use)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(surrogate)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(unassigned)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(dash_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(open_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(close_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(connector_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(initial_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(final_punctuation)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(math_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(currency_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(modifier_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Derived Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(alphabetic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(uppercase)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lowercase)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(white_space)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hex_digit)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(noncharacter_code_point)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(default_ignorable_code_point)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Scripts
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(arabic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(imperial_aramaic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(armenian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(avestan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(balinese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bamum)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bengali)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bopomofo)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(braille)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(buginese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(buhid)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(canadian_aboriginal)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(carian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cherokee)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(coptic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cypriot)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cyrillic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(devanagari)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(deseret)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(egyptian_hieroglyphs)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ethiopic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(georgian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(glagolitic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gothic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(greek)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gujarati)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gurmukhi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hangul)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(han)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hanunoo)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hebrew)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hiragana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(katakana_or_hiragana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_italic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(javanese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kayah_li)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(katakana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kharoshthi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(khmer)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kannada)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kaithi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_tham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lao)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(latin)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lepcha)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(limbu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(linear_b)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lisu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lycian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lydian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(malayalam)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(mongolian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(meetei_mayek)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(myanmar)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(nko)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ogham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ol_chiki)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_turkic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(oriya)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(osmanya)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(phags_pa)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inscriptional_pahlavi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(phoenician)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inscriptional_parthian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(rejang)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(runic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(samaritan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_south_arabian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(saurashtra)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(shavian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(sinhala)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(sundanese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(syloti_nagri)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(syriac)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tagbanwa)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_le)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(new_tai_lue)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tamil)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_viet)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(telugu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tifinagh)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tagalog)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(thaana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(thai)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tibetan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ugaritic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(vai)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_persian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cuneiform)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(yi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inherited)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(common)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(unknown)

#undef BOOST_SPIRIT_UNICODE_CHAR_CODE
    }}}
#endif

#endif
