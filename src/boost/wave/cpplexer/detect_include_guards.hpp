/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    State machine detecting include guards in an included file.
    This detects two forms of include guards:

        #ifndef INCLUDE_GUARD_MACRO
        #define INCLUDE_GUARD_MACRO
        ...
        #endif

    or

        if !defined(INCLUDE_GUARD_MACRO)
        #define INCLUDE_GUARD_MACRO
        ...
        #endif

    note, that the parenthesis are optional (i.e. !defined INCLUDE_GUARD_MACRO
    will work as well). The code allows for any whitespace, newline and single
    '#' tokens before the #if/#ifndef and after the final #endif.

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_DETECT_INCLUDE_GUARDS_HK060304_INCLUDED)
#define BOOST_DETECT_INCLUDE_GUARDS_HK060304_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {

template <typename Token>
class include_guards
{
public:
    include_guards()
    :   state(&include_guards::state_0), detected_guards(false),
        current_state(true), if_depth(0)
    {}

    Token& detect_guard(Token& t)
        { return current_state ? (this->*state)(t) : t; }
    bool detected(std::string& guard_name_) const
    {
        if (detected_guards) {
            guard_name_ = guard_name.c_str();
            return true;
        }
        return false;
    }

private:
    typedef Token& state_type(Token& t);
    state_type include_guards::* state;

    bool detected_guards;
    bool current_state;
    typename Token::string_type guard_name;
    int if_depth;

    state_type state_0, state_1, state_2, state_3, state_4, state_5;
    state_type state_1a, state_1b, state_1c, state_1d, state_1e;

    bool is_skippable(token_id id) const
    {
        return (T_POUND == BASE_TOKEN(id) ||
                IS_CATEGORY(id, WhiteSpaceTokenType) ||
                IS_CATEGORY(id, EOLTokenType));
    }
};

///////////////////////////////////////////////////////////////////////////////
//  state 0: beginning of a file, tries to recognize #ifndef or #if tokens
template <typename Token>
inline Token&
include_guards<Token>::state_0(Token& t)
{
    token_id id = token_id(t);
    if (T_PP_IFNDEF == id)
        state = &include_guards::state_1;
    else if (T_PP_IF == id)
        state = &include_guards::state_1a;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1: found #ifndef, looking for T_IDENTIFIER
template <typename Token>
inline Token&
include_guards<Token>::state_1(Token& t)
{
    token_id id = token_id(t);
    if (T_IDENTIFIER == id) {
        guard_name = t.get_value();
        state = &include_guards::state_2;
    }
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1a: found T_PP_IF, looking for T_NOT ("!")
template <typename Token>
inline Token&
include_guards<Token>::state_1a(Token& t)
{
    token_id id = token_id(t);
    if (T_NOT == BASE_TOKEN(id))
        state = &include_guards::state_1b;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1b: found T_NOT, looking for 'defined'
template <typename Token>
inline Token&
include_guards<Token>::state_1b(Token& t)
{
    token_id id = token_id(t);
    if (T_IDENTIFIER == id && t.get_value() == "defined")
        state = &include_guards::state_1c;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1c: found 'defined', looking for (optional) T_LEFTPAREN
template <typename Token>
inline Token&
include_guards<Token>::state_1c(Token& t)
{
    token_id id = token_id(t);
    if (T_LEFTPAREN == id)
        state = &include_guards::state_1d;
    else if (T_IDENTIFIER == id) {
        guard_name = t.get_value();
        state = &include_guards::state_2;
    }
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1d: found T_LEFTPAREN, looking for T_IDENTIFIER guard
template <typename Token>
inline Token&
include_guards<Token>::state_1d(Token& t)
{
    token_id id = token_id(t);
    if (T_IDENTIFIER == id) {
        guard_name = t.get_value();
        state = &include_guards::state_1e;
    }
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 1e: found T_IDENTIFIER guard, looking for T_RIGHTPAREN
template <typename Token>
inline Token&
include_guards<Token>::state_1e(Token& t)
{
    token_id id = token_id(t);
    if (T_RIGHTPAREN == id)
        state = &include_guards::state_2;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 2: found T_IDENTIFIER, looking for #define
template <typename Token>
inline Token&
include_guards<Token>::state_2(Token& t)
{
    token_id id = token_id(t);
    if (T_PP_DEFINE == id)
        state = &include_guards::state_3;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 3: found #define, looking for T_IDENTIFIER as recognized by state 1
template <typename Token>
inline Token&
include_guards<Token>::state_3(Token& t)
{
    token_id id = token_id(t);
    if (T_IDENTIFIER == id && t.get_value() == guard_name)
        state = &include_guards::state_4;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 4: found guard T_IDENTIFIER, looking for #endif
template <typename Token>
inline Token&
include_guards<Token>::state_4(Token& t)
{
    token_id id = token_id(t);
    if (T_PP_IF == id || T_PP_IFDEF == id || T_PP_IFNDEF == id)
        ++if_depth;
    else if (T_PP_ENDIF == id) {
        if (if_depth > 0)
            --if_depth;
        else
            state = &include_guards::state_5;
    }
    return t;
}

///////////////////////////////////////////////////////////////////////////////
//  state 5: found final #endif, looking for T_EOF
template <typename Token>
inline Token&
include_guards<Token>::state_5(Token& t)
{
    token_id id = token_id(t);
    if (T_EOF == id)
        detected_guards = current_state;
    else if (!is_skippable(id))
        current_state = false;
    return t;
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !BOOST_DETECT_INCLUDE_GUARDS_HK060304_INCLUDED
