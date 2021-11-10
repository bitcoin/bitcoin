///////////////////////////////////////////////////////////////////////////////
// logical_newline_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_LOGICAL_NEWLINE_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_LOGICAL_NEWLINE_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>

namespace boost { namespace xpressive { namespace detail
{

    //////////////////////////////////////////////////////////////////////////
    // logical_newline_matcher
    //
    template<typename Traits>
    struct logical_newline_matcher
      : quant_style_variable_width
    {
        typedef typename Traits::char_type char_type;
        typedef typename Traits::char_class_type char_class_type;

        logical_newline_matcher(Traits const &tr)
          : newline_(lookup_classname(tr, "newline"))
          , nl_(tr.widen('\n'))
          , cr_(tr.widen('\r'))
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            if(state.eos())
            {
                return false;
            }

            char_type ch = *state.cur_;
            if(traits_cast<Traits>(state).isctype(ch, this->newline_))
            {
                ++state.cur_;
                if(this->cr_ == ch && !state.eos() && this->nl_ == *state.cur_)
                {
                    ++state.cur_;
                    if(next.match(state))
                    {
                        return true;
                    }
                    --state.cur_;
                }
                else if(next.match(state))
                {
                    return true;
                }

                --state.cur_;
            }
            return false;
        }

        char_class_type newline() const
        {
            return this->newline_;
        }

    private:
        char_class_type newline_;
        char_type nl_, cr_;
    };

}}}

#endif
