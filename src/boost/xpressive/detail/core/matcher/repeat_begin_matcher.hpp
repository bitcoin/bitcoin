///////////////////////////////////////////////////////////////////////////////
// repeat_end_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_REPEAT_BEGIN_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_REPEAT_BEGIN_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>

namespace boost { namespace xpressive { namespace detail
{

    //
    // Note: here is the variable-width xpression quantifier. It always
    // matches at least once, so if the min is 0, it is the responsibility
    // of the parser to make it alternate with an epsilon matcher.
    //

    ///////////////////////////////////////////////////////////////////////////////
    // repeat_begin_matcher
    //
    struct repeat_begin_matcher
      : quant_style<quant_variable_width, unknown_width::value, false>
    {
        int mark_number_;

        repeat_begin_matcher(int mark_number)
          : mark_number_(mark_number)
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            sub_match_impl<BidiIter> &br = state.sub_match(this->mark_number_);

            unsigned int old_repeat_count = br.repeat_count_;
            bool old_zero_width = br.zero_width_;

            br.repeat_count_ = 1;
            br.zero_width_ = false;

            // "push" next onto the stack, so it can be "popped" in
            // repeat_end_matcher and used to loop back.
            if(next.BOOST_NESTED_TEMPLATE push_match<Next>(state))
            {
                return true;
            }

            br.repeat_count_ = old_repeat_count;
            br.zero_width_ = old_zero_width;

            return false;
        }
    };

}}}

#endif
