/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_traits_defaults.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares API's for access to regex_traits default properties.
  */

#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#define BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED

#include <boost/regex/config.hpp>

#include <boost/regex/v5/syntax_type.hpp>
#include <boost/regex/v5/error_type.hpp>
#include <boost/regex/v5/regex_workaround.hpp>
#include <type_traits>
#include <cstdint>
#include <cctype>
#include <locale>
#include <cwctype>
#include <limits>

namespace boost{ namespace BOOST_REGEX_DETAIL_NS{


//
// helpers to suppress warnings:
//
template <class charT>
inline bool is_extended(charT c)
{
   typedef typename std::make_unsigned<charT>::type unsigned_type; 
   return (sizeof(charT) > 1) && (static_cast<unsigned_type>(c) >= 256u); 
}
inline bool is_extended(char)
{ return false; }

inline const char*  get_default_syntax(regex_constants::syntax_type n)
{
   // if the user hasn't supplied a message catalog, then this supplies
   // default "messages" for us to load in the range 1-100.
   const char* messages[] = {
         "",
         "(",
         ")",
         "$",
         "^",
         ".",
         "*",
         "+",
         "?",
         "[",
         "]",
         "|",
         "\\",
         "#",
         "-",
         "{",
         "}",
         "0123456789",
         "b",
         "B",
         "<",
         ">",
         "",
         "",
         "A`",
         "z'",
         "\n",
         ",",
         "a",
         "f",
         "n",
         "r",
         "t",
         "v",
         "x",
         "c",
         ":",
         "=",
         "e",
         "",
         "",
         "",
         "",
         "",
         "",
         "",
         "",
         "E",
         "Q",
         "X",
         "C",
         "Z",
         "G",
         "!",
         "p",
         "P",
         "N",
         "gk",
         "K",
         "R",
   };

   return ((n >= (sizeof(messages) / sizeof(messages[1]))) ? "" : messages[n]);
}

inline const char*  get_default_error_string(regex_constants::error_type n)
{
   static const char* const s_default_error_messages[] = {
      "Success",                                                            /* REG_NOERROR 0 error_ok */
      "No match",                                                           /* REG_NOMATCH 1 error_no_match */
      "Invalid regular expression.",                                        /* REG_BADPAT 2 error_bad_pattern */
      "Invalid collation character.",                                       /* REG_ECOLLATE 3 error_collate */
      "Invalid character class name, collating name, or character range.",  /* REG_ECTYPE 4 error_ctype */
      "Invalid or unterminated escape sequence.",                           /* REG_EESCAPE 5 error_escape */
      "Invalid back reference: specified capturing group does not exist.",  /* REG_ESUBREG 6 error_backref */
      "Unmatched [ or [^ in character class declaration.",                  /* REG_EBRACK 7 error_brack */
      "Unmatched marking parenthesis ( or \\(.",                            /* REG_EPAREN 8 error_paren */
      "Unmatched quantified repeat operator { or \\{.",                     /* REG_EBRACE 9 error_brace */
      "Invalid content of repeat range.",                                   /* REG_BADBR 10 error_badbrace */
      "Invalid range end in character class",                               /* REG_ERANGE 11 error_range */
      "Out of memory.",                                                     /* REG_ESPACE 12 error_space NOT USED */
      "Invalid preceding regular expression prior to repetition operator.", /* REG_BADRPT 13 error_badrepeat */
      "Premature end of regular expression",                                /* REG_EEND 14 error_end NOT USED */
      "Regular expression is too large.",                                   /* REG_ESIZE 15 error_size NOT USED */
      "Unmatched ) or \\)",                                                 /* REG_ERPAREN 16 error_right_paren NOT USED */
      "Empty regular expression.",                                          /* REG_EMPTY 17 error_empty */
      "The complexity of matching the regular expression exceeded predefined bounds.  "
      "Try refactoring the regular expression to make each choice made by the state machine unambiguous.  "
      "This exception is thrown to prevent \"eternal\" matches that take an "
      "indefinite period time to locate.",                                  /* REG_ECOMPLEXITY 18 error_complexity */
      "Ran out of stack space trying to match the regular expression.",     /* REG_ESTACK 19 error_stack */
      "Invalid or unterminated Perl (?...) sequence.",                      /* REG_E_PERL 20 error_perl */
      "Unknown error.",                                                     /* REG_E_UNKNOWN 21 error_unknown */
   };

   return (n > ::boost::regex_constants::error_unknown) ? s_default_error_messages[::boost::regex_constants::error_unknown] : s_default_error_messages[n];
}

inline regex_constants::syntax_type  get_default_syntax_type(char c)
{
   //
   // char_syntax determines how the compiler treats a given character
   // in a regular expression.
   //
   static regex_constants::syntax_type char_syntax[] = {
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_newline,     /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /* */    // 32
      regex_constants::syntax_not,        /*!*/
      regex_constants::syntax_char,        /*"*/
      regex_constants::syntax_hash,        /*#*/
      regex_constants::syntax_dollar,        /*$*/
      regex_constants::syntax_char,        /*%*/
      regex_constants::syntax_char,        /*&*/
      regex_constants::escape_type_end_buffer,  /*'*/
      regex_constants::syntax_open_mark,        /*(*/
      regex_constants::syntax_close_mark,        /*)*/
      regex_constants::syntax_star,        /***/
      regex_constants::syntax_plus,        /*+*/
      regex_constants::syntax_comma,        /*,*/
      regex_constants::syntax_dash,        /*-*/
      regex_constants::syntax_dot,        /*.*/
      regex_constants::syntax_char,        /*/*/
      regex_constants::syntax_digit,        /*0*/
      regex_constants::syntax_digit,        /*1*/
      regex_constants::syntax_digit,        /*2*/
      regex_constants::syntax_digit,        /*3*/
      regex_constants::syntax_digit,        /*4*/
      regex_constants::syntax_digit,        /*5*/
      regex_constants::syntax_digit,        /*6*/
      regex_constants::syntax_digit,        /*7*/
      regex_constants::syntax_digit,        /*8*/
      regex_constants::syntax_digit,        /*9*/
      regex_constants::syntax_colon,        /*:*/
      regex_constants::syntax_char,        /*;*/
      regex_constants::escape_type_left_word, /*<*/
      regex_constants::syntax_equal,        /*=*/
      regex_constants::escape_type_right_word, /*>*/
      regex_constants::syntax_question,        /*?*/
      regex_constants::syntax_char,        /*@*/
      regex_constants::syntax_char,        /*A*/
      regex_constants::syntax_char,        /*B*/
      regex_constants::syntax_char,        /*C*/
      regex_constants::syntax_char,        /*D*/
      regex_constants::syntax_char,        /*E*/
      regex_constants::syntax_char,        /*F*/
      regex_constants::syntax_char,        /*G*/
      regex_constants::syntax_char,        /*H*/
      regex_constants::syntax_char,        /*I*/
      regex_constants::syntax_char,        /*J*/
      regex_constants::syntax_char,        /*K*/
      regex_constants::syntax_char,        /*L*/
      regex_constants::syntax_char,        /*M*/
      regex_constants::syntax_char,        /*N*/
      regex_constants::syntax_char,        /*O*/
      regex_constants::syntax_char,        /*P*/
      regex_constants::syntax_char,        /*Q*/
      regex_constants::syntax_char,        /*R*/
      regex_constants::syntax_char,        /*S*/
      regex_constants::syntax_char,        /*T*/
      regex_constants::syntax_char,        /*U*/
      regex_constants::syntax_char,        /*V*/
      regex_constants::syntax_char,        /*W*/
      regex_constants::syntax_char,        /*X*/
      regex_constants::syntax_char,        /*Y*/
      regex_constants::syntax_char,        /*Z*/
      regex_constants::syntax_open_set,        /*[*/
      regex_constants::syntax_escape,        /*\*/
      regex_constants::syntax_close_set,        /*]*/
      regex_constants::syntax_caret,        /*^*/
      regex_constants::syntax_char,        /*_*/
      regex_constants::syntax_char,        /*`*/
      regex_constants::syntax_char,        /*a*/
      regex_constants::syntax_char,        /*b*/
      regex_constants::syntax_char,        /*c*/
      regex_constants::syntax_char,        /*d*/
      regex_constants::syntax_char,        /*e*/
      regex_constants::syntax_char,        /*f*/
      regex_constants::syntax_char,        /*g*/
      regex_constants::syntax_char,        /*h*/
      regex_constants::syntax_char,        /*i*/
      regex_constants::syntax_char,        /*j*/
      regex_constants::syntax_char,        /*k*/
      regex_constants::syntax_char,        /*l*/
      regex_constants::syntax_char,        /*m*/
      regex_constants::syntax_char,        /*n*/
      regex_constants::syntax_char,        /*o*/
      regex_constants::syntax_char,        /*p*/
      regex_constants::syntax_char,        /*q*/
      regex_constants::syntax_char,        /*r*/
      regex_constants::syntax_char,        /*s*/
      regex_constants::syntax_char,        /*t*/
      regex_constants::syntax_char,        /*u*/
      regex_constants::syntax_char,        /*v*/
      regex_constants::syntax_char,        /*w*/
      regex_constants::syntax_char,        /*x*/
      regex_constants::syntax_char,        /*y*/
      regex_constants::syntax_char,        /*z*/
      regex_constants::syntax_open_brace,        /*{*/
      regex_constants::syntax_or,        /*|*/
      regex_constants::syntax_close_brace,        /*}*/
      regex_constants::syntax_char,        /*~*/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
      regex_constants::syntax_char,        /**/
   };

   return char_syntax[(unsigned char)c];
}

inline regex_constants::escape_syntax_type  get_default_escape_syntax_type(char c)
{
   //
   // char_syntax determines how the compiler treats a given character
   // in a regular expression.
   //
   static regex_constants::escape_syntax_type char_syntax[] = {
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,     /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /* */    // 32
      regex_constants::escape_type_identity,        /*!*/
      regex_constants::escape_type_identity,        /*"*/
      regex_constants::escape_type_identity,        /*#*/
      regex_constants::escape_type_identity,        /*$*/
      regex_constants::escape_type_identity,        /*%*/
      regex_constants::escape_type_identity,        /*&*/
      regex_constants::escape_type_end_buffer,        /*'*/
      regex_constants::syntax_open_mark,        /*(*/
      regex_constants::syntax_close_mark,        /*)*/
      regex_constants::escape_type_identity,        /***/
      regex_constants::syntax_plus,                 /*+*/
      regex_constants::escape_type_identity,        /*,*/
      regex_constants::escape_type_identity,        /*-*/
      regex_constants::escape_type_identity,        /*.*/
      regex_constants::escape_type_identity,        /*/*/
      regex_constants::escape_type_decimal,        /*0*/
      regex_constants::escape_type_backref,        /*1*/
      regex_constants::escape_type_backref,        /*2*/
      regex_constants::escape_type_backref,        /*3*/
      regex_constants::escape_type_backref,        /*4*/
      regex_constants::escape_type_backref,        /*5*/
      regex_constants::escape_type_backref,        /*6*/
      regex_constants::escape_type_backref,        /*7*/
      regex_constants::escape_type_backref,        /*8*/
      regex_constants::escape_type_backref,        /*9*/
      regex_constants::escape_type_identity,        /*:*/
      regex_constants::escape_type_identity,        /*;*/
      regex_constants::escape_type_left_word,        /*<*/
      regex_constants::escape_type_identity,        /*=*/
      regex_constants::escape_type_right_word,        /*>*/
      regex_constants::syntax_question,              /*?*/
      regex_constants::escape_type_identity,         /*@*/
      regex_constants::escape_type_start_buffer,     /*A*/
      regex_constants::escape_type_not_word_assert,  /*B*/
      regex_constants::escape_type_C,                /*C*/
      regex_constants::escape_type_not_class,        /*D*/
      regex_constants::escape_type_E,                /*E*/
      regex_constants::escape_type_not_class,        /*F*/
      regex_constants::escape_type_G,                /*G*/
      regex_constants::escape_type_not_class,        /*H*/
      regex_constants::escape_type_not_class,        /*I*/
      regex_constants::escape_type_not_class,        /*J*/
      regex_constants::escape_type_reset_start_mark, /*K*/
      regex_constants::escape_type_not_class,        /*L*/
      regex_constants::escape_type_not_class,        /*M*/
      regex_constants::escape_type_named_char,       /*N*/
      regex_constants::escape_type_not_class,        /*O*/
      regex_constants::escape_type_not_property,     /*P*/
      regex_constants::escape_type_Q,                /*Q*/
      regex_constants::escape_type_line_ending,      /*R*/
      regex_constants::escape_type_not_class,        /*S*/
      regex_constants::escape_type_not_class,        /*T*/
      regex_constants::escape_type_not_class,        /*U*/
      regex_constants::escape_type_not_class,        /*V*/
      regex_constants::escape_type_not_class,        /*W*/
      regex_constants::escape_type_X,                /*X*/
      regex_constants::escape_type_not_class,        /*Y*/
      regex_constants::escape_type_Z,                /*Z*/
      regex_constants::escape_type_identity,        /*[*/
      regex_constants::escape_type_identity,        /*\*/
      regex_constants::escape_type_identity,        /*]*/
      regex_constants::escape_type_identity,        /*^*/
      regex_constants::escape_type_identity,        /*_*/
      regex_constants::escape_type_start_buffer,        /*`*/
      regex_constants::escape_type_control_a,        /*a*/
      regex_constants::escape_type_word_assert,        /*b*/
      regex_constants::escape_type_ascii_control,        /*c*/
      regex_constants::escape_type_class,        /*d*/
      regex_constants::escape_type_e,        /*e*/
      regex_constants::escape_type_control_f,       /*f*/
      regex_constants::escape_type_extended_backref,  /*g*/
      regex_constants::escape_type_class,        /*h*/
      regex_constants::escape_type_class,        /*i*/
      regex_constants::escape_type_class,        /*j*/
      regex_constants::escape_type_extended_backref, /*k*/
      regex_constants::escape_type_class,        /*l*/
      regex_constants::escape_type_class,        /*m*/
      regex_constants::escape_type_control_n,       /*n*/
      regex_constants::escape_type_class,           /*o*/
      regex_constants::escape_type_property,        /*p*/
      regex_constants::escape_type_class,           /*q*/
      regex_constants::escape_type_control_r,       /*r*/
      regex_constants::escape_type_class,           /*s*/
      regex_constants::escape_type_control_t,       /*t*/
      regex_constants::escape_type_class,         /*u*/
      regex_constants::escape_type_control_v,       /*v*/
      regex_constants::escape_type_class,           /*w*/
      regex_constants::escape_type_hex,             /*x*/
      regex_constants::escape_type_class,           /*y*/
      regex_constants::escape_type_end_buffer,      /*z*/
      regex_constants::syntax_open_brace,           /*{*/
      regex_constants::syntax_or,                   /*|*/
      regex_constants::syntax_close_brace,          /*}*/
      regex_constants::escape_type_identity,        /*~*/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
      regex_constants::escape_type_identity,        /**/
   };

   return char_syntax[(unsigned char)c];
}

// is charT c a combining character?
inline bool  is_combining_implementation(std::uint_least16_t c)
{
   const std::uint_least16_t combining_ranges[] = { 0x0300, 0x0361,
                           0x0483, 0x0486,
                           0x0903, 0x0903,
                           0x093E, 0x0940,
                           0x0949, 0x094C,
                           0x0982, 0x0983,
                           0x09BE, 0x09C0,
                           0x09C7, 0x09CC,
                           0x09D7, 0x09D7,
                           0x0A3E, 0x0A40,
                           0x0A83, 0x0A83,
                           0x0ABE, 0x0AC0,
                           0x0AC9, 0x0ACC,
                           0x0B02, 0x0B03,
                           0x0B3E, 0x0B3E,
                           0x0B40, 0x0B40,
                           0x0B47, 0x0B4C,
                           0x0B57, 0x0B57,
                           0x0B83, 0x0B83,
                           0x0BBE, 0x0BBF,
                           0x0BC1, 0x0BCC,
                           0x0BD7, 0x0BD7,
                           0x0C01, 0x0C03,
                           0x0C41, 0x0C44,
                           0x0C82, 0x0C83,
                           0x0CBE, 0x0CBE,
                           0x0CC0, 0x0CC4,
                           0x0CC7, 0x0CCB,
                           0x0CD5, 0x0CD6,
                           0x0D02, 0x0D03,
                           0x0D3E, 0x0D40,
                           0x0D46, 0x0D4C,
                           0x0D57, 0x0D57,
                           0x0F7F, 0x0F7F,
                           0x20D0, 0x20E1,
                           0x3099, 0x309A,
                           0xFE20, 0xFE23,
                           0xffff, 0xffff, };

   const std::uint_least16_t* p = combining_ranges + 1;
   while (*p < c) p += 2;
   --p;
   if ((c >= *p) && (c <= *(p + 1)))
      return true;
   return false;
}

template <class charT>
inline bool is_combining(charT c)
{
   return (c <= static_cast<charT>(0)) ? false : ((c >= static_cast<charT>((std::numeric_limits<uint_least16_t>::max)())) ? false : is_combining_implementation(static_cast<unsigned short>(c)));
}
template <>
inline bool is_combining<char>(char)
{
   return false;
}
template <>
inline bool is_combining<signed char>(signed char)
{
   return false;
}
template <>
inline bool is_combining<unsigned char>(unsigned char)
{
   return false;
}
#ifdef _MSC_VER
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return is_combining_implementation(static_cast<unsigned short>(c));
}
#elif !defined(__DECCXX) && !defined(__osf__) && !defined(__OSF__) && defined(WCHAR_MIN) && (WCHAR_MIN == 0) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
#if defined(WCHAR_MAX) && (WCHAR_MAX <= USHRT_MAX)
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return is_combining_implementation(static_cast<unsigned short>(c));
}
#else
template<>
inline bool is_combining<wchar_t>(wchar_t c)
{
   return (c >= (std::numeric_limits<uint_least16_t>::max)()) ? false : is_combining_implementation(static_cast<unsigned short>(c));
}
#endif
#endif

//
// is a charT c a line separator?
//
template <class charT>
inline bool is_separator(charT c)
{
   return BOOST_REGEX_MAKE_BOOL(
      (c == static_cast<charT>('\n'))
      || (c == static_cast<charT>('\r'))
      || (c == static_cast<charT>('\f'))
      || (static_cast<std::uint16_t>(c) == 0x2028u)
      || (static_cast<std::uint16_t>(c) == 0x2029u)
      || (static_cast<std::uint16_t>(c) == 0x85u));
}
template <>
inline bool is_separator<char>(char c)
{
   return BOOST_REGEX_MAKE_BOOL((c == '\n') || (c == '\r') || (c == '\f'));
}

//
// get a default collating element:
//
inline std::string  lookup_default_collate_name(const std::string& name)
{
   //
   // these are the POSIX collating names:
   //
   static const char* def_coll_names[] = {
   "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "alert", "backspace", "tab", "newline",
   "vertical-tab", "form-feed", "carriage-return", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK",
   "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "IS4", "IS3", "IS2", "IS1", "space", "exclamation-mark",
   "quotation-mark", "number-sign", "dollar-sign", "percent-sign", "ampersand", "apostrophe",
   "left-parenthesis", "right-parenthesis", "asterisk", "plus-sign", "comma", "hyphen",
   "period", "slash", "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
   "colon", "semicolon", "less-than-sign", "equals-sign", "greater-than-sign",
   "question-mark", "commercial-at", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
   "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "left-square-bracket", "backslash",
   "right-square-bracket", "circumflex", "underscore", "grave-accent", "a", "b", "c", "d", "e", "f",
   "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "left-curly-bracket",
   "vertical-line", "right-curly-bracket", "tilde", "DEL", "",
   };

   // these multi-character collating elements
   // should keep most Western-European locales
   // happy - we should really localise these a
   // little more - but this will have to do for
   // now:

   static const char* def_multi_coll[] = {
      "ae",
      "Ae",
      "AE",
      "ch",
      "Ch",
      "CH",
      "ll",
      "Ll",
      "LL",
      "ss",
      "Ss",
      "SS",
      "nj",
      "Nj",
      "NJ",
      "dz",
      "Dz",
      "DZ",
      "lj",
      "Lj",
      "LJ",
      "",
   };

   unsigned int i = 0;
   while (*def_coll_names[i])
   {
      if (def_coll_names[i] == name)
      {
         return std::string(1, char(i));
      }
      ++i;
   }
   i = 0;
   while (*def_multi_coll[i])
   {
      if (def_multi_coll[i] == name)
      {
         return def_multi_coll[i];
      }
      ++i;
   }
   return std::string();
}

//
// get the state_id of a character classification, the individual
// traits classes then transform that state_id into a bitmask:
//
template <class charT>
struct character_pointer_range
{
   const charT* p1;
   const charT* p2;

   bool operator < (const character_pointer_range& r)const
   {
      return std::lexicographical_compare(p1, p2, r.p1, r.p2);
   }
   bool operator == (const character_pointer_range& r)const
   {
      // Not only do we check that the ranges are of equal size before
      // calling std::equal, but there is no other algorithm available:
      // not even a non-standard MS one.  So forward to unchecked_equal
      // in the MS case.
#ifdef __cpp_lib_robust_nonmodifying_seq_ops
      return std::equal(p1, p2, r.p1, r.p2);
#elif defined(BOOST_REGEX_MSVC)
      if (((p2 - p1) != (r.p2 - r.p1)))
         return false;
      const charT* with = r.p1;
      const charT* pos = p1;
      while (pos != p2)
         if (*pos++ != *with++) return false;
      return true;

#else
      return ((p2 - p1) == (r.p2 - r.p1)) && std::equal(p1, p2, r.p1);
#endif
   }
};
template <class charT>
int get_default_class_id(const charT* p1, const charT* p2)
{
   static const charT data[73] = {
      'a', 'l', 'n', 'u', 'm',
      'a', 'l', 'p', 'h', 'a',
      'b', 'l', 'a', 'n', 'k',
      'c', 'n', 't', 'r', 'l',
      'd', 'i', 'g', 'i', 't',
      'g', 'r', 'a', 'p', 'h',
      'l', 'o', 'w', 'e', 'r',
      'p', 'r', 'i', 'n', 't',
      'p', 'u', 'n', 'c', 't',
      's', 'p', 'a', 'c', 'e',
      'u', 'n', 'i', 'c', 'o', 'd', 'e',
      'u', 'p', 'p', 'e', 'r',
      'v',
      'w', 'o', 'r', 'd',
      'x', 'd', 'i', 'g', 'i', 't',
   };

   static const character_pointer_range<charT> ranges[21] =
   {
      {data+0, data+5,}, // alnum
      {data+5, data+10,}, // alpha
      {data+10, data+15,}, // blank
      {data+15, data+20,}, // cntrl
      {data+20, data+21,}, // d
      {data+20, data+25,}, // digit
      {data+25, data+30,}, // graph
      {data+29, data+30,}, // h
      {data+30, data+31,}, // l
      {data+30, data+35,}, // lower
      {data+35, data+40,}, // print
      {data+40, data+45,}, // punct
      {data+45, data+46,}, // s
      {data+45, data+50,}, // space
      {data+57, data+58,}, // u
      {data+50, data+57,}, // unicode
      {data+57, data+62,}, // upper
      {data+62, data+63,}, // v
      {data+63, data+64,}, // w
      {data+63, data+67,}, // word
      {data+67, data+73,}, // xdigit
   };
   const character_pointer_range<charT>* ranges_begin = ranges;
   const character_pointer_range<charT>* ranges_end = ranges + (sizeof(ranges)/sizeof(ranges[0]));

   character_pointer_range<charT> t = { p1, p2, };
   const character_pointer_range<charT>* p = std::lower_bound(ranges_begin, ranges_end, t);
   if((p != ranges_end) && (t == *p))
      return static_cast<int>(p - ranges);
   return -1;
}

//
// helper functions:
//
template <class charT>
std::ptrdiff_t global_length(const charT* p)
{
   std::ptrdiff_t n = 0;
   while(*p)
   {
      ++p;
      ++n;
   }
   return n;
}
template<>
inline std::ptrdiff_t global_length<char>(const char* p)
{
   return (std::strlen)(p);
}
#ifndef BOOST_NO_WREGEX
template<>
inline std::ptrdiff_t global_length<wchar_t>(const wchar_t* p)
{
   return (std::ptrdiff_t)(std::wcslen)(p);
}
#endif
template <class charT>
inline charT  global_lower(charT c)
{
   return c;
}
template <class charT>
inline charT  global_upper(charT c)
{
   return c;
}

inline char  do_global_lower(char c)
{
   return static_cast<char>((std::tolower)((unsigned char)c));
}

inline char  do_global_upper(char c)
{
   return static_cast<char>((std::toupper)((unsigned char)c));
}
#ifndef BOOST_NO_WREGEX
inline wchar_t  do_global_lower(wchar_t c)
{
   return (std::towlower)(c);
}

inline wchar_t  do_global_upper(wchar_t c)
{
   return (std::towupper)(c);
}
#endif
//
// This sucks: declare template specialisations of global_lower/global_upper
// that just forward to the non-template implementation functions.  We do
// this because there is one compiler (Compaq Tru64 C++) that doesn't seem
// to differentiate between templates and non-template overloads....
// what's more, the primary template, plus all overloads have to be
// defined in the same translation unit (if one is inline they all must be)
// otherwise the "local template instantiation" compiler option can pick
// the wrong instantiation when linking:
//
template<> inline char  global_lower<char>(char c) { return do_global_lower(c); }
template<> inline char  global_upper<char>(char c) { return do_global_upper(c); }
#ifndef BOOST_NO_WREGEX
template<> inline wchar_t  global_lower<wchar_t>(wchar_t c) { return do_global_lower(c); }
template<> inline wchar_t  global_upper<wchar_t>(wchar_t c) { return do_global_upper(c); }
#endif

template <class charT>
int global_value(charT c)
{
   static const charT zero = '0';
   static const charT nine = '9';
   static const charT a = 'a';
   static const charT f = 'f';
   static const charT A = 'A';
   static const charT F = 'F';

   if(c > f) return -1;
   if(c >= a) return 10 + (c - a);
   if(c > F) return -1;
   if(c >= A) return 10 + (c - A);
   if(c > nine) return -1;
   if(c >= zero) return c - zero;
   return -1;
}
template <class charT, class traits>
std::intmax_t global_toi(const charT*& p1, const charT* p2, int radix, const traits& t)
{
   (void)t; // warning suppression
   std::intmax_t limit = (std::numeric_limits<std::intmax_t>::max)() / radix;
   std::intmax_t next_value = t.value(*p1, radix);
   if((p1 == p2) || (next_value < 0) || (next_value >= radix))
      return -1;
   std::intmax_t result = 0;
   while(p1 != p2)
   {
      next_value = t.value(*p1, radix);
      if((next_value < 0) || (next_value >= radix))
         break;
      result *= radix;
      result += next_value;
      ++p1;
      if (result > limit)
         return -1;
   }
   return result;
}

template <class charT>
inline typename std::enable_if<(sizeof(charT) > 1), const charT*>::type get_escape_R_string()
{
#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#  pragma warning(disable:4309 4245)
#endif
   static const charT e1[] = { '(', '?', '-', 'x', ':', '(', '?', '>', '\x0D', '\x0A', '?',
      '|', '[', '\x0A', '\x0B', '\x0C', static_cast<charT>(0x85), static_cast<charT>(0x2028),
      static_cast<charT>(0x2029), ']', ')', ')', '\0' };
   static const charT e2[] = { '(', '?', '-', 'x', ':', '(', '?', '>', '\x0D', '\x0A', '?',
      '|', '[', '\x0A', '\x0B', '\x0C', static_cast<charT>(0x85), ']', ')', ')', '\0' };

   charT c = static_cast<charT>(0x2029u);
   bool b = (static_cast<unsigned>(c) == 0x2029u);

   return (b ? e1 : e2);
#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif
}

template <class charT>
inline typename std::enable_if<(sizeof(charT) == 1), const charT*>::type get_escape_R_string()
{
#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#  pragma warning(disable:4309 4245)
#endif
   static const charT e2[] = { 
      static_cast<charT>('('), 
      static_cast<charT>('?'), 
      static_cast<charT>('-'), 
      static_cast<charT>('x'), 
      static_cast<charT>(':'), 
      static_cast<charT>('('), 
      static_cast<charT>('?'), 
      static_cast<charT>('>'), 
      static_cast<charT>('\x0D'), 
      static_cast<charT>('\x0A'), 
      static_cast<charT>('?'),
      static_cast<charT>('|'), 
      static_cast<charT>('['), 
      static_cast<charT>('\x0A'), 
      static_cast<charT>('\x0B'), 
      static_cast<charT>('\x0C'), 
      static_cast<charT>('\x85'), 
      static_cast<charT>(']'), 
      static_cast<charT>(')'), 
      static_cast<charT>(')'), 
      static_cast<charT>('\0') 
   };
   return e2;
#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif
}

} // BOOST_REGEX_DETAIL_NS
} // boost

#endif
