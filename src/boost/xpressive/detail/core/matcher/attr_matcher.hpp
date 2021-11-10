///////////////////////////////////////////////////////////////////////////////
// attr_matcher.hpp
//
//  Copyright 2008 Eric Niebler.
//  Copyright 2008 David Jenkins.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ATTR_MATCHER_HPP_EAN_06_09_2007
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ATTR_MATCHER_HPP_EAN_06_09_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/utility/symbols.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // char_translate
    //
    template<typename Traits, bool ICase>
    struct char_translate
    {
        typedef typename Traits::char_type char_type;
        Traits const &traits_;

        explicit char_translate(Traits const &tr)
          : traits_(tr)
        {}

        char_type operator ()(char_type ch1) const
        {
            return this->traits_.translate(ch1);
        }
    private:
        char_translate &operator =(char_translate const &);
    };

    ///////////////////////////////////////////////////////////////////////////////
    // char_translate
    //
    template<typename Traits>
    struct char_translate<Traits, true>
    {
        typedef typename Traits::char_type char_type;
        Traits const &traits_;

        explicit char_translate(Traits const &tr)
          : traits_(tr)
        {}

        char_type operator ()(char_type ch1) const
        {
            return this->traits_.translate_nocase(ch1);
        }
    private:
        char_translate &operator =(char_translate const &);
    };

    ///////////////////////////////////////////////////////////////////////////////
    // attr_matcher
    //  Note: the Matcher is a std::map
    template<typename Matcher, typename Traits, typename ICase>
    struct attr_matcher
      : quant_style<quant_none, 0, false>
    {
        typedef typename Matcher::value_type::second_type const* result_type;

        attr_matcher(int slot, Matcher const &matcher, Traits const& tr)
          : slot_(slot-1)
        {
            char_translate<Traits, ICase::value> trans(tr);
            this->sym_.load(matcher, trans);
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            BidiIter tmp = state.cur_;
            char_translate<Traits, ICase::value> trans(traits_cast<Traits>(state));
            result_type const &result = this->sym_(state.cur_, state.end_, trans);
            if(result)
            {
                void const *old_slot = state.attr_context_.attr_slots_[this->slot_];
                state.attr_context_.attr_slots_[this->slot_] = &*result;
                if(next.match(state))
                {
                    return true;
                }
                state.attr_context_.attr_slots_[this->slot_] = old_slot;
            }
            state.cur_ = tmp;
            return false;
        }

        int slot_;
        boost::xpressive::detail::symbols<Matcher> sym_;
    };

}}}

#endif
