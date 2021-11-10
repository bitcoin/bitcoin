///////////////////////////////////////////////////////////////////////////////
// parser_enum.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_DYNAMIC_PARSER_ENUM_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_DYNAMIC_PARSER_ENUM_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

namespace boost { namespace xpressive { namespace regex_constants
{

///////////////////////////////////////////////////////////////////////////////
// compiler_token_type
//
enum compiler_token_type
{
    token_literal,
    token_any,                          // .
    token_escape,                       //
    token_group_begin,                  // (
    token_group_end,                    // )
    token_alternate,                    // |
    token_invalid_quantifier,           // {
    token_charset_begin,                // [
    token_charset_end,                  // ]
    token_charset_invert,               // ^
    token_charset_hyphen,               // -
    token_charset_backspace,            // \b
    token_posix_charset_begin,          // [:
    token_posix_charset_end,            // :]
    token_equivalence_class_begin,      // [=
    token_equivalence_class_end,        // =]
    token_collation_element_begin,      // [.
    token_collation_element_end,        // .]

    token_quote_meta_begin,             // \Q
    token_quote_meta_end,               // \E

    token_no_mark,                      // ?:
    token_positive_lookahead,           // ?=
    token_negative_lookahead,           // ?!
    token_positive_lookbehind,          // ?<=
    token_negative_lookbehind,          // ?<!
    token_independent_sub_expression,   // ?>
    token_comment,                      // ?#
    token_recurse,                      // ?R
    token_rule_assign,                  // ?$[name]=
    token_rule_ref,                     // ?$[name]
    token_named_mark,                   // ?P<name>
    token_named_mark_ref,               // ?P=name

    token_assert_begin_sequence,        // \A
    token_assert_end_sequence,          // \Z
    token_assert_begin_line,            // ^
    token_assert_end_line,              // $
    token_assert_word_begin,            // \<
    token_assert_word_end,              // \>
    token_assert_word_boundary,         // \b
    token_assert_not_word_boundary,     // \B

    token_escape_newline,               // \n
    token_escape_escape,                // \e
    token_escape_formfeed,              // \f
    token_escape_horizontal_tab,        // \t
    token_escape_vertical_tab,          // \v
    token_escape_bell,                  // \a
    token_escape_control,               // \c

    token_end_of_pattern
};

}}} // namespace boost::xpressive::regex_constants

#endif
