///////////////////////////////////////////////////////////////////////////////
// end_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_END_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_END_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/core/sub_match_impl.hpp>
#include <boost/xpressive/detail/core/flow_control.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // end_matcher
    //
    struct end_matcher
      : quant_style_assertion
    {
        template<typename BidiIter, typename Next>
        static bool match(match_state<BidiIter> &state, Next const &)
        {
            BidiIter const tmp = state.cur_;
            sub_match_impl<BidiIter> &s0 = state.sub_match(0);
            BOOST_ASSERT(!s0.matched);

            // SPECIAL: if there is a match context on the context stack, then
            // this pattern has been nested within another. pop that context and
            // continue executing.
            if(0 != state.context_.prev_context_)
            {
                if(!pop_context_match(state))
                {
                    return false;
                }

                // record the end of sub-match zero
                s0.first = s0.begin_;
                s0.second = tmp;
                s0.matched = true;

                return true;
            }
            else if((state.flags_.match_all_ && !state.eos()) ||
                    (state.flags_.match_not_null_ && state.cur_ == s0.begin_))
            {
                return false;
            }

            // record the end of sub-match zero
            s0.first = s0.begin_;
            s0.second = tmp;
            s0.matched = true;

            // Now execute any actions that have been queued
            for(actionable const *actor = state.action_list_.next; 0 != actor; actor = actor->next)
            {
                actor->execute(state.action_args_);
            }

            return true;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // independent_end_matcher
    //
    struct independent_end_matcher
      : quant_style_assertion
    {
        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &) const
        {
            // Now execute any actions that have been queued
            for(actionable const *actor = state.action_list_.next; 0 != actor; actor = actor->next)
            {
                actor->execute(state.action_args_);
            }

            return true;
        }
    };

}}}

#endif
