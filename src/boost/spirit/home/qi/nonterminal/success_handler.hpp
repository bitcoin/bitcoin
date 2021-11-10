/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_SUCCESS_HANDLER_FEBRUARY_25_2011_1051AM)
#define BOOST_SPIRIT_SUCCESS_HANDLER_FEBRUARY_25_2011_1051AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/function.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <
        typename Iterator, typename Context
      , typename Skipper, typename F
    >
    struct success_handler
    {
        typedef function<
            bool(Iterator& first, Iterator const& last
              , Context& context
              , Skipper const& skipper
            )>
        function_type;

        success_handler(function_type subject_, F f_)
          : subject(subject_)
          , f(f_)
        {
        }

        bool operator()(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper) const
        {
            Iterator i = first;
            bool r = subject(i, last, context, skipper);
            if (r)
            {
                typedef
                    fusion::vector<
                        Iterator&
                      , Iterator const&
                      , Iterator const&>
                params;
                skip_over(first, last, skipper);
                params args(first, last, i);
                f(args, context);

                first = i;
            }
            return r;
        }

        function_type subject;
        F f;
    };

    template <
        typename Iterator, typename T0, typename T1, typename T2
      , typename F>
    void on_success(rule<Iterator, T0, T1, T2>& r, F f)
    {
        typedef rule<Iterator, T0, T1, T2> rule_type;

        typedef
            success_handler<
                Iterator
              , typename rule_type::context_type
              , typename rule_type::skipper_type
              , F>
        success_handler;
        r.f = success_handler(r.f, f);
    }
}}}

#endif
