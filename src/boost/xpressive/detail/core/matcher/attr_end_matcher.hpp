///////////////////////////////////////////////////////////////////////////////
// attr_end_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ATTR_END_MATCHER_HPP_EAN_06_09_2007
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ATTR_END_MATCHER_HPP_EAN_06_09_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // attr_end_matcher
    //
    struct attr_end_matcher
      : quant_style<quant_none, 0, false>
    {
        template<typename BidiIter, typename Next>
        static bool match(match_state<BidiIter> &state, Next const &next)
        {
            attr_context old_attr_context = state.attr_context_;
            state.attr_context_ = *old_attr_context.prev_attr_context_;

            if(next.match(state))
            {
                return true;
            }

            state.attr_context_ = old_attr_context;
            return false;
        }
    };

}}}

#endif
