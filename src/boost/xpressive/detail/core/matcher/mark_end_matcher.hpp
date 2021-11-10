///////////////////////////////////////////////////////////////////////////////
// mark_end_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_MARK_END_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_MARK_END_MATCHER_HPP_EAN_10_04_2005

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
    // mark_end_matcher
    //
    struct mark_end_matcher
      : quant_style<quant_none, 0, false>
    {
        int mark_number_;

        mark_end_matcher(int mark_number)
          : mark_number_(mark_number)
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            sub_match_impl<BidiIter> &br = state.sub_match(this->mark_number_);

            BidiIter old_first = br.first;
            BidiIter old_second = br.second;
            bool old_matched = br.matched;

            br.first = br.begin_;
            br.second = state.cur_;
            br.matched = true;

            if(next.match(state))
            {
                return true;
            }

            br.first = old_first;
            br.second = old_second;
            br.matched = old_matched;

            return false;
        }
    };

}}}

#endif
