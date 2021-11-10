// tokeniser_state.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_RE_TOKENISER_STATE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_RE_TOKENISER_STATE_HPP

#include "../../consts.hpp"
#include <locale>
#include "../../size_t.hpp"
#include <stack>

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT>
struct basic_re_tokeniser_state
{
    const CharT * const _start;
    const CharT * const _end;
    const CharT *_curr;
    regex_flags _flags;
    std::stack<regex_flags> _flags_stack;
    std::locale _locale;
    long _paren_count;
    bool _in_string;
    bool _seen_BOL_assertion;
    bool _seen_EOL_assertion;

    basic_re_tokeniser_state (const CharT *start_, const CharT * const end_,
        const regex_flags flags_, const std::locale locale_) :
        _start (start_),
        _end (end_),
        _curr (start_),
        _flags (flags_),
        _locale (locale_),
        _paren_count (0),
        _in_string (false),
        _seen_BOL_assertion (false),
        _seen_EOL_assertion (false)
    {
    }

    // prevent VC++ 7.1 warning:
    const basic_re_tokeniser_state &operator =
        (const basic_re_tokeniser_state &rhs_)
    {
        _start = rhs_._start;
        _end = rhs_._end;
        _curr = rhs_._curr;
        _flags = rhs_._flags;
        _locale = rhs_._locale;
        _paren_count = rhs_._paren_count;
        _in_string = rhs_._in_string;
        _seen_BOL_assertion = rhs_._seen_BOL_assertion;
        _seen_EOL_assertion = rhs_._seen_EOL_assertion;
        return this;
    }

    inline bool next (CharT &ch_)
    {
        if (_curr >= _end)
        {
            ch_ = 0;
            return true;
        }
        else
        {
            ch_ = *_curr;
            increment ();
            return false;
        }
    }

    inline void increment ()
    {
        ++_curr;
    }

    inline std::size_t index ()
    {
        return _curr - _start;
    }

    inline bool eos ()
    {
        return _curr >= _end;
    }
};
}
}
}

#endif
