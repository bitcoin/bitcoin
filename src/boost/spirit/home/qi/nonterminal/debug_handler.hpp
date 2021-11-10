/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_DEBUG_HANDLER_DECEMBER_05_2008_0734PM)
#define BOOST_SPIRIT_DEBUG_HANDLER_DECEMBER_05_2008_0734PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/spirit/home/qi/nonterminal/debug_handler_state.hpp>
#include <boost/spirit/home/qi/detail/expectation_failure.hpp>
#include <boost/function.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/out.hpp>
#include <iostream>

namespace boost { namespace spirit { namespace qi
{
    template <
        typename Iterator, typename Context
      , typename Skipper, typename F>
    struct debug_handler
    {
        typedef function<
            bool(Iterator& first, Iterator const& last
              , Context& context
              , Skipper const& skipper
            )>
        function_type;

        debug_handler(
            function_type subject_
          , F f_
          , std::string const& rule_name_)
          : subject(subject_)
          , f(f_)
          , rule_name(rule_name_)
        {
        }

        bool operator()(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper) const
        {
            f(first, last, context, pre_parse, rule_name);
            try // subject might throw an exception
            {
                if (subject(first, last, context, skipper))
                {
                    f(first, last, context, successful_parse, rule_name);
                    return true;
                }
                f(first, last, context, failed_parse, rule_name);
            }
            catch (expectation_failure<Iterator> const& e)
            {
                f(first, last, context, failed_parse, rule_name);
                boost::throw_exception(e);
            }
            return false;
        }

        function_type subject;
        F f;
        std::string rule_name;
    };

    template <typename Iterator
      , typename T1, typename T2, typename T3, typename T4, typename F>
    void debug(rule<Iterator, T1, T2, T3, T4>& r, F f)
    {
        typedef rule<Iterator, T1, T2, T3, T4> rule_type;

        typedef
            debug_handler<
                Iterator
              , typename rule_type::context_type
              , typename rule_type::skipper_type
              , F>
        debug_handler;
        r.f = debug_handler(r.f, f, r.name());
    }

    struct simple_trace;

    namespace detail
    {
        // This class provides an extra level of indirection through a
        // template to produce the simple_trace type. This way, the use
        // of simple_trace below is hidden behind a dependent type, so
        // that compilers eagerly type-checking template definitions
        // won't complain that simple_trace is incomplete.
        template<typename T>
        struct get_simple_trace
        {
            typedef simple_trace type;
        };
    }

    template <typename Iterator
      , typename T1, typename T2, typename T3, typename T4>
    void debug(rule<Iterator, T1, T2, T3, T4>& r)
    {
        typedef rule<Iterator, T1, T2, T3, T4> rule_type;

        typedef
            debug_handler<
                Iterator
              , typename rule_type::context_type
              , typename rule_type::skipper_type
              , simple_trace>
        debug_handler;

        typedef typename qi::detail::get_simple_trace<Iterator>::type trace;
        r.f = debug_handler(r.f, trace(), r.name());
    }

}}}

///////////////////////////////////////////////////////////////////////////////
//  Utility macro for easy enabling of rule and grammar debugging
#if !defined(BOOST_SPIRIT_DEBUG_NODE)
  #if defined(BOOST_SPIRIT_DEBUG) || defined(BOOST_SPIRIT_QI_DEBUG)
    #define BOOST_SPIRIT_DEBUG_NODE(r)  r.name(#r); debug(r)
  #else
    #define BOOST_SPIRIT_DEBUG_NODE(r)  r.name(#r)
  #endif
#endif

#define BOOST_SPIRIT_DEBUG_NODE_A(r, _, name)                                   \
    BOOST_SPIRIT_DEBUG_NODE(name);                                              \
    /***/

#define BOOST_SPIRIT_DEBUG_NODES(seq)                                           \
    BOOST_PP_SEQ_FOR_EACH(BOOST_SPIRIT_DEBUG_NODE_A, _, seq)                    \
    /***/

#endif
