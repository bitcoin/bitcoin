///////////////////////////////////////////////////////////////////////////////
// string_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_STRING_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_STRING_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <string>
#include <boost/mpl/bool.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/utility/algorithm.hpp>
#include <boost/xpressive/detail/utility/traits_utils.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // string_matcher
    //
    template<typename Traits, typename ICase>
    struct string_matcher
      : quant_style_fixed_unknown_width
    {
        typedef typename Traits::char_type char_type;
        typedef typename Traits::string_type string_type;
        typedef ICase icase_type;
        string_type str_;
        char_type const *end_;

        string_matcher(string_type const &str, Traits const &tr)
          : str_(str)
          , end_()
        {
            typename range_iterator<string_type>::type cur = boost::begin(this->str_);
            typename range_iterator<string_type>::type end = boost::end(this->str_);
            for(; cur != end; ++cur)
            {
                *cur = detail::translate(*cur, tr, icase_type());
            }
            this->end_ = detail::data_end(str_);
        }

        string_matcher(string_matcher<Traits, ICase> const &that)
          : str_(that.str_)
          , end_(detail::data_end(str_))
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            BidiIter const tmp = state.cur_;
            char_type const *begin = detail::data_begin(this->str_);
            for(; begin != this->end_; ++begin, ++state.cur_)
            {
                if(state.eos() ||
                    (detail::translate(*state.cur_, traits_cast<Traits>(state), icase_type()) != *begin))
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

        detail::width get_width() const
        {
            return boost::size(this->str_);
        }
    };

}}}

#endif
