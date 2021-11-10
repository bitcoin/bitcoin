/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ERROR_HANDLER_APRIL_29_2007_1042PM)
#define BOOST_SPIRIT_ERROR_HANDLER_APRIL_29_2007_1042PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/operator/expect.hpp>
#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/spirit/home/support/multi_pass_wrapper.hpp>
#include <boost/function.hpp>
#include <boost/assert.hpp>

namespace boost { namespace spirit { namespace qi
{
    enum error_handler_result
    {
        fail
      , retry
      , accept
      , rethrow
    };

    namespace detail
    {
        // Helper template allowing to manage the inhibit clear queue flag in
        // a multi_pass iterator. This is the usual specialization used for
        // anything but a multi_pass iterator.
        template <typename Iterator, bool active>
        struct reset_on_exit
        {
            reset_on_exit(Iterator&) {}
        };

        // For 'retry' or 'fail' error handlers we need to inhibit the flushing 
        // of the internal multi_pass buffers which otherwise might happen at 
        // deterministic expectation points inside the encapsulated right hand 
        // side of rule.
        template <typename Iterator>
        struct reset_on_exit<Iterator, true>
        {
            reset_on_exit(Iterator& it)
              : it_(it)
              , inhibit_clear_queue_(spirit::traits::inhibit_clear_queue(it)) 
            {
                spirit::traits::inhibit_clear_queue(it_, true);
            }

            ~reset_on_exit()
            {
                // reset inhibit flag in multi_pass on exit
                spirit::traits::inhibit_clear_queue(it_, inhibit_clear_queue_);
            }

            Iterator& it_;
            bool inhibit_clear_queue_;
        };
    }

    template <
        typename Iterator, typename Context
      , typename Skipper, typename F, error_handler_result action
    >
    struct error_handler
    {
        typedef function<
            bool(Iterator& first, Iterator const& last
              , Context& context
              , Skipper const& skipper
            )>
        function_type;

        error_handler(function_type subject_, F f_)
          : subject(subject_)
          , f(f_)
        {
        }

        bool operator()(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper) const
        {
            typedef qi::detail::reset_on_exit<Iterator
              , traits::is_multi_pass<Iterator>::value && 
                  (action == retry || action == fail)> on_exit_type;

            on_exit_type on_exit(first);
            for(;;)
            {
                try
                {
                    Iterator i = first;
                    bool r = subject(i, last, context, skipper);
                    if (r)
                        first = i;
                    return r;
                }
                catch (expectation_failure<Iterator> const& x)
                {
                    typedef
                        fusion::vector<
                            Iterator&
                          , Iterator const&
                          , Iterator const&
                          , info const&>
                    params;
                    error_handler_result r = action;
                    params args(first, last, x.first, x.what_);
                    f(args, context, r);

                    // The assertions below will fire if you are using a
                    // multi_pass as the underlying iterator, one of your error
                    // handlers forced its guarded rule to 'fail' or 'retry',
                    // and the error handler has not been instantiated using
                    // either 'fail' or 'retry' in the first place. Please see 
                    // the multi_pass docs for more information.
                    switch (r)
                    {
                        case fail: 
                            BOOST_ASSERT(
                                !traits::is_multi_pass<Iterator>::value ||
                                    action == retry || action == fail);
                            return false;
                        case retry: 
                            BOOST_ASSERT(
                                !traits::is_multi_pass<Iterator>::value ||
                                    action == retry || action == fail);
                            continue;
                        case accept: return true;
                        case rethrow: boost::throw_exception(x);
                    }
                }
            }
            return false;
        }

        function_type subject;
        F f;
    };

    template <
        error_handler_result action
      , typename Iterator, typename T0, typename T1, typename T2
      , typename F>
    void on_error(rule<Iterator, T0, T1, T2>& r, F f)
    {
        typedef rule<Iterator, T0, T1, T2> rule_type;

        typedef
            error_handler<
                Iterator
              , typename rule_type::context_type
              , typename rule_type::skipper_type
              , F
              , action>
        error_handler;
        r.f = error_handler(r.f, f);
    }

    // Error handling support when <action> is not
    // specified. We will default to <fail>.
    template <typename Iterator, typename T0, typename T1
      , typename T2, typename F>
    void on_error(rule<Iterator, T0, T1, T2>& r, F f)
    {
        on_error<fail>(r, f);
    }
}}}

#endif
