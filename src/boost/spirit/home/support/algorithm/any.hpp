/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ANY_APRIL_22_2006_1147AM)
#define BOOST_SPIRIT_ANY_APRIL_22_2006_1147AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/fusion/include/equal_to.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/deref.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/end.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/spirit/home/support/unused.hpp>

namespace boost { namespace spirit
{
    // This is the binary version of fusion::any. This might
    // be a good candidate for inclusion in fusion algorithm

    namespace detail
    {
        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any(First1 const&, First2 const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any(First1 const& first1, First2 const& first2, Last const& last, F& f, mpl::false_)
        {
            return f(*first1, *first2) ||
                detail::any(
                    fusion::next(first1)
                  , fusion::next(first2)
                  , last
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First1>::type, Last>());
        }
    }

    template <typename Sequence1, typename Sequence2, typename F>
    inline bool
    any(Sequence1 const& seq1, Sequence2& seq2, F f)
    {
        return detail::any(
                fusion::begin(seq1)
              , fusion::begin(seq2)
              , fusion::end(seq1)
              , f
              , fusion::result_of::equal_to<
                    typename fusion::result_of::begin<Sequence1>::type
                  , typename fusion::result_of::end<Sequence1>::type>());
    }

    template <typename Sequence, typename F>
    inline bool
    any(Sequence const& seq, unused_type, F f)
    {
        return fusion::any(seq, f);
    }

}}

#endif

