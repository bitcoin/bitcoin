/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Whitespace eater

    http://www.boost.org/

    Copyright (c) 2003 Paul Mensonides
    Copyright (c) 2001-2012 Hartmut Kaiser.
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WHITESPACE_HANDLING_HPP_INCLUDED)
#define BOOST_WHITESPACE_HANDLING_HPP_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/preprocessing_hooks.hpp>
#include <boost/wave/language_support.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace context_policies {

namespace util {
    ///////////////////////////////////////////////////////////////////////////
    //  This function returns true if the given C style comment contains at
    //  least one newline
    template <typename TokenT>
    bool ccomment_has_newline(TokenT const& token)
    {
        using namespace boost::wave;

        if (T_CCOMMENT == token_id(token) &&
            TokenT::string_type::npos != token.get_value().find_first_of("\n"))
        {
            return true;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  This function returns the number of newlines in the given C style
    //  comment
    template <typename TokenT>
    int ccomment_count_newlines(TokenT const& token)
    {
        using namespace boost::wave;
        int newlines = 0;
        if (T_CCOMMENT == token_id(token)) {
        typename TokenT::string_type const& value = token.get_value();
        typename TokenT::string_type::size_type p = value.find_first_of("\n");

            while (TokenT::string_type::npos != p) {
                ++newlines;
                p = value.find_first_of("\n", p+1);
            }
        }
        return newlines;
    }

#if BOOST_WAVE_SUPPORT_CPP0X != 0
    ///////////////////////////////////////////////////////////////////////////
    //  This function returns the number of newlines in the given C++11 style
    //  raw string
    template <typename TokenT>
    int rawstring_count_newlines(TokenT const& token)
    {
        using namespace boost::wave;
        int newlines = 0;
        if (T_RAWSTRINGLIT == token_id(token)) {
        typename TokenT::string_type const& value = token.get_value();
        typename TokenT::string_type::size_type p = value.find_first_of("\n");

            while (TokenT::string_type::npos != p) {
                ++newlines;
                p = value.find_first_of("\n", p+1);
            }
        }
        return newlines;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
template <typename TokenT>
class eat_whitespace
:   public default_preprocessing_hooks
{
public:
    eat_whitespace();

    template <typename ContextT>
    bool may_skip_whitespace(ContextT const& ctx, TokenT &token,
        bool &skipped_newline);
    template <typename ContextT>
    bool may_skip_whitespace(ContextT const& ctx, TokenT &token,
        bool preserve_comments_, bool preserve_bol_whitespace_,
        bool &skipped_newline);

protected:
    bool skip_cppcomment(boost::wave::token_id id)
    {
        return !preserve_comments && T_CPPCOMMENT == id;
    }

private:
    typedef bool state_t(TokenT &token, bool &skipped_newline);
    state_t eat_whitespace::* state;
    state_t general, newline, newline_2nd, whitespace, bol_whitespace;
    bool preserve_comments;
    bool preserve_bol_whitespace;
};

template <typename TokenT>
inline
eat_whitespace<TokenT>::eat_whitespace()
:   state(&eat_whitespace::newline), preserve_comments(false),
    preserve_bol_whitespace(false)
{
}

template <typename TokenT>
template <typename ContextT>
inline bool
eat_whitespace<TokenT>::may_skip_whitespace(ContextT const& ctx, TokenT &token,
    bool &skipped_newline)
{
    // re-initialize the preserve comments state
    preserve_comments = boost::wave::need_preserve_comments(ctx.get_language());
    return (this->*state)(token, skipped_newline);
}

template <typename TokenT>
template <typename ContextT>
inline bool
eat_whitespace<TokenT>::may_skip_whitespace(ContextT const& ctx, TokenT &token,
    bool preserve_comments_, bool preserve_bol_whitespace_,
    bool &skipped_newline)
{
    // re-initialize the preserve comments state
    preserve_comments = preserve_comments_;
    preserve_bol_whitespace = preserve_bol_whitespace_;
    return (this->*state)(token, skipped_newline);
}

template <typename TokenT>
inline bool
eat_whitespace<TokenT>::general(TokenT &token, bool &skipped_newline)
{
    using namespace boost::wave;

    token_id id = token_id(token);
    if (T_NEWLINE == id || T_CPPCOMMENT == id) {
        state = &eat_whitespace::newline;
    }
    else if (T_SPACE == id || T_SPACE2 == id || T_CCOMMENT == id) {
        state = &eat_whitespace::whitespace;

        if (util::ccomment_has_newline(token))
            skipped_newline = true;

        if ((!preserve_comments || T_CCOMMENT != id) &&
            token.get_value().size() > 1)
        {
            token.set_value(" ");   // replace with a single space
        }
    }
    else {
        state = &eat_whitespace::general;
    }
    return false;
}

template <typename TokenT>
inline bool
eat_whitespace<TokenT>::newline(TokenT &token, bool &skipped_newline)
{
    using namespace boost::wave;

    token_id id = token_id(token);
    if (T_NEWLINE == id || T_CPPCOMMENT == id) {
        skipped_newline = true;
        state = &eat_whitespace::newline_2nd;
        return T_NEWLINE == id || skip_cppcomment(id);
    }

    if (T_SPACE != id && T_SPACE2 != id && T_CCOMMENT != id)
        return general(token, skipped_newline);

    if (T_CCOMMENT == id) {
        if (util::ccomment_has_newline(token)) {
            skipped_newline = true;
            state = &eat_whitespace::newline_2nd;
        }
        if (preserve_comments) {
            state = &eat_whitespace::general;
            return false;
        }
        return true;
    }

    if (preserve_bol_whitespace) {
        state = &eat_whitespace::bol_whitespace;
        return false;
    }

    return true;
}

template <typename TokenT>
inline bool
eat_whitespace<TokenT>::newline_2nd(TokenT &token, bool &skipped_newline)
{
    using namespace boost::wave;

    token_id id = token_id(token);
    if (T_SPACE == id || T_SPACE2 == id) {
        if (preserve_bol_whitespace) {
            state = &eat_whitespace::bol_whitespace;
            return false;
        }
        return true;
    }

    if (T_CCOMMENT == id) {
        if (util::ccomment_has_newline(token))
            skipped_newline = true;

        if (preserve_comments) {
            state = &eat_whitespace::general;
            return false;
        }
        return  true;
    }

    if (T_NEWLINE != id && T_CPPCOMMENT != id)
        return general(token, skipped_newline);

    skipped_newline = true;
    return T_NEWLINE == id || skip_cppcomment(id);
}

template <typename TokenT>
inline bool
eat_whitespace<TokenT>::bol_whitespace(TokenT &token, bool &skipped_newline)
{
    using namespace boost::wave;

    token_id id = token_id(token);
    if (T_SPACE == id || T_SPACE2 == id)
        return !preserve_bol_whitespace;

    return general(token, skipped_newline);
}

template <typename TokenT>
inline bool
eat_whitespace<TokenT>::whitespace(TokenT &token, bool &skipped_newline)
{
    using namespace boost::wave;

    token_id id = token_id(token);
    if (T_SPACE != id && T_SPACE2 != id &&
        T_CCOMMENT != id && T_CPPCOMMENT != id)
    {
        return general(token, skipped_newline);
    }

    if (T_CCOMMENT == id) {
        if (util::ccomment_has_newline(token))
            skipped_newline = true;
        return !preserve_comments;
    }

    return T_SPACE == id || T_SPACE2 == id || skip_cppcomment(id);
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace context_policies
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_WHITESPACE_HANDLING_HPP_INCLUDED)

