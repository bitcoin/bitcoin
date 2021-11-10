/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Grammar for universal character validation (see C++ standard: Annex E)

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_VALIDATE_UNIVERSAL_CHAR_HPP_55F1B811_CD76_4C72_8344_CBC69CF3B339_INCLUDED)
#define BOOST_VALIDATE_UNIVERSAL_CHAR_HPP_55F1B811_CD76_4C72_8344_CBC69CF3B339_INCLUDED

#include <boost/assert.hpp>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/cpplexer/cpplexer_exceptions.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace impl {

enum universal_char_type {
    universal_char_type_valid = 0,
    universal_char_type_invalid = 1,
    universal_char_type_base_charset = 2,
    universal_char_type_not_allowed_for_identifiers = 3
};

///////////////////////////////////////////////////////////////////////////
//
//  is_range is a helper function for the classification by brute force
//  below
//
///////////////////////////////////////////////////////////////////////////
inline bool
in_range(unsigned long ch, unsigned long l, unsigned long u)
{
    return (l <= ch && ch <= u);
}

///////////////////////////////////////////////////////////////////////////////
//
//  classify_universal_char
//
//      This function classifies an universal character value into 4 subranges:
//      universal_char_type_valid
//          the universal character value is valid for identifiers
//      universal_char_type_invalid
//          the universal character value is not valid for its usage inside
//          identifiers (see C++ Standard: 2.2.2 [lex.charset])
//      universal_char_type_base_charset
//          the universal character value designates a character from the base
//          character set
//      universal_char_type_not_allowed_for_identifiers
//          the universal character value is not allowed in an identifier
//
//  Implementation note:
//      This classification isn't implemented very effectively here. This
//      function should be rewritten with some range run matching algorithm.
//
///////////////////////////////////////////////////////////////////////////////
inline universal_char_type
classify_universal_char (unsigned long ch)
{
    // test for invalid characters
    if (ch <= 0x0020 || in_range(ch, 0x007f, 0x009f))
        return universal_char_type_invalid;

    // test for characters in the range of the base character set
    if (in_range(ch, 0x0021, 0x005f) || in_range(ch, 0x0061, 0x007e))
        return universal_char_type_base_charset;

    // test for additional valid character values (see C++ Standard: Annex E)
    if (in_range(ch, 0x00c0, 0x00d6) || in_range(ch, 0x00d8, 0x00f6) ||
        in_range(ch, 0x00f8, 0x01f5) || in_range(ch, 0x01fa, 0x0217) ||
        in_range(ch, 0x0250, 0x02a8) || in_range(ch, 0x1e00, 0x1e9a) ||
        in_range(ch, 0x1ea0, 0x1ef9))
    {
        return universal_char_type_valid;   // Latin
    }

    if (0x0384 == ch || in_range(ch, 0x0388, 0x038a) ||
        0x038c == ch || in_range(ch, 0x038e, 0x03a1) ||
        in_range(ch, 0x03a3, 0x03ce) || in_range(ch, 0x03d0, 0x03d6) ||
        0x03da == ch || 0x03dc == ch || 0x03de == ch || 0x03e0 == ch ||
        in_range(ch, 0x03e2, 0x03f3) || in_range(ch, 0x1f00, 0x1f15) ||
        in_range(ch, 0x1f18, 0x1f1d) || in_range(ch, 0x1f20, 0x1f45) ||
        in_range(ch, 0x1f48, 0x1f4d) || in_range(ch, 0x1f50, 0x1f57) ||
        0x1f59 == ch || 0x1f5b == ch || 0x1f5d == ch ||
        in_range(ch, 0x1f5f, 0x1f7d) || in_range(ch, 0x1f80, 0x1fb4) ||
        in_range(ch, 0x1fb6, 0x1fbc) || in_range(ch, 0x1fc2, 0x1fc4) ||
        in_range(ch, 0x1fc6, 0x1fcc) || in_range(ch, 0x1fd0, 0x1fd3) ||
        in_range(ch, 0x1fd6, 0x1fdb) || in_range(ch, 0x1fe0, 0x1fec) ||
        in_range(ch, 0x1ff2, 0x1ff4) || in_range(ch, 0x1ff6, 0x1ffc))
    {
        return universal_char_type_valid;   // Greek
    }

    if (in_range(ch, 0x0401, 0x040d) || in_range(ch, 0x040f, 0x044f) ||
        in_range(ch, 0x0451, 0x045c) || in_range(ch, 0x045e, 0x0481) ||
        in_range(ch, 0x0490, 0x04c4) || in_range(ch, 0x04c7, 0x04c8) ||
        in_range(ch, 0x04cb, 0x04cc) || in_range(ch, 0x04d0, 0x04eb) ||
        in_range(ch, 0x04ee, 0x04f5) || in_range(ch, 0x04f8, 0x04f9))
    {
        return universal_char_type_valid;   // Cyrillic
    }

    if (in_range(ch, 0x0531, 0x0556) || in_range(ch, 0x0561, 0x0587))
        return universal_char_type_valid;   // Armenian

    if (in_range(ch, 0x05d0, 0x05ea) || in_range(ch, 0x05f0, 0x05f4))
        return universal_char_type_valid;   // Hebrew

    if (in_range(ch, 0x0621, 0x063a) || in_range(ch, 0x0640, 0x0652) ||
        in_range(ch, 0x0670, 0x06b7) || in_range(ch, 0x06ba, 0x06be) ||
        in_range(ch, 0x06c0, 0x06ce) || in_range(ch, 0x06e5, 0x06e7))
    {
        return universal_char_type_valid;   // Arabic
    }

    if (in_range(ch, 0x0905, 0x0939) || in_range(ch, 0x0958, 0x0962))
        return universal_char_type_valid;   // Devanagari

    if (in_range(ch, 0x0985, 0x098c) || in_range(ch, 0x098f, 0x0990) ||
        in_range(ch, 0x0993, 0x09a8) || in_range(ch, 0x09aa, 0x09b0) ||
        0x09b2 == ch || in_range(ch, 0x09b6, 0x09b9) ||
        in_range(ch, 0x09dc, 0x09dd) || in_range(ch, 0x09df, 0x09e1) ||
        in_range(ch, 0x09f0, 0x09f1))
    {
        return universal_char_type_valid;   // Bengali
    }

    if (in_range(ch, 0x0a05, 0x0a0a) || in_range(ch, 0x0a0f, 0x0a10) ||
        in_range(ch, 0x0a13, 0x0a28) || in_range(ch, 0x0a2a, 0x0a30) ||
        in_range(ch, 0x0a32, 0x0a33) || in_range(ch, 0x0a35, 0x0a36) ||
        in_range(ch, 0x0a38, 0x0a39) || in_range(ch, 0x0a59, 0x0a5c) ||
        0x0a5e == ch)
    {
        return universal_char_type_valid;   // Gurmukhi
    }

    if (in_range(ch, 0x0a85, 0x0a8b) || 0x0a8d == ch ||
        in_range(ch, 0x0a8f, 0x0a91) || in_range(ch, 0x0a93, 0x0aa8) ||
        in_range(ch, 0x0aaa, 0x0ab0) || in_range(ch, 0x0ab2, 0x0ab3) ||
        in_range(ch, 0x0ab5, 0x0ab9) || 0x0ae0 == ch)
    {
        return universal_char_type_valid;   // Gujarati
    }

    if (in_range(ch, 0x0b05, 0x0b0c) || in_range(ch, 0x0b0f, 0x0b10) ||
        in_range(ch, 0x0b13, 0x0b28) || in_range(ch, 0x0b2a, 0x0b30) ||
        in_range(ch, 0x0b32, 0x0b33) || in_range(ch, 0x0b36, 0x0b39) ||
        in_range(ch, 0x0b5c, 0x0b5d) || in_range(ch, 0x0b5f, 0x0b61))
    {
        return universal_char_type_valid;   // Oriya
    }

    if (in_range(ch, 0x0b85, 0x0b8a) || in_range(ch, 0x0b8e, 0x0b90) ||
        in_range(ch, 0x0b92, 0x0b95) || in_range(ch, 0x0b99, 0x0b9a) ||
        0x0b9c == ch || in_range(ch, 0x0b9e, 0x0b9f) ||
        in_range(ch, 0x0ba3, 0x0ba4) || in_range(ch, 0x0ba8, 0x0baa) ||
        in_range(ch, 0x0bae, 0x0bb5) || in_range(ch, 0x0bb7, 0x0bb9))
    {
        return universal_char_type_valid;   // Tamil
    }

    if (in_range(ch, 0x0c05, 0x0c0c) || in_range(ch, 0x0c0e, 0x0c10) ||
        in_range(ch, 0x0c12, 0x0c28) || in_range(ch, 0x0c2a, 0x0c33) ||
        in_range(ch, 0x0c35, 0x0c39) || in_range(ch, 0x0c60, 0x0c61))
    {
        return universal_char_type_valid;   // Telugu
    }

    if (in_range(ch, 0x0c85, 0x0c8c) || in_range(ch, 0x0c8e, 0x0c90) ||
        in_range(ch, 0x0c92, 0x0ca8) || in_range(ch, 0x0caa, 0x0cb3) ||
        in_range(ch, 0x0cb5, 0x0cb9) || in_range(ch, 0x0ce0, 0x0ce1))
    {
        return universal_char_type_valid;   // Kannada
    }

    if (in_range(ch, 0x0d05, 0x0d0c) || in_range(ch, 0x0d0e, 0x0d10) ||
        in_range(ch, 0x0d12, 0x0d28) || in_range(ch, 0x0d2a, 0x0d39) ||
        in_range(ch, 0x0d60, 0x0d61))
    {
        return universal_char_type_valid;   // Malayalam
    }

    if (in_range(ch, 0x0e01, 0x0e30) || in_range(ch, 0x0e32, 0x0e33) ||
        in_range(ch, 0x0e40, 0x0e46) || in_range(ch, 0x0e4f, 0x0e5b))
    {
        return universal_char_type_valid;   // Thai
    }

    return universal_char_type_not_allowed_for_identifiers;
}

///////////////////////////////////////////////////////////////////////////////
//
//  validate_identifier_name
//
//      The validate_identifier_name function tests a given identifier name for
//      its validity with regard to eventually contained universal characters.
//      These should be in valid ranges (see the function
//      classify_universal_char above).
//
//      If the identifier name contains invalid or not allowed universal
//      characters a corresponding lexing_exception is thrown.
//
///////////////////////////////////////////////////////////////////////////////
template <typename StringT>
inline void
validate_identifier_name (StringT const &name, std::size_t line,
    std::size_t column, StringT const &file_name)
{
    using namespace std;    // some systems have strtoul in namespace std::

typename StringT::size_type pos = name.find_first_of('\\');

    while (StringT::npos != pos) {
        // the identifier name contains a backslash (must be universal char)
        BOOST_ASSERT('u' == name[pos+1] || 'U' == name[pos+1]);

        StringT uchar_val(name.substr(pos + 2, ('u' == name[pos + 1]) ? 4 : 8));
        universal_char_type type =
            classify_universal_char(strtoul(uchar_val.c_str(), 0, 16));

        if (universal_char_type_valid != type) {
            // an invalid char was found, so throw an exception
            StringT error_uchar(name.substr(pos, ('u' == name[pos + 1]) ? 6 : 10));

            if (universal_char_type_invalid == type) {
                BOOST_WAVE_LEXER_THROW(lexing_exception, universal_char_invalid,
                    error_uchar, line, column, file_name.c_str());
            }
            else if (universal_char_type_base_charset == type) {
                BOOST_WAVE_LEXER_THROW(lexing_exception, universal_char_base_charset,
                    error_uchar, line, column, file_name.c_str());
            }
            else {
                BOOST_WAVE_LEXER_THROW(lexing_exception, universal_char_not_allowed,
                    error_uchar, line, column, file_name.c_str());
            }
        }

        // find next universal char (if appropriate)
        pos = name.find_first_of('\\', pos+2);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  validate_literal
//
//      The validate_literal function tests a given string or character literal
//      for its validity with regard to eventually contained universal
//      characters. These should be in valid ranges (see the function
//      classify_universal_char above).
//
//      If the string or character literal contains invalid or not allowed
//      universal characters a corresponding lexing_exception is thrown.
//
///////////////////////////////////////////////////////////////////////////////
template <typename StringT>
inline void
validate_literal (StringT const &name, std::size_t line, std::size_t column,
    StringT const &file_name)
{
    using namespace std;    // some systems have strtoul in namespace std::

    typename StringT::size_type pos = name.find_first_of('\\');

    while (StringT::npos != pos) {
        // the literal contains a backslash (may be universal char)
        if ('u' == name[pos+1] || 'U' == name[pos+1]) {
            StringT uchar_val(name.substr(pos + 2, ('u' == name[pos + 1]) ? 4 : 8));
            universal_char_type type =
                classify_universal_char(strtoul(uchar_val.c_str(), 0, 16));

            if (universal_char_type_valid != type &&
                universal_char_type_not_allowed_for_identifiers != type)
            {
                // an invalid char was found, so throw an exception
                StringT error_uchar(name.substr(pos, ('u' == name[pos + 1]) ? 6 : 10));

                if (universal_char_type_invalid == type) {
                    BOOST_WAVE_LEXER_THROW(lexing_exception, universal_char_invalid,
                        error_uchar, line, column, file_name.c_str());
                }
                else {
                    BOOST_WAVE_LEXER_THROW(lexing_exception, universal_char_base_charset,
                        error_uchar, line, column, file_name.c_str());
                }
            }
        }

        // find next universal char (if appropriate)
        pos = name.find_first_of('\\', pos+2);
    }
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_VALIDATE_UNIVERSAL_CHAR_HPP_55F1B811_CD76_4C72_8344_CBC69CF3B339_INCLUDED)
