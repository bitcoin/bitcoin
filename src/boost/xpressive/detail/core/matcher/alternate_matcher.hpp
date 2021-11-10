///////////////////////////////////////////////////////////////////////////////
// alternate_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ALTERNATE_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ALTERNATE_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/version.hpp>
#if BOOST_VERSION <= 103200
// WORKAROUND for Fusion bug in Boost 1.32
namespace boost { namespace fusion
{
    namespace detail { struct iterator_root; }
    using detail::iterator_root;
}}
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#include <boost/xpressive/detail/utility/hash_peek_bitset.hpp>
#include <boost/xpressive/detail/utility/algorithm.hpp>
#include <boost/xpressive/detail/utility/any.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // alt_match_pred
    //
    template<typename BidiIter, typename Next>
    struct alt_match_pred
    {
        alt_match_pred(match_state<BidiIter> &state)
          : state_(&state)
        {
        }

        template<typename Xpr>
        bool operator ()(Xpr const &xpr) const
        {
            return xpr.BOOST_NESTED_TEMPLATE push_match<Next>(*this->state_);
        }

    private:
        match_state<BidiIter> *state_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // alt_match
    //
    template<typename BidiIter, typename Next>
    inline bool alt_match
    (
        alternates_vector<BidiIter> const &alts, match_state<BidiIter> &state, Next const &
    )
    {
        return detail::any(alts.begin(), alts.end(), alt_match_pred<BidiIter, Next>(state));
    }

    template<typename Head, typename Tail, typename BidiIter, typename Next>
    inline bool alt_match
    (
        alternates_list<Head, Tail> const &alts, match_state<BidiIter> &state, Next const &
    )
    {
        return fusion::any(alts, alt_match_pred<BidiIter, Next>(state));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // alternate_matcher
    template<typename Alternates, typename Traits>
    struct alternate_matcher
      : quant_style<
            Alternates::width != unknown_width::value && Alternates::pure ? quant_fixed_width : quant_variable_width
          , Alternates::width
          , Alternates::pure
        >
    {
        typedef Alternates alternates_type;
        typedef typename Traits::char_type char_type;

        Alternates alternates_;
        mutable hash_peek_bitset<char_type> bset_;

        explicit alternate_matcher(Alternates const &alternates = Alternates())
          : alternates_(alternates)
          , bset_()
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            if(!state.eos() && !this->can_match_(*state.cur_, traits_cast<Traits>(state)))
            {
                return false;
            }

            return detail::alt_match(this->alternates_, state, next);
        }

        detail::width get_width() const
        {
            // Only called when constructing static regexes, and this is a
            // set of same-width alternates where the widths are known at compile
            // time, as in: sregex rx = +(_ | 'a' | _n);
            BOOST_MPL_ASSERT_RELATION(unknown_width::value, !=, Alternates::width);
            return Alternates::width;
        }

    private:
        alternate_matcher &operator =(alternate_matcher const &);

        bool can_match_(char_type ch, Traits const &tr) const
        {
            return this->bset_.test(ch, tr);
        }
    };

}}}

#endif
