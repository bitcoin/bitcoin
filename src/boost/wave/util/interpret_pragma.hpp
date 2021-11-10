/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_INTERPRET_PRAGMA_HPP_B1F2315E_C5CE_4ED1_A343_0EF548B7942A_INCLUDED)
#define BOOST_INTERPRET_PRAGMA_HPP_B1F2315E_C5CE_4ED1_A343_0EF548B7942A_INCLUDED

#include <string>
#include <list>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>
#include <boost/spirit/include/classic_confix.hpp>

#include <boost/wave/wave_config.hpp>

#include <boost/wave/util/pattern_parser.hpp>
#include <boost/wave/util/macro_helpers.hpp>

#include <boost/wave/token_ids.hpp>
#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/cpp_iteration_context.hpp>
#include <boost/wave/language_support.hpp>

#if !defined(spirit_append_actor)
#define spirit_append_actor(actor) boost::spirit::classic::push_back_a(actor)
#define spirit_assign_actor(actor) boost::spirit::classic::assign_a(actor)
#endif // !defined(spirit_append_actor)

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

///////////////////////////////////////////////////////////////////////////////
//
//  The function interpret_pragma interprets the given token sequence as the
//  body of a #pragma directive (or parameter to the _Pragma operator) and
//  executes the actions associated with recognized Wave specific options.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ContextT, typename IteratorT, typename ContainerT>
inline bool
interpret_pragma(ContextT &ctx, typename ContextT::token_type const &act_token,
    IteratorT it, IteratorT const &end, ContainerT &pending)
{
    typedef typename ContextT::token_type token_type;
    typedef typename token_type::string_type string_type;

    using namespace cpplexer;
    if (T_IDENTIFIER == token_id(*it)) {
    // check for pragma wave ...
        if ((*it).get_value() == BOOST_WAVE_PRAGMA_KEYWORD)
        {
        //  this is a wave specific option, it should have the form:
        //
        //      #pragma command option(value)
        //
        //  where
        //      'command' is the value of the preprocessor constant
        //                BOOST_WAVE_PRAGMA_KEYWORD (defaults to "wave") and
        //      '(value)' is required only for some pragma directives (this is
        //                optional)
        //
        //  All recognized #pragma operators are forwarded to the supplied
        //  preprocessing hook.
            using namespace boost::spirit::classic;
            token_type option;
            ContainerT values;

            if (!parse (++it, end,
                            (   ch_p(T_IDENTIFIER)
                                [
                                    spirit_assign_actor(option)
                                ]
                            |   pattern_p(KeywordTokenType,
                                    TokenTypeMask|PPTokenFlag)
                                [
                                    spirit_assign_actor(option)
                                ]
                            |   pattern_p(OperatorTokenType|AltExtTokenType,
                                    ExtTokenTypeMask|PPTokenFlag)   // and, bit_and etc.
                                [
                                    spirit_assign_actor(option)
                                ]
                            |   pattern_p(BoolLiteralTokenType,
                                    TokenTypeMask|PPTokenFlag)
                                [
                                    spirit_assign_actor(option)
                                ]
                            )
                        >> !comment_nest_p(
                                ch_p(T_LEFTPAREN),
                                ch_p(T_RIGHTPAREN)
                            )[spirit_assign_actor(values)],
                    pattern_p(WhiteSpaceTokenType, TokenTypeMask|PPTokenFlag)).hit)
            {
                typename ContextT::string_type msg(
                    impl::as_string<string_type>(it, end));
                BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                    ill_formed_pragma_option,
                    msg.c_str(), act_token.get_position());
                return false;
            }

            // remove the falsely matched surrounding parenthesis's
            if (values.size() >= 2) {
                BOOST_ASSERT(T_LEFTPAREN == values.front() && T_RIGHTPAREN == values.back());
                values.erase(values.begin());
                typename ContainerT::reverse_iterator rit = values.rbegin();
                values.erase((++rit).base());
            }

            // decode the option (call the context_policy hook)
            if (!ctx.get_hooks().interpret_pragma(
                  ctx.derived(), pending, option, values, act_token))
            {
                // unknown #pragma option
                string_type option_str((*it).get_value());

                option_str += option.get_value();
                if (values.size() > 0) {
                    option_str += "(";
                    option_str += impl::as_string(values);
                    option_str += ")";
                }
                BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                    ill_formed_pragma_option,
                    option_str.c_str(), act_token.get_position());
                return false;
            }
            return true;
        }
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
        else if ((*it).get_value() == "once") {
            // #pragma once
            return ctx.add_pragma_once_header(act_token, ctx.get_current_filename());
        }
#endif
#if BOOST_WAVE_SUPPORT_PRAGMA_MESSAGE != 0
        else if ((*it).get_value() == "message") {
            // #pragma message(...) or #pragma message ...
            using namespace boost::spirit::classic;
            ContainerT values;

            if (!parse (++it, end,
                            (   (   ch_p(T_LEFTPAREN)
                                >>  lexeme_d[
                                        *(anychar_p[spirit_append_actor(values)] - ch_p(T_RIGHTPAREN))
                                    ]
                                >>  ch_p(T_RIGHTPAREN)
                                )
                            |   lexeme_d[
                                    *(anychar_p[spirit_append_actor(values)] - ch_p(T_NEWLINE))
                                ]
                            ),
                            pattern_p(WhiteSpaceTokenType, TokenTypeMask|PPTokenFlag)
                       ).hit
               )
            {
                typename ContextT::string_type msg(
                    impl::as_string<string_type>(it, end));
                BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                    ill_formed_pragma_message,
                    msg.c_str(), act_token.get_position());
                return false;
            }

            // remove the falsely matched closing parenthesis/newline
            if (values.size() > 0) {
                BOOST_ASSERT(T_RIGHTPAREN == values.back() || T_NEWLINE == values.back());
                typename ContainerT::reverse_iterator rit = values.rbegin();
                values.erase((++rit).base());
            }

            // output the message itself
            typename ContextT::string_type msg(impl::as_string(values));
            BOOST_WAVE_THROW_CTX(ctx, preprocess_exception,
                pragma_message_directive,
                msg.c_str(), act_token.get_position());
            return false;
        }
#endif
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_INTERPRET_PRAGMA_HPP_B1F2315E_C5CE_4ED1_A343_0EF548B7942A_INCLUDED)
