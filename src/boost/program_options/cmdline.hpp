// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CMDLINE_HPP_VP_2004_03_13
#define BOOST_CMDLINE_HPP_VP_2004_03_13

namespace boost { namespace program_options { namespace command_line_style {
    /** Various possible styles of options.
        
    There are "long" options, which start with "--" and "short",
    which start with either "-" or "/". Both kinds can be allowed or
    disallowed, see allow_long and allow_short. The allowed character
    for short options is also configurable.

    Option's value can be specified in the same token as name
    ("--foo=bar"), or in the next token.

    It's possible to introduce long options by the same character as
    short options, see allow_long_disguise.

    Finally, guessing (specifying only prefix of option) and case
    insensitive processing are supported.
    */
    enum style_t {
        /// Allow "--long_name" style
        allow_long = 1,
        /// Allow "-<single character" style
        allow_short = allow_long << 1,
        /// Allow "-" in short options
        allow_dash_for_short = allow_short << 1,
        /// Allow "/" in short options
        allow_slash_for_short = allow_dash_for_short << 1,
        /** Allow option parameter in the same token
            for long option, like in
            @verbatim
            --foo=10
            @endverbatim
        */
        long_allow_adjacent = allow_slash_for_short << 1,
        /** Allow option parameter in the next token for
            long options. */
        long_allow_next = long_allow_adjacent << 1,
        /** Allow option parameter in the same token for
            short options. */
        short_allow_adjacent = long_allow_next << 1,
        /** Allow option parameter in the next token for
            short options. */
        short_allow_next = short_allow_adjacent << 1,
        /** Allow to merge several short options together,
            so that "-s -k" become "-sk". All of the options
            but last should accept no parameter. For example, if
            "-s" accept a parameter, then "k" will be taken as
            parameter, not another short option. 
            Dos-style short options cannot be sticky.
        */
        allow_sticky = short_allow_next << 1,
        /** Allow abbreviated spellings for long options,
            if they unambiguously identify long option. 
            No long option name should be prefix of other 
            long option name if guessing is in effect.
        */
        allow_guessing = allow_sticky << 1,
        /** Ignore the difference in case for long options.
        */            
        long_case_insensitive = allow_guessing << 1,        
        /** Ignore the difference in case for short options.
        */            
        short_case_insensitive = long_case_insensitive << 1,
        /** Ignore the difference in case for all options.
        */        
        case_insensitive = (long_case_insensitive | short_case_insensitive),        
        /** Allow long options with single option starting character,
            e.g <tt>-foo=10</tt>
        */
        allow_long_disguise = short_case_insensitive << 1,
        /** The more-or-less traditional unix style. */
        unix_style = (allow_short | short_allow_adjacent | short_allow_next
                      | allow_long | long_allow_adjacent | long_allow_next
                      | allow_sticky | allow_guessing 
                      | allow_dash_for_short),
        /** The default style. */
        default_style = unix_style
    };
}}}


#endif

