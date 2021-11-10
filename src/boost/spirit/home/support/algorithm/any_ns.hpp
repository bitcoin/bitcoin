/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ANY_NS_MARCH_13_2007_0827AM)
#define BOOST_SPIRIT_ANY_NS_MARCH_13_2007_0827AM

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
    // A non-short circuiting (ns) version of the any algorithm (uses
    // | instead of ||.

    namespace detail
    {
        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any_ns(First1 const&, First2 const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any_ns(First1 const& first1, First2 const& first2, Last const& last, F& f, mpl::false_)
        {
            return (0 != (f(*first1, *first2) |
                detail::any_ns(
                    fusion::next(first1)
                  , fusion::next(first2)
                  , last
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First1>::type, Last>())));
        }

        template <typename First, typename Last, typename F>
        inline bool
        any_ns(First const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First, typename Last, typename F>
        inline bool
        any_ns(First const& first, Last const& last, F& f, mpl::false_)
        {
            return (0 != (f(*first) |
                detail::any_ns(
                    fusion::next(first)
                  , last
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First>::type, Last>())));
        }
    }

    template <typename Sequence1, typename Sequence2, typename F>
    inline bool
    any_ns(Sequence1 const& seq1, Sequence2& seq2, F f)
    {
        return detail::any_ns(
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
    any_ns(Sequence const& seq, unused_type, F f)
    {
        return detail::any_ns(
                fusion::begin(seq)
              , fusion::end(seq)
              , f
              , fusion::result_of::equal_to<
                    typename fusion::result_of::begin<Sequence>::type
                  , typename fusion::result_of::end<Sequence>::type>());
    }

}}

#endif

