///////////////////////////////////////////////////////////////////////////////
/// \file regex_constants.hpp
/// Contains definitions for the syntax_option_type, match_flag_type and
/// error_type enumerations.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_REGEX_CONSTANTS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_REGEX_CONSTANTS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/identity.hpp>

#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# define icase icase_
#endif

namespace boost { namespace xpressive { namespace regex_constants
{

/// Flags used to customize the regex syntax
///
enum syntax_option_type
{
    // these flags are required:

    ECMAScript  = 0,        ///< Specifies that the grammar recognized by the regular expression
                            ///< engine uses its normal semantics: that is the same as that given
                            ///< in the ECMA-262, ECMAScript Language Specification, Chapter 15
                            ///< part 10, RegExp (Regular Expression) Objects (FWD.1).
                            ///<
    icase       = 1 << 1,   ///< Specifies that matching of regular expressions against a character
                            ///< container sequence shall be performed without regard to case.
                            ///<
    nosubs      = 1 << 2,   ///< Specifies that when a regular expression is matched against a
                            ///< character container sequence, then no sub-expression matches are to
                            ///< be stored in the supplied match_results structure.
                            ///<
    optimize    = 1 << 3,   ///< Specifies that the regular expression engine should pay more
                            ///< attention to the speed with which regular expressions are matched,
                            ///< and less to the speed with which regular expression objects are
                            ///< constructed. Otherwise it has no detectable effect on the program
                            ///< output.
                            ///<
    collate     = 1 << 4,   ///< Specifies that character ranges of the form "[a-b]" should be
                            ///< locale sensitive.
                            ///<

    // These flags are optional. If the functionality is supported
    // then the flags shall take these names.

    //basic       = 1 << 5,   ///< Specifies that the grammar recognized by the regular expression
    //                        ///< engine is the same as that used by POSIX basic regular expressions
    //                        ///< in IEEE Std 1003.1-2001, Portable Operating System Interface
    //                        ///< (POSIX), Base Definitions and Headers, Section 9, Regular
    //                        ///< Expressions (FWD.1).
    //                        ///<
    //extended    = 1 << 6,   ///< Specifies that the grammar recognized by the regular expression
    //                        ///< engine is the same as that used by POSIX extended regular
    //                        ///< expressions in IEEE Std 1003.1-2001, Portable Operating System
    //                        ///< Interface (POSIX), Base Definitions and Headers, Section 9,
    //                        ///< Regular Expressions (FWD.1).
    //                        ///<
    //awk         = 1 << 7,   ///< Specifies that the grammar recognized by the regular expression
    //                        ///< engine is the same as that used by POSIX utility awk in IEEE Std
    //                        ///< 1003.1-2001, Portable Operating System Interface (POSIX), Shells
    //                        ///< and Utilities, Section 4, awk (FWD.1).
    //                        ///<
    //grep        = 1 << 8,   ///< Specifies that the grammar recognized by the regular expression
    //                        ///< engine is the same as that used by POSIX utility grep in IEEE Std
    //                        ///< 1003.1-2001, Portable Operating System Interface (POSIX),
    //                        ///< Shells and Utilities, Section 4, Utilities, grep (FWD.1).
    //                        ///<
    //egrep       = 1 << 9,   ///< Specifies that the grammar recognized by the regular expression
    //                        ///< engine is the same as that used by POSIX utility grep when given
    //                        ///< the -E option in IEEE Std 1003.1-2001, Portable Operating System
    //                        ///< Interface (POSIX), Shells and Utilities, Section 4, Utilities,
    //                        ///< grep (FWD.1).
    //                        ///<

    // these flags are specific to xpressive, and they help with perl compliance.

    single_line         = 1 << 10,  ///< Specifies that the ^ and \$ metacharacters DO NOT match at
                                    ///< internal line breaks. Note that this is the opposite of the
                                    ///< perl default. It is the inverse of perl's /m (multi-line)
                                    ///< modifier.
                                    ///<
    not_dot_null        = 1 << 11,  ///< Specifies that the . metacharacter does not match the null
                                    ///< character \\0.
                                    ///<
    not_dot_newline     = 1 << 12,  ///< Specifies that the . metacharacter does not match the
                                    ///< newline character \\n.
                                    ///<
    ignore_white_space  = 1 << 13   ///< Specifies that non-escaped white-space is not significant.
                                    ///<
};

/// Flags used to customize the behavior of the regex algorithms
///
enum match_flag_type
{
    match_default           = 0,        ///< Specifies that matching of regular expressions proceeds
                                        ///< without any modification of the normal rules used in
                                        ///< ECMA-262, ECMAScript Language Specification, Chapter 15
                                        ///< part 10, RegExp (Regular Expression) Objects (FWD.1)
                                        ///<
    match_not_bol           = 1 << 1,   ///< Specifies that the expression "^" should not be matched
                                        ///< against the sub-sequence [first,first).
                                        ///<
    match_not_eol           = 1 << 2,   ///< Specifies that the expression "\$" should not be
                                        ///< matched against the sub-sequence [last,last).
                                        ///<
    match_not_bow           = 1 << 3,   ///< Specifies that the expression "\\b" should not be
                                        ///< matched against the sub-sequence [first,first).
                                        ///<
    match_not_eow           = 1 << 4,   ///< Specifies that the expression "\\b" should not be
                                        ///< matched against the sub-sequence [last,last).
                                        ///<
    match_any               = 1 << 7,   ///< Specifies that if more than one match is possible then
                                        ///< any match is an acceptable result.
                                        ///<
    match_not_null          = 1 << 8,   ///< Specifies that the expression can not be matched
                                        ///< against an empty sequence.
                                        ///<
    match_continuous        = 1 << 10,  ///< Specifies that the expression must match a sub-sequence
                                        ///< that begins at first.
                                        ///<
    match_partial           = 1 << 11,  ///< Specifies that if no match can be found, then it is
                                        ///< acceptable to return a match [from, last) where
                                        ///< from != last, if there exists some sequence of characters
                                        ///< [from,to) of which [from,last) is a prefix, and which
                                        ///< would result in a full match.
                                        ///<
    match_prev_avail        = 1 << 12,  ///< Specifies that --first is a valid iterator position,
                                        ///< when this flag is set then the flags match_not_bol
                                        ///< and match_not_bow are ignored by the regular expression
                                        ///< algorithms (RE.7) and iterators (RE.8).
                                        ///<
    format_default          = 0,        ///< Specifies that when a regular expression match is to be
                                        ///< replaced by a new string, that the new string is
                                        ///< constructed using the rules used by the ECMAScript
                                        ///< replace function in ECMA-262, ECMAScript Language
                                        ///< Specification, Chapter 15 part 5.4.11
                                        ///< String.prototype.replace. (FWD.1). In addition during
                                        ///< search and replace operations then all non-overlapping
                                        ///< occurrences of the regular expression are located and
                                        ///< replaced, and sections of the input that did not match
                                        ///< the expression, are copied unchanged to the output
                                        ///< string.
                                        ///<
    format_sed              = 1 << 13,  ///< Specifies that when a regular expression match is to be
                                        ///< replaced by a new string, that the new string is
                                        ///< constructed using the rules used by the Unix sed
                                        ///< utility in IEEE Std 1003.1-2001, Portable Operating
                                        ///< SystemInterface (POSIX), Shells and Utilities.
                                        ///<
    format_perl             = 1 << 14,  ///< Specifies that when a regular expression match is to be
                                        ///< replaced by a new string, that the new string is
                                        ///< constructed using an implementation defined superset
                                        ///< of the rules used by the ECMAScript replace function in
                                        ///< ECMA-262, ECMAScript Language Specification, Chapter 15
                                        ///< part 5.4.11 String.prototype.replace (FWD.1).
                                        ///<
    format_no_copy          = 1 << 15,  ///< When specified during a search and replace operation,
                                        ///< then sections of the character container sequence being
                                        ///< searched that do match the regular expression, are not
                                        ///< copied to the output string.
                                        ///<
    format_first_only       = 1 << 16,  ///< When specified during a search and replace operation,
                                        ///< then only the first occurrence of the regular
                                        ///< expression is replaced.
                                        ///<
    format_literal          = 1 << 17,  ///< Treat the format string as a literal.
                                        ///<
    format_all              = 1 << 18   ///< Specifies that all syntax extensions are enabled,
                                        ///< including conditional (?ddexpression1:expression2)
                                        ///< replacements.
                                        ///<
};

/// Error codes used by the regex_error type
///
enum error_type
{
    error_collate,              ///< The expression contained an invalid collating element name.
                                ///<
    error_ctype,                ///< The expression contained an invalid character class name.
                                ///<
    error_escape,               ///< The expression contained an invalid escaped character,
                                ///< or a trailing escape.
                                ///<
    error_subreg,               ///< The expression contained an invalid back-reference.
                                ///<
    error_brack,                ///< The expression contained mismatched [ and ].
                                ///<
    error_paren,                ///< The expression contained mismatched ( and ).
                                ///<
    error_brace,                ///< The expression contained mismatched { and }.
                                ///<
    error_badbrace,             ///< The expression contained an invalid range in a {} expression.
                                ///<
    error_range,                ///< The expression contained an invalid character range, for
                                ///< example [b-a].
                                ///<
    error_space,                ///< There was insufficient memory to convert the expression into a
                                ///< finite state machine.
                                ///<
    error_badrepeat,            ///< One of *?+{ was not preceded by a valid regular expression.
                                ///<
    error_complexity,           ///< The complexity of an attempted match against a regular
                                ///< expression exceeded a pre-set level.
                                ///<
    error_stack,                ///< There was insufficient memory to determine whether the regular
                                ///< expression could match the specified character sequence.
                                ///<
    error_badref,               ///< An nested regex is uninitialized.
                                ///<
    error_badmark,              ///< An invalid use of a named capture.
                                ///<
    error_badlookbehind,        ///< An attempt to create a variable-width look-behind assertion
                                ///< was detected.
                                ///<
    error_badrule,              ///< An invalid use of a rule was detected.
                                ///<
    error_badarg,               ///< An argument to an action was unbound.
                                ///<
    error_badattr,              ///< Tried to read from an uninitialized attribute.
                                ///<
    error_internal              ///< An internal error has occurred.
                                ///<
};

/// INTERNAL ONLY
inline syntax_option_type operator &(syntax_option_type b1, syntax_option_type b2)
{
    return static_cast<syntax_option_type>(
        static_cast<int>(b1) & static_cast<int>(b2));
}

/// INTERNAL ONLY
inline syntax_option_type operator |(syntax_option_type b1, syntax_option_type b2)
{
    return static_cast<syntax_option_type>(static_cast<int>(b1) | static_cast<int>(b2));
}

/// INTERNAL ONLY
inline syntax_option_type operator ^(syntax_option_type b1, syntax_option_type b2)
{
    return static_cast<syntax_option_type>(static_cast<int>(b1) ^ static_cast<int>(b2));
}

/// INTERNAL ONLY
inline syntax_option_type operator ~(syntax_option_type b)
{
    return static_cast<syntax_option_type>(~static_cast<int>(b));
}

/// INTERNAL ONLY
inline match_flag_type operator &(match_flag_type b1, match_flag_type b2)
{
    return static_cast<match_flag_type>(static_cast<int>(b1) & static_cast<int>(b2));
}

/// INTERNAL ONLY
inline match_flag_type operator |(match_flag_type b1, match_flag_type b2)
{
    return static_cast<match_flag_type>(static_cast<int>(b1) | static_cast<int>(b2));
}

/// INTERNAL ONLY
inline match_flag_type operator ^(match_flag_type b1, match_flag_type b2)
{
    return static_cast<match_flag_type>(static_cast<int>(b1) ^ static_cast<int>(b2));
}

/// INTERNAL ONLY
inline match_flag_type operator ~(match_flag_type b)
{
    return static_cast<match_flag_type>(~static_cast<int>(b));
}

}}} // namespace boost::xpressive::regex_constants

#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# undef icase
#endif

#endif
