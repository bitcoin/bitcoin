///////////////////////////////////////////////////////////////////////////////
// mark_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_MARK_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_MARK_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/utility/traits_utils.hpp>

namespace boost { namespace xpressive { namespace detail
{

    // TODO: the mark matcher is acually a fixed-width matcher, but the width is
    // not known until pattern match time.

    ///////////////////////////////////////////////////////////////////////////////
    // mark_matcher
    //
    template<typename Traits, typename ICase>
    struct mark_matcher
      : quant_style_variable_width
    {
        typedef ICase icase_type;
        int mark_number_;

        mark_matcher(int mark_number, Traits const &)
          : mark_number_(mark_number)
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            BOOST_ASSERT(this->mark_number_ < static_cast<int>(state.mark_count_));
            sub_match_impl<BidiIter> const &br = state.sub_match(this->mark_number_);

            if(!br.matched)
            {
                return false;
            }

            BidiIter const tmp = state.cur_;
            for(BidiIter begin = br.first, end = br.second; begin != end; ++begin, ++state.cur_)
            {
                if(state.eos()
                    || detail::translate(*state.cur_, traits_cast<Traits>(state), icase_type())
                    != detail::translate(*begin, traits_cast<Traits>(state), icase_type()))
                {
                    state.cur_ = tmp;
                    return false;
                }
            }

            if(next.match(state))
            {
                return true;
            }

            state.cur_ = tmp;
            return false;
        }
    };

}}}

#endif
